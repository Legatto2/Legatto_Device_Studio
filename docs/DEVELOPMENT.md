# Development guide

## Choose the correct application

| Target | Purpose | Dependencies |
| --- | --- | --- |
| `LegattoDeviceStudio` | Legatto Device Studio desktop diagnostics; default | C++17, Qt Core and Widgets |
| `LegacyServiceClient` | Preserved legacy client for engineering review; opt-in | Qt 5 Core, Widgets, Network, SerialPort; external runtime dependencies unresolved |
| Ordinary test targets | Deterministic hardware-free checks | Qt Test and Python; fake subprocess fixtures |
| `emulator_integration_tests` | Separately enabled genuine ADB/UI integration | Qt Test, Python, Android SDK, two disposable AVDs; `LEGATTO_BUILD_EMULATOR_TESTS=ON` |

The new application does not load the legacy client. Building that client does
not enable its workflows in diagnostics.

## Toolchain

- CMake **3.21+**.
- A compiler supporting **C++17**.
- Qt **5.15+** or **6.2+** for diagnostics. Legacy compilation is Qt 5 only.
- Qt Test and Python **3.8+** for the default test configuration.
- Ninja for the Linux presets; Visual Studio 2022/MSVC for the Windows preset.

Keep the compiler, architecture, and Qt kit compatible. A Qt MinGW distribution
cannot be mixed with the MSVC preset. For a different supported generator, use
a separate build directory and explicit CMake configuration.

### Linux / Qt 5

Install your distribution's Qt 5 development packages (Core/Widgets/Test), CMake,
Ninja, a C++ compiler, and Python. The optional legacy target also needs Network
and SerialPort development files.

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

### Linux / Qt 6

Install the corresponding Qt 6 development packages and use a separate preset:

```sh
cmake --preset linux-qt6
cmake --build --preset linux-qt6
ctest --preset linux-qt6
```

### Windows / MSVC

Install the Visual Studio 2022 C++ desktop workload, CMake, Python, and a matching
64-bit Qt MSVC kit. Run from a configured developer PowerShell/terminal. The path
below is an example; use the actual kit location on your machine.

```powershell
cmake --preset windows-msvc -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build --preset windows-msvc
ctest --preset windows-msvc
```

The preset selects the Release configuration for build/test. A successful source
build does not create a self-contained installer: the executable still needs the
matching Qt libraries/plugins. Do not label a compiler output folder a release.

The Linux executable is `build/linux-debug/LegattoDeviceStudio` (or under
`build/linux-qt6` for that preset). The Windows executable is
`build/windows-msvc/Release/LegattoDeviceStudio.exe`. Launch it from the configured
Qt environment until packaging/runtime deployment is completed.

### Optional legacy compilation

Use a Qt 5 configuration and enable the explicit option:

```sh
cmake --preset linux-debug -DLEGATTO_BUILD_LEGACY=ON
cmake --build --preset linux-debug
ctest --preset linux-debug
```

The same option can be added to the Windows Qt 5 configuration. It persists in
that CMake cache; set `-DLEGATTO_BUILD_LEGACY=OFF` when returning to a diagnostics-only
build. The original `LegacyServiceClient.pro` remains available to existing qmake users.
Do not launch the legacy application as an automatic build/test step.

## Working with ADB

