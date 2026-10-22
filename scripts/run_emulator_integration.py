#!/usr/bin/env python3
"""Opt-in Linux Android integration on two owned, disposable virtual devices.

This is deliberately separate from CTest and LEGATTO_TEST_ADB. SDK packages
must already be installed. No physical device, existing AVD, or shared ADB
server is stopped. The runner never installs an APK or changes phone data.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import signal
import socket
import subprocess
import sys
import tempfile
import time
import uuid
from datetime import datetime, timezone


DEFAULT_IMAGE = "system-images;android-35;google_apis;x86_64"
SERIALS = ("emulator-5554", "emulator-5556")
PORTS = (5554, 5556)
INITIAL_CASES = (
    "realToolDiscoveryAndReports",
    "cancellationAndRecovery",
    "disconnectRejectsStaleSelection",
)
RESTART_CASES = ("freshScanAfterRestart",)
PROPERTY_KEYS = {
    "Pkg.Revision", "Pkg.Desc", "AndroidVersion.ApiLevel",
    "SystemImage.Abi", "SystemImage.TagId", "SystemImage.VendorId",
}
ADB_ENV = {
    "ANDROID_SERIAL", "ADB_SERVER_SOCKET", "ANDROID_ADB_SERVER_PORT",
    "ADB_SERVER_PORT", "ADB_TRACE", "ADB_MDNS_AUTO_CONNECT",
    "LEGATTO_TEST_ADB",
}


class SessionError(Exception):
    """A fixed, non-sensitive error suitable for public CI evidence."""


def image_relative_path(package):
    if not re.fullmatch(r"system-images;android-[1-9][0-9]*;google_apis;x86_64", package):
        raise SessionError("Use an official google_apis x86_64 Android system-image package ID.")
    return Path(*package.split(";"))


def parse_inventory(output):
    devices = {}
    header = False
    for line in output.splitlines():
        line = line.strip()
        if not line or line.startswith("* daemon "):
            continue
        if line == "List of devices attached":
            if header:
                raise SessionError("ADB returned an ambiguous device inventory.")
            header = True
            continue
        fields = line.split()
        if not header or len(fields) < 2 or fields[0] in devices:
            raise SessionError("ADB returned an invalid device inventory.")
        devices[fields[0]] = fields[1]
    if not header:
        raise SessionError("ADB did not return a device inventory.")
    return devices


def require_empty_inventory(output):
    if parse_inventory(output):
        raise SessionError("Disconnect all devices and close existing emulators before this dedicated session.")


def tool_version(output, prefix):
    # In particular, do not retain ADB's 'Installed as /private/path/adb' line.
    lines = [line.strip() for line in output.splitlines()
             if line.strip().startswith(prefix)]
    if len(lines) != 1 or len(lines[0]) > 256:
        raise SessionError("A required SDK version could not be identified.")
    return lines[0]


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def regular_file(path, executable=False):
    if path.is_symlink() or not path.is_file():
        raise SessionError("A required SDK or test executable is missing or is a symbolic link.")
    if executable and os.name != "nt" and not os.access(path, os.X_OK):
        raise SessionError("A required SDK or test file is not executable.")
    return path.resolve(strict=True)


def package_metadata(directory):
    source = regular_file(directory / "source.properties")
    values = {}
    for line in source.read_text(encoding="utf-8").splitlines():
        key, separator, value = line.partition("=")
        if separator and key.strip() in PROPERTY_KEYS:
            value = value.strip()
            if len(value) > 256 or any(ord(ch) < 32 for ch in value):
                raise SessionError("An installed SDK metadata value is invalid.")
            values[key.strip()] = value
    if not values.get("Pkg.Revision"):
        raise SessionError("An installed SDK package has no recorded revision.")
    return {"properties": values, "metadataSha256": sha256(source)}


def sanitized_transcript(output, private_paths):
    for path in sorted({str(p) for p in private_paths if str(p)}, key=len, reverse=True):
        output = output.replace(path, "[local-path]").replace(path.replace("\\", "/"), "[local-path]")
    # QTest failure locations and environment warnings can include paths not
    # supplied on the command line. Only these fixed test-program messages ship.
    output = re.sub(r"(?m)^.*(?:Installed as |XDG_RUNTIME_DIR|QStandardPaths:).*$", "[local environment omitted]", output)
    output = re.sub(r"(?m)^(\s*Loc:\s*).*?$", r"\1[local source location]", output)
    output = re.sub(r"(?i)(?:[A-Z]:[\\/]|/(?:home|Users|tmp|private|workspace)/)[^\s\"<>]*", "[local-path]", output)
    return output


def process_identity(pid):
    """Read Linux process state, group, session and birth time without running tools."""
    try:
        fields = (Path("/proc") / str(pid) / "stat").read_text().rsplit(") ", 1)[1].split()
        return fields[0], int(fields[2]), int(fields[3]), int(fields[19])
    except (FileNotFoundError, ProcessLookupError):
        return None
    except (OSError, ValueError, IndexError) as error:
        raise SessionError("An owned process group could not be inspected safely.") from error


class OwnedProcessGroup:
    """Track a newly created Linux session even after its launcher has exited."""

    def __init__(self, process):
        self.process = process
        self.pgid = process.pid
        identity = process_identity(self.pgid)
        if identity is None or identity[1:3] != (self.pgid, self.pgid):
            raise SessionError("The emulator launcher did not have its own process group and session.")
        self.birth = identity[3]

    def alive(self):
        leader = process_identity(self.pgid)
        if leader is not None and leader[3] != self.birth:
            # The original session is gone. Never signal a subsequently reused PID.
            return False
        try:
            os.killpg(self.pgid, 0)
        except ProcessLookupError:
            return False
        except OSError as error:
            raise SessionError("An owned process group could not be inspected safely.") from error
        found = False
        for entry in Path("/proc").iterdir():
            if not entry.name.isdecimal():
                continue
            try:
                identity = process_identity(int(entry.name))
            except SessionError:
                # Other users' processes may be hidden. The owned session's
                # processes are readable; an unconfirmed group stays live below.
                continue
            if identity is not None and identity[1:3] == (self.pgid, self.pgid):
                found = True
                if identity[0] not in ("Z", "X"):
                    return True
        if found:
            # Exited descendants can await reaping by init; they hold no live VM.
            return False
        try:
            os.killpg(self.pgid, 0)
            return True  # A changing or unreadable group is never assumed stopped.
        except ProcessLookupError:
            return False

    def stop(self, terminate_timeout=1.0, kill_timeout=1.0):
        for action, allowance in ((signal.SIGTERM, terminate_timeout), (signal.SIGKILL, kill_timeout)):
            if not self.alive():
                self.process.poll()
                return True
            try:
                os.killpg(self.pgid, action)
            except ProcessLookupError:
                self.process.poll()
                return True
            deadline = time.monotonic() + allowance
            while time.monotonic() < deadline:
                self.process.poll()
                if not self.alive():
                    return True
                time.sleep(0.05)
        self.process.poll()
        return not self.alive()


class Session:
    def __init__(self, args):
        self.args = args
        self.environment = {key: value for key, value in os.environ.items() if key not in ADB_ENV}
        self.environment["ADB_MDNS_AUTO_CONNECT"] = "0"
        self.processes = []
        self.process_groups = {}
        self.logs = []
        self.temp = None
        self.adb = None
        self.names = []
        self.evidence = None
        self.stage = "preflight"
        self.started = time.monotonic()
        self.summary = {
            "schemaVersion": 1,
            "startedAt": datetime.now(timezone.utc).isoformat(),
            "outcome": "failed",
            "host": {"system": platform.system(), "machine": platform.machine(), "python": platform.python_version()},
            "systemImage": args.system_image,
            "deviceCount": 2,
            "phases": {},
            "cleanup": "not_started",
            "scope": "Android emulator integration; no physical-device acceptance",
        }

    def command(self, args, timeout=15, input_text=None, check=True):
        try:
            result = subprocess.run(
                [str(arg) for arg in args], input=input_text, text=True, encoding="utf-8",
                errors="replace", stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                timeout=timeout, env=self.environment, shell=False,
            )
        except subprocess.TimeoutExpired as error:
            raise SessionError("A setup or reference command exceeded its deadline.") from error
        except OSError as error:
            raise SessionError("A required setup or reference command could not start.") from error
        if len(result.stdout) + len(result.stderr) > 1024 * 1024:
            raise SessionError("A setup or reference command exceeded its output allowance.")
        if check and result.returncode:
            raise SessionError("A required setup or reference command failed.")
        return result

    def adb_command(self, *args, timeout=10, check=True):
        return self.command([self.adb, "-H", "127.0.0.1", "-P", "5037", *args], timeout, check=check)

    def inventory(self):
        return parse_inventory(self.adb_command("devices", "-l").stdout)

    def announce(self, stage):
        self.stage = stage
        print("Emulator integration: " + stage, flush=True)

    def preflight(self):
        if not self.args.allow_disposable_emulators:
            raise SessionError("Explicit --allow-disposable-emulators is required; no SDK command was run.")
        if sys.platform != "linux":
            raise SessionError("This disposable-emulator runner supports Linux only; no SDK command was run.")
        relative = image_relative_path(self.args.system_image)
        sdk = self.args.sdk_root.expanduser().resolve(strict=True)
        self.adb = regular_file(sdk / "platform-tools/adb", True)
        self.emulator = regular_file(sdk / "emulator/emulator", True)
        self.avdmanager = regular_file(sdk / "cmdline-tools/latest/bin/avdmanager", True)
        self.test = regular_file(self.args.test_executable.expanduser().absolute(), True)
        evidence = self.args.evidence_dir.expanduser().absolute()
        if evidence.is_symlink() or (evidence.exists() and (not evidence.is_dir() or any(evidence.iterdir()))):
            raise SessionError("Use a new or empty evidence directory; existing evidence will not be overwritten.")
        evidence.mkdir(parents=True, exist_ok=True)
        self.evidence = evidence.resolve(strict=True)
        self.summary["sdk"] = {
            "platformTools": package_metadata(sdk / "platform-tools"),
            "emulator": package_metadata(sdk / "emulator"),
            "systemImage": package_metadata(sdk / relative),
        }
        installed = self.summary["sdk"]["systemImage"]["properties"]
        if (installed.get("AndroidVersion.ApiLevel"), installed.get("SystemImage.Abi"), installed.get("SystemImage.TagId")) != (relative.parts[1].split("-")[1], "x86_64", "google_apis"):
            raise SessionError("The installed system image does not match the requested API, ABI and tag.")
        self.summary["testExecutableSha256"] = sha256(self.test)
        self.summary["adbSha256"] = sha256(self.adb)
        self.sdk = sdk
        self.temp = tempfile.TemporaryDirectory(prefix="legatto-avd-")
        self.private = Path(self.temp.name)
        for key, directory in (("ANDROID_AVD_HOME", "avds"), ("ANDROID_USER_HOME", "user"), ("ANDROID_EMULATOR_HOME", "emulator")):
            path = self.private / directory
            path.mkdir()
            self.environment[key] = str(path)
        self.environment["ANDROID_SDK_ROOT"] = str(sdk)
        self.environment["ANDROID_HOME"] = str(sdk)
        self.environment["QT_QPA_PLATFORM"] = "offscreen"
        # Checks happen before creating or launching any AVD. Never reset the
        # host's shared server to make this prerequisite pass.
        require_empty_inventory(self.adb_command("devices", "-l").stdout)
        sockets = []
        try:
            for port in (5554, 5555, 5556, 5557):
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sockets.append(sock)
                sock.bind(("127.0.0.1", port))
        except OSError as error:
            raise SessionError("The reserved emulator ports are already in use.") from error
        finally:
            for sock in sockets:
                sock.close()
        self.command([self.emulator, "-accel-check"], timeout=30)
        self.summary["adbVersion"] = tool_version(self.adb_command("version").stdout, "Android Debug Bridge version ")
        version = self.command([self.emulator, "-version"], timeout=30)
        self.summary["emulatorVersion"] = tool_version(version.stdout + "\n" + version.stderr, "Android emulator version ")
        revision = self.command(["git", "-C", Path(__file__).resolve().parents[1], "rev-parse", "HEAD"], check=False)
        if re.fullmatch(r"[a-f0-9]{40}\n?", revision.stdout):
            self.summary["sourceCommit"] = revision.stdout.strip()
        else:
            self.summary["sourceCommit"] = None
        prefix = "legatto-test-" + uuid.uuid4().hex
        self.names = [prefix + "-a", prefix + "-b"]

    def create_avds(self):
        self.announce("create disposable virtual devices")
        for name in self.names:
            avd_path = self.private / (name + ".avd")
            self.command([self.avdmanager, "create", "avd", "--name", name,
                          "--package", self.args.system_image, "--path", avd_path, "--force"],
                         timeout=60, input_text="no\n")
            config = avd_path / "config.ini"
            settings = {}
            for line in config.read_text(encoding="utf-8").splitlines():
                key, separator, value = line.partition("=")
                if separator:
                    settings[key.strip()] = value.strip()
            settings.update({"hw.ramSize": "1536", "hw.cpu.ncore": "2", "hw.lcd.width": "720",
                             "hw.lcd.height": "1280", "hw.lcd.density": "320", "disk.dataPartition.size": "2G",
                             "hw.audioInput": "no", "hw.audioOutput": "no", "showDeviceFrame": "no"})
            config.write_text("".join(key + "=" + value + "\n" for key, value in sorted(settings.items())), encoding="utf-8")

    def launch(self, index):
        log = (self.private / ("emulator-" + uuid.uuid4().hex + ".log")).open("wb")
        self.logs.append(log)
        process = subprocess.Popen([
            str(self.emulator), "-avd", self.names[index], "-port", str(PORTS[index]),
            "-no-window", "-no-audio", "-no-boot-anim", "-no-snapshot",
            "-gpu", "software", "-accel", "on", "-memory", "1536", "-cores", "2",
            "-camera-back", "none", "-camera-front", "none",
        ], stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT,
            env=self.environment, shell=False, start_new_session=True)
        self.processes.append((index, process))
        self.process_groups[process.pid] = OwnedProcessGroup(process)
        return process

    def identity(self, index):
        serial = SERIALS[index]
        qemu = [self.adb_command("-s", serial, "shell", "getprop", key, check=False)
                for key in ("ro.kernel.qemu", "ro.boot.qemu")]
        if not any(result.returncode == 0 and result.stdout.strip() == "1" for result in qemu):
            return False
        avd = self.adb_command("-s", serial, "emu", "avd", "name", check=False)
        lines = [line.strip() for line in avd.stdout.splitlines() if line.strip() and line.strip() != "OK"]
        return avd.returncode == 0 and lines == [self.names[index]]

    def wait_for_boot(self):
        deadline = time.monotonic() + 300
        while time.monotonic() < deadline:
            for index in range(2):
                latest = [process for owned_index, process in self.processes if owned_index == index][-1]
                if latest.poll() is not None:
                    raise SessionError("An owned emulator exited before boot completed.")
            devices = self.inventory()
            if set(devices) - set(SERIALS):
                raise SessionError("An unexpected device appeared; the dedicated session has been stopped.")
            if devices == dict.fromkeys(SERIALS, "device"):
                booted = all(self.adb_command("-s", serial, "shell", "getprop", "sys.boot_completed", check=False).stdout.strip() == "1" for serial in SERIALS)
                if booted:
                    if not all(self.identity(index) for index in range(2)):
                        raise SessionError("An emulator did not match the disposable session identity.")
                    return
            time.sleep(1)
        raise SessionError("The disposable virtual devices did not boot within 300 seconds.")

    def run_tests(self, phase, cases):
        self.announce("run " + phase + " Qt integration cases")
        if self.inventory() != dict.fromkeys(SERIALS, "device") or not all(self.identity(index) for index in range(2)):
            raise SessionError("The test preconditions changed before starting the integration executable.")
        self.environment.update({
            "LEGATTO_EMULATOR_ADB": str(self.adb),
            "LEGATTO_EMULATOR_SERIAL_A": SERIALS[0], "LEGATTO_EMULATOR_SERIAL_B": SERIALS[1],
            "LEGATTO_EMULATOR_AVD_A": self.names[0], "LEGATTO_EMULATOR_AVD_B": self.names[1],
            "LEGATTO_EMULATOR_ALLOW_DISPOSABLE": "1", "LEGATTO_EMULATOR_EVIDENCE_DIR": str(self.evidence),
        })
        started = time.monotonic()
        result = self.command([self.test, *cases, "-txt"], timeout=180, check=False)
        transcript = sanitized_transcript(result.stdout + "\n" + result.stderr,
                                         [self.sdk, self.test.parent, self.private, self.evidence,
                                          Path(__file__).resolve().parents[1], Path.home()])
        (self.evidence / ("qt-" + phase + ".txt")).write_text(transcript, encoding="utf-8")
        totals = re.search(r"Totals:\s+(\d+) passed,\s+(\d+) failed,\s+(\d+) skipped", transcript)
        passed = result.returncode == 0 and totals is not None and int(totals[1]) >= len(cases) + 2 and totals.groups()[1:3] == ("0", "0")
        self.summary["phases"][phase] = {"cases": list(cases), "outcome": "passed" if passed else "failed",
            "durationSeconds": round(time.monotonic() - started, 2), "exitCode": result.returncode,
            "totals": {"passed": int(totals[1]), "failed": int(totals[2]), "skipped": int(totals[3])} if totals else None}
        print(transcript, flush=True)
        if not passed:
            raise SessionError("An integration phase failed, skipped a case, or did not produce complete Qt results.")

    def cleanup(self):
        errors = []
        if self.processes:
            print("Emulator integration: stop owned virtual devices", flush=True)
        for index in range(len(self.names)):
            try:
                if self.inventory().get(SERIALS[index]) == "device" and self.identity(index):
                    self.adb_command("-s", SERIALS[index], "emu", "kill", check=False)
            except (SessionError, OSError):
                pass  # Owned process handles remain the bounded fallback.
        for _, process in reversed(self.processes):
            try:
                group = self.process_groups.get(process.pid)
                if group is None:
                    # Registration failed. Reap the exact child handle, but do
                    # not claim its unverified descendants have been cleaned up.
                    process.kill()
                    process.wait(timeout=2)
                    errors.append("owned_process_group_unavailable")
                elif not group.stop():
                    errors.append("owned_process_group_did_not_exit")
            except (subprocess.TimeoutExpired, OSError, SessionError):
                errors.append("owned_process_group_did_not_exit")
        for log in self.logs:
            log.close()
        if self.temp is not None:
            try:
                self.temp.cleanup()
            except OSError:
                errors.append("temporary_session_removal_failed")
        self.summary["cleanup"] = "passed" if not errors else "failed"
        if errors:
            self.summary["cleanupErrors"] = sorted(set(errors))
        return not errors

    def run(self):
        try:
            self.preflight()
            self.create_avds()
            self.announce("boot two virtual devices")
            first_a = self.launch(0)
            self.launch(1)
            self.wait_for_boot()
            self.run_tests("initial", INITIAL_CASES)
            self.announce("restart the disconnected virtual device")
            try:
                first_a.wait(timeout=30)
            except subprocess.TimeoutExpired as error:
                raise SessionError("The disconnected owned emulator did not exit.") from error
            if SERIALS[0] in self.inventory():
                raise SessionError("The disconnected emulator still appears in the ADB inventory.")
            if not self.process_groups[first_a.pid].stop():
                raise SessionError("The disconnected emulator still has a running owned process.")
            self.launch(0)
            self.wait_for_boot()
            self.run_tests("restarted", RESTART_CASES)
            self.summary["outcome"] = "passed"
        except KeyboardInterrupt:
            self.summary["error"] = "Session interrupted."
        except SessionError as error:
            self.summary["error"] = str(error)
        except (OSError, ValueError, KeyError, subprocess.SubprocessError):
            self.summary["error"] = "A required local file or process operation failed."
        finally:
            self.summary["lastStage"] = self.stage
            if not self.cleanup():
                self.summary["outcome"] = "failed"
            self.summary["durationSeconds"] = round(time.monotonic() - self.started, 2)
            self.summary["finishedAt"] = datetime.now(timezone.utc).isoformat()
            if self.evidence is not None:
                (self.evidence / "summary.json").write_text(json.dumps(self.summary, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        print("Emulator integration: " + self.summary["outcome"], flush=True)
        if "error" in self.summary:
            print(self.summary["error"], file=sys.stderr)
        return 0 if self.summary["outcome"] == "passed" else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sdk-root", type=Path, required=True)
    parser.add_argument("--test-executable", type=Path, required=True)
    parser.add_argument("--system-image", default=DEFAULT_IMAGE)
    parser.add_argument("--evidence-dir", type=Path, required=True)
    parser.add_argument("--allow-disposable-emulators", action="store_true")
    args = parser.parse_args(argv)
    def interrupted(_signum, _frame):
        raise KeyboardInterrupt
    signal.signal(signal.SIGTERM, interrupted)
    return Session(args).run()


if __name__ == "__main__":
    sys.exit(main())
