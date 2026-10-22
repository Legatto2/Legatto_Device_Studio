# Disposable Android emulator integration

This lane tests Legatto Device Studio with genuine Android SDK tools and two
disposable Android Virtual Devices (AVDs). It complements the deterministic fake
ADB tests by exercising the actual service and Qt window against a running
Android environment.

**Initial execution status: Not run.** The test implementation, workflow, or
successful compilation alone is not execution evidence. Update the run record
below only from the completed run and its retained artifacts.

## Scope and acceptance

| Test | Required behavior |
| --- | --- |
| `realToolDiscoveryAndReports` | Verify the selected native SDK ADB, discover both owned emulators, explicitly select each target in the real window, inspect its allowed properties, and verify the exported report against that target |
| `cancellationAndRecovery` | Cancel through the actual UI at the first inspection progress signal, before command dispatch; observe one terminal outcome and complete a subsequent real-tool inspection |
| `disconnectRejectsStaleSelection` | Stop the selected owned emulator, reject its stale selection/report, and leave the other emulator usable |
| `freshScanAfterRestart` | After the runner restarts emulator A, launch a new test process/window and require fresh discovery and explicit selection before successful reports for both targets |

The harness creates and stops synthetic devices to exercise lifecycle behavior.
The diagnostics application retains its read-only command boundary. This lane
does not run the legacy application, servicing tools, account services, firmware
operations, or tests on phones. It does not establish physical USB reliability,
Motorola/MediaTek compatibility, accessibility acceptance, or a working Windows
installer. Those require their own evidence in
[DEVICE_TEST_MATRIX.md](DEVICE_TEST_MATRIX.md).

Cancellation in this integration scenario is deliberately triggered before
dispatch so it does not depend on emulator speed; it is not evidence of
interrupting an already running ADB command. Fake-process tests cover that
failure boundary. The restart scenario opens a fresh window/process; it does
not establish how an already open window handles a target returning with the
same serial. Two instances of the same image can report identical property
values, so the ordinary argument-binding tests remain necessary alongside the
real-tool report comparisons.

## Isolation and ownership

Run on a dedicated development/CI session with no other ADB devices, including
USB phones, TCP/wireless connections, and personal emulator sessions. Close
competing emulator workloads before beginning. Do not sign into a personal or
customer account in the disposable AVDs or copy customer data into them.

The runner requires `--allow-disposable-emulators`, creates isolated AVD state,
and tracks the emulator processes it starts. The C++ integration test also
requires the explicit test environment, expected A/B serials, an emulator qemu
property, and the exact runner-created AVD names. A serial that merely looks like
`emulator-…` is not sufficient proof of ownership.

The runner reserves console ports 5554 and 5556, uses randomized
`legatto-test-…-a` / `legatto-test-…-b` names, and requires a new or empty
`--evidence-dir`. It rejects an initially nonempty ADB device list. Use a new
evidence directory for a rerun so a prior success cannot be mistaken for the
current result.

ADB uses the application's local server endpoint, `127.0.0.1:5037`. That server
can already know about USB, TCP, or wireless transports; an isolated AVD directory
does not isolate the server. The runner must reject unexpected devices rather
than disconnect or stop them. Do not bypass its guard to use an existing device.

Cleanup applies only to the runner's own emulator processes and temporary AVD
state. It must not run global `adb kill-server`, terminate unrelated emulator
processes, or remove a developer's AVD directory. Application cancellation stops
its client operation, not the shared ADB server. If a run is interrupted, verify
that its owned processes exited before starting another; resolve any remaining
owned process by its recorded session identity, never by killing every `adb`
or emulator process on the host.

## Prerequisites

- The ordinary development toolchain in [DEVELOPMENT.md](DEVELOPMENT.md):
  CMake, a C++17 compiler, compatible Qt Core/Widgets/Test, and Python 3.8+.
  The dedicated CI configuration uses Qt 6.
