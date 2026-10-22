"""Runner prerequisite/privacy checks. These tests never execute SDK tools."""

import argparse
import importlib.util
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
import time
import unittest
from unittest import mock


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("emulator_runner", ROOT / "scripts/run_emulator_integration.py")
RUNNER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(RUNNER)


class EmulatorRunnerGuards(unittest.TestCase):
    def test_missing_opt_in_precedes_any_file_or_process_access(self):
        args = argparse.Namespace(system_image=RUNNER.DEFAULT_IMAGE,
                                  allow_disposable_emulators=False)
        session = RUNNER.Session(args)
        with mock.patch.object(session, "command") as command:
            with self.assertRaisesRegex(RUNNER.SessionError, "Explicit --allow"):
                session.preflight()
            command.assert_not_called()

    def test_image_id_cannot_escape_sdk_or_supply_tool_arguments(self):
        for value in ("../../private", "system-images;../android-35;google_apis;x86_64",
                      "system-images;android-35;google_apis;x86_64;--force",
                      "system-images;android-35;google_apis;arm64-v8a"):
            with self.subTest(value=value), self.assertRaises(RUNNER.SessionError):
                RUNNER.image_relative_path(value)
        self.assertEqual(RUNNER.image_relative_path(RUNNER.DEFAULT_IMAGE),
                         Path("system-images/android-35/google_apis/x86_64"))

    def test_non_linux_refusal_precedes_sdk_commands(self):
        args = argparse.Namespace(system_image=RUNNER.DEFAULT_IMAGE,
                                  allow_disposable_emulators=True)
        session = RUNNER.Session(args)
        with mock.patch.object(RUNNER.sys, "platform", "win32"), mock.patch.object(session, "command") as command:
            with self.assertRaisesRegex(RUNNER.SessionError, "supports Linux only"):
                session.preflight()
            command.assert_not_called()

    def test_preexisting_device_refusal_does_not_disclose_its_identifier(self):
        for serial in ("customer-private-serial", "emulator-5554", "192.0.2.3:5555"):
            with self.subTest(serial=serial), self.assertRaises(RUNNER.SessionError) as caught:
                RUNNER.require_empty_inventory("List of devices attached\n" + serial + " device\n")
            self.assertNotIn(serial, str(caught.exception))
        RUNNER.require_empty_inventory("List of devices attached\n\n")

    def test_invalid_or_duplicate_inventory_cannot_look_empty(self):
        for output in ("", "emulator-5554 device\n", "List of devices attached\nList of devices attached\n",
                       "List of devices attached\nemulator-5554 device\nemulator-5554 offline\n"):
            with self.subTest(output=output), self.assertRaises(RUNNER.SessionError):
                RUNNER.parse_inventory(output)

    def test_version_evidence_omits_installed_path(self):
        version = RUNNER.tool_version("Android Debug Bridge version 1.0.41\nVersion 37.0.1\n"
                                      "Installed as /home/private/Android/adb\n", "Android Debug Bridge version ")
        self.assertEqual(version, "Android Debug Bridge version 1.0.41")

    def test_qt_transcript_removes_supplied_and_incidental_private_paths(self):
        text = ("FAIL! : case\n   Loc: [/checkout/tests/emulator.cpp(21)]\n"
                "QStandardPaths: XDG_RUNTIME_DIR not set, defaulting to '/tmp/runtime-user'\n"
                "tool=/opt/private-sdk/platform-tools/adb\n"
                "other=C:\\Users\\private\\sdk\\adb.exe\n"
                "Totals: 2 passed, 1 failed, 0 skipped, 0 blacklisted, 7ms\n")
        sanitized = RUNNER.sanitized_transcript(text, [Path("/opt/private-sdk")])
        for value in ("/checkout", "/tmp/runtime-user", "/opt/private-sdk", "C:\\Users\\private"):
            self.assertNotIn(value, sanitized)
        self.assertIn("Totals: 2 passed, 1 failed, 0 skipped", sanitized)

    def test_executable_symlink_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            source = Path(directory) / "tool"
            source.write_bytes(b"fixture")
            link = Path(directory) / "link"
            # Windows standard users may not have symlink-creation privileges.
            try:
                link.symlink_to(source)
            except OSError:
                with mock.patch.object(Path, "is_symlink", return_value=True):
                    with self.assertRaises(RUNNER.SessionError):
                        RUNNER.regular_file(source)
            else:
                with self.assertRaises(RUNNER.SessionError):
                    RUNNER.regular_file(link)


@unittest.skipUnless(sys.platform == "linux", "Linux process-group ownership regression")
class OwnedProcessGroupCleanup(unittest.TestCase):
    def exercise_orphan_cleanup(self, ignore_term):
        child_source = (
            "from pathlib import Path; import signal,sys,time; "
            "signal.signal(signal.SIGTERM, signal.SIG_IGN) if sys.argv[2] == '1' else None; "
            "Path(sys.argv[1]).write_text('ready'); time.sleep(30)"
        )
        launcher_source = (
            "from pathlib import Path; import subprocess,sys,time\n"
            "subprocess.Popen([sys.executable, '-c', sys.argv[1], sys.argv[2], sys.argv[4]], "
            "stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)\n"
            "while not Path(sys.argv[3]).exists(): time.sleep(0.01)\n"
        )
        with tempfile.TemporaryDirectory() as directory:
            ready = Path(directory) / "child-ready"
            release = Path(directory) / "release-launcher"
            process = subprocess.Popen(
                [sys.executable, "-c", launcher_source, child_source, str(ready), str(release), "1" if ignore_term else "0"],
                stdin=subprocess.DEVNULL, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                start_new_session=True,
            )
            group = None
            try:
                group = RUNNER.OwnedProcessGroup(process)
                deadline = time.monotonic() + 2
                while not ready.exists() and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertTrue(ready.exists(), "The harmless child did not start in time")
                release.write_text("exit")
                process.wait(timeout=2)
                self.assertTrue(group.alive(), "The child should outlive its exited launcher")
                session = RUNNER.Session(argparse.Namespace(system_image=RUNNER.DEFAULT_IMAGE))
                session.processes = [(0, process)]
                session.process_groups[process.pid] = group
                started = time.monotonic()
                self.assertTrue(session.cleanup())
                self.assertLess(time.monotonic() - started, 3)
                self.assertFalse(group.alive(), "Cleanup left a running descendant")
                self.assertEqual(session.summary["cleanup"], "passed")
            finally:
                # A failing regression must not leave its own harmless sleeper.
                if group is None or group.alive():
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                process.wait(timeout=2)

    def test_cleanup_stops_child_after_launcher_has_exited(self):
        self.exercise_orphan_cleanup(ignore_term=False)

    def test_cleanup_escalates_when_orphaned_child_ignores_term(self):
        self.exercise_orphan_cleanup(ignore_term=True)


if __name__ == "__main__":
    unittest.main()