Ordinary builds/tests do not require a phone or a real ADB installation. For a
deliberate manual diagnostics run, obtain Platform-Tools from the
[official Android source](https://developer.android.com/tools/releases/platform-tools),
select its native `adb`/`adb.exe`, inspect the tool details, refresh the device
list, select the intended authorized device, and request inspection.

Do not download an unknown replacement executable to satisfy a failing test.
Use the fake tool fixtures for process, parser, and failure-path testing. A local
hash identifies the selected file; authenticity still depends on acquisition.

ADB may start or reuse a local server. Diagnostics should not kill a shared server,
change driver packages, elevate, or send recovery commands automatically. Document
hardware outcomes separately in [DEVICE_TEST_MATRIX.md](DEVICE_TEST_MATRIX.md).

## Optional emulator integration

The dedicated [emulator guide](EMULATOR_TESTING.md) covers real Platform-Tools,
two disposable Android Virtual Devices, and the actual diagnostics window/service.
It exercises explicit selection and reports, cancellation/recovery, disconnect
handling, and a fresh scan after an emulator restart. The CMake option
`LEGATTO_BUILD_EMULATOR_TESTS` is **OFF by default**; ordinary CTest continues to
use fake ADB fixtures.

Use the documented Python runner in a dedicated session with no other ADB devices
or emulator workloads. It owns its AVD directories and emulator processes; it
does not reset the shared ADB server or clean unrelated sessions. Linux Qt 6 is
the dedicated CI lane; the automated runner currently supports Linux only.
The guide distinguishes manual Windows emulator evaluation from ordinary native
Windows build/tests and physical-device acceptance.

Record the exact source, SDK package revisions, and test outcomes. A successful
emulator run proves the tested synthetic Android environment; it does not fill
physical Motorola/MediaTek, USB driver, firmware-servicing, or packaged Windows
acceptance rows.

## Professional change workflow

1. Inspect the current branch/worktree and preserve unrelated work.
2. Describe the concrete behavior and acceptance criteria; identify the target
   application and any device, account, privacy, or dependency impact.
3. Make a small branch with one reviewable outcome. Prefer the new service/UI
   boundaries over modifying legacy command strings.
4. Add meaningful tests for changed behavior and failure cases. Avoid tests that
   merely assert the implementation text has a particular shape.
5. Run the applicable build/CTest preset. Use the other Qt/platform lane when
   the change affects cross-platform behavior or a required CI gate.
6. Review the diff for correctness, secrets, paths, resource ownership, and claims.
7. Open a PR with the problem, behavior change, actual tests, remaining limitations,
   and rollback implications. Wait for required CI/review before merging.
8. Release only after the relevant rights, packaging, and device gates are met.

See [CONTRIBUTING.md](../CONTRIBUTING.md) for contribution conventions and proposed
repository rules. Proposed branch-protection settings are recommendations until
actually enabled in GitHub.

## Test layers and what they prove

| Layer | Purpose | Does not prove |
| --- | --- | --- |
| Qt helper/unit tests | Preferences, parsing, selection, deterministic state behavior | Physical USB behavior or device compatibility |
| Fake-process service tests | Arguments, output handling, process results, cancellation/deadlines | Genuine ADB/driver behavior |
| UI tests/smoke checks | Controls, state transitions, layout constraints | Windows accessibility or physical-device safety unless specifically executed |
| Source guards | Inherited containment regression signals | Functional behavior by themselves |
| Linux/Windows CI | Native compilation and hardware-free test results | Packaged installation, external services, or phones |
| Dedicated emulator integration | Genuine ADB, two disposable Android instances, selected-device UI/report and lifecycle behavior | Physical USB/driver behavior, Motorola firmware support, or packaged Windows installation |
| Device/package acceptance | Specific real environments recorded in the matrix | All models, firmware, drivers, or future tool versions |

Do not collect legacy service credentials, access customer phones, or debit an
account to make CI pass. Ordinary tests use synthetic values and local fixtures;
the opt-in emulator lane uses only its verified disposable AVDs.

## Pipeline tools

| Tool | Role |
| --- | --- |
| Qt Creator or a CMake-capable editor | Code navigation, debugger, local builds |
| CMake and presets | Repeatable target/configuration selection |
| Ninja / MSVC build tools | Native compilation |
| Qt Test and CTest | Deterministic functional tests and aggregation |
| Python unittest | Existing source safety guards |
| GitHub Actions | Windows/Linux checks with pinned actions and limited permissions |
| Dependabot | Reviewable action dependency updates |
| GitHub PR/issue templates | Requirements, evidence, review, and defect tracking |

The repository does not require a paid editor, cloud backend, or AI service to
build and test. Keep additional tooling proportional to a demonstrated need.

## Release evidence

A future release record must identify the source revision, compiler/Qt/toolchain,
included dependencies/notices, artifact hashes, CI outcomes, Windows clean-machine
results, and applicable device rows. Document unresolved findings rather than
silently converting development artifacts into a customer-ready download.