- An x86-64 Linux host with KVM available to the current user. The automated
  runner currently supports Linux only and rejects other hosts before invoking
  SDK tools. Check the installed emulator with `emulator -accel-check` and follow
  the [official acceleration guide](https://developer.android.com/studio/run/emulator-acceleration)
  for host setup. The test runner does not install drivers, elevate, or change
  virtualization settings.
- An Android SDK containing Platform-Tools, Emulator, Command-Line Tools, and
  `system-images;android-35;google_apis;x86_64`. Install from the
  [official Android SDK distribution](https://developer.android.com/studio).
  A compatible Java runtime is required by the SDK command-line tools.
- Enough RAM, disk, and available virtualization capacity to boot two emulators
  concurrently. A failed acceleration or resource prerequisite is an
  infrastructure failure, not a pass or a reason to test a connected phone.
- Review and accept the applicable SDK package licenses through Android Studio
  or the SDK Manager before running. The
  [SDK Manager documentation](https://developer.android.com/tools/sdkmanager)
  describes package installation and interactive `--licenses` handling.

The integration uses the SDK's `emulator` and `avdmanager` interfaces. Android's
current documentation also describes their replacement Android CLI; changes to
that interface need a separately verified runner update. Use the actual package
revisions recorded by each run rather than assuming that an SDK directory named
`latest` or a fixed API package name identifies immutable bytes.

## Linux local run

Run from the repository root. Set the SDK path to your installation. The package
installation command below is a one-time preparation step; review any license
prompt yourself. It can download substantial data.

```sh
export LEGATTO_SDK_ROOT="$HOME/Android/Sdk"
"$LEGATTO_SDK_ROOT/cmdline-tools/latest/bin/sdkmanager" \
  "platform-tools" "emulator" "system-images;android-35;google_apis;x86_64"
"$LEGATTO_SDK_ROOT/emulator/emulator" -accel-check

cmake -S . -B build/emulator-integration -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DLEGATTO_QT_MAJOR=6 \
  -DBUILD_TESTING=ON \
  -DLEGATTO_BUILD_EMULATOR_TESTS=ON
cmake --build build/emulator-integration --target emulator_integration_tests

QT_QPA_PLATFORM=offscreen python3 scripts/run_emulator_integration.py \
  --sdk-root "$LEGATTO_SDK_ROOT" \
  --test-executable build/emulator-integration/emulator_integration_tests \
  --system-image "system-images;android-35;google_apis;x86_64" \
  --evidence-dir build/emulator-evidence \
  --allow-disposable-emulators
```

This uses a separate CMake cache. The normal `linux-debug` and `linux-qt6`
presets remain hardware-free. Enabling the integration build option is not
permission to replace `LEGATTO_TEST_ADB` in ordinary CTest with genuine ADB.

## Windows status and manual evaluation

The application continues to have native Windows compilation and ordinary
fake-tool tests. See [DEVELOPMENT.md](DEVELOPMENT.md) for the MSVC/Qt build.
The automated emulator runner is **Linux only**: Windows process-tree ownership
and cleanup have not been implemented/validated, so it intentionally rejects
Windows rather than promising safe cross-platform cleanup.

A developer can separately evaluate the Windows application against a dedicated
disposable emulator using the normal tool-selection, discovery, inspection, and
export UI. Android recommends Windows Hypervisor Platform (WHPX); use
`emulator.exe -accel-check` and the
[official host setup guide](https://developer.android.com/studio/run/emulator-acceleration).
Run this as a documented manual session with no customer devices or data.
It does not substitute for the four automated scenarios or for physical USB,
driver, accessibility, or installer acceptance.

Native Windows emulator evaluation and automated Windows runner validation remain
**Not run**. No packaged application acceptance is implied by a development build.

## Dedicated GitHub Actions lane

[`Android emulator integration`](../.github/workflows/emulator-integration.yml)
is separate from the ordinary build/test workflow. It runs on relevant pull
requests to `main` and can be started manually using `workflow_dispatch`.
The PR path filter covers diagnostics code, the integration test/runner, CMake,
and this workflow. There is no general push trigger.

The job uses Ubuntu 24.04, KVM, Qt 6, and two API 35 Google APIs x86-64 AVDs.
The system-image identifier fixes the Android/API family, not an immutable
package revision. The run manifest records the installed SDK package versions.
Do not describe the lane as byte-for-byte reproducible until every relevant
tool/image input is pinned and verified.

The job has a 30-minute limit and uploads sanitized evidence with seven-day
retention. It does not upload emulator disks, Android logs, or a distributable
application package. If a run fails before evidence is written or the artifact has
expired, record that limitation instead of inferring a pass from a job name.

## Evidence and interpretation

The runner writes `summary.json` and sanitized Qt test summaries
`qt-initial.txt` / `qt-restarted.txt` in `--evidence-dir`, together with the
reviewed `report-*.json` exports. The summary records the named test cases. Evidence must
identify the source revision, host OS/architecture, Qt/compiler configuration,
ADB version and SHA-256, emulator version, system-image package and revision,
named test outcomes, and cleanup result. Keep the GitHub run URL when applicable.
Distinguish the PR source revision from any merge revision used by CI.

Synthetic report exports may be retained only after checking that they came from
the verified disposable AVDs. Report schema checks should reject raw serial/path
or transcript fields; also inspect allowed property values before sharing.
Do not publish customer logs, personal AVD contents, host usernames/private paths,
authentication data, or full environment dumps. Emulator console output is not a
substitute for a test result and should not be included in public evidence by
default.

| Run | Source revision | Environment and SDK evidence | Named checks | Result | Evidence |
| --- | --- | --- | --- | --- | --- |
| Initial implementation | Not recorded | Not recorded | Four scenarios above | **Not run** | — |
| Native Windows emulator session | Not recorded | Not recorded | Four scenarios above | **Not run** | — |

Record **Passed** only when all required scenarios executed successfully and
cleanup completed. Use **Failed** for an observed failed assertion/operation and
**Blocked** for unmet infrastructure prerequisites. Missing or interrupted
evidence is **Inconclusive**. A successful result applies only to the recorded
source and tool/image combination; it does not turn the physical-device matrix
green.

## Failure recovery

| Symptom | Next step |
| --- | --- |
| Existing/unknown ADB transport or AVD name mismatch | Stop the test; return to a dedicated session and inspect ownership. Do not force the guard or disconnect someone else's device. |
| SDK package, Java, Qt DLL/plugin, or test executable missing | Fix the declared development prerequisites and retry with the same intended target. Do not substitute an arbitrary executable. |
| Hardware acceleration unavailable | Follow the host-specific official setup guide; record the run as blocked until usable. |
| Boot timeout or emulator exits | Review the bounded local diagnostic information, SDK revisions, and available resources. Do not turn a timeout into a passing or skipped check. |
| Cancellation/disconnect assertion fails | Keep the source revision and sanitized evidence, reproduce with fake fixtures where possible, and fix the service/UI behavior before claiming that scenario passed. |
| Cleanup incomplete after interruption | Identify only the runner-owned processes/state and finish their cleanup; do not reset the shared ADB server. |

## Official references

- [Android Emulator hardware acceleration](https://developer.android.com/studio/run/emulator-acceleration)
- [Emulator command-line behavior and ports](https://developer.android.com/studio/run/emulator-commandline)
- [SDK Manager package and license handling](https://developer.android.com/tools/sdkmanager)
- [AVD Manager configuration and creation](https://developer.android.com/tools/avdmanager)
- [Android SDK Platform-Tools](https://developer.android.com/tools/releases/platform-tools)
