# Legatto Device Studio

### Know the device. See the result.

A Qt/C++ desktop workspace for read-only Android diagnostics. Choose the device,
inspect its reported information, and review exactly what you export.

Developed by **Michael Heald (Legatto2)**.
[Project repository](https://github.com/Legatto2/Legatto_Device_Studio).

**Development preview · `0.1.0-dev` · Windows-first · Qt 5 / Qt 6**

[Getting started](#getting-started) · [Build guide](docs/DEVELOPMENT.md) ·
[Roadmap](docs/ROADMAP.md) · [Architecture](docs/ARCHITECTURE.md) ·
[Engineering audit](docs/AUDIT.md) · [Privacy](PRIVACY.md) · [Disclaimer](DISCLAIMER.md)

![Legatto Device Studio development workspace; no device connected](docs/images/diagnostics-workspace.png)

## What this version does

Legatto Device Studio builds as `LegattoDeviceStudio`. It provides a focused
workflow: choose your Android tools, select a connected device, inspect it, and
review a report before exporting.

| Capability | Behavior |
| --- | --- |
| Guided tool setup | Select an explicit local ADB executable; see its version and SHA-256 fingerprint |
| Device discovery | See connected devices and their authorization/connection state |
| Explicit targeting | Select the intended device; inspection checks its identity and binds commands to it |
| Read-only inspection | Read manufacturer, model, Android version, build ID, and security-patch date |
| Clear execution state | Bounded requests, cancellation, and distinct success/error outcomes |
| Controlled reporting | Preview/export JSON with a session device pseudonym; omit raw serial/path/transcript fields |
| Usable workspace | Resizable desktop layout, keyboard-accessible controls, and connection guidance |

Tool checks run the executable you select. Obtain it from the official Android
Platform-Tools distribution; a displayed fingerprint identifies a file but does
not authenticate its publisher. ADB may start or reuse its local server.

This is a development build. Physical-device compatibility and an installable
Windows release are still being validated. There is no published production
installer or universal Motorola/MediaTek support claim.

## Getting started

1. **Build the diagnostics application** using the [development guide](docs/DEVELOPMENT.md).
2. **Get Android Platform-Tools** from [Google's official download](https://developer.android.com/tools/releases/platform-tools)
   or Android Studio's SDK Manager.
3. **Open Legatto Device Studio**, browse to the native `adb.exe` on Windows or
   `adb` on Linux, and choose **Check tool**.
4. Connect a device with USB debugging enabled and approve its debugging prompt.
   Choose **Refresh**, then select the intended device.
5. Choose **Inspect selected device**. Review **Device details** and **Export preview**;
   use **Export report** when you want a JSON file.

No account-service login is needed. Inspection uses a restricted set of
read-only commands.

### Privacy and reporting

Raw serials are excluded from exported reports. Device selection and ADB commands
still use the selected serial, and the local ADB server has its own transport and
authorization state. The application does not promise that serials remain only
inside its window. Review device-provided values before sharing an export; see
the [privacy statement](PRIVACY.md) for the full scope.

Reports identify the application as `Legatto Device Studio` and use
`schemaVersion: 1`. The export includes the five inspected properties, an ADB
fingerprint/version, a generation timestamp, and a device pseudonym that is
stable within the current application session.

### Quick build

With Qt development libraries, CMake, Ninja, a C++17 compiler, and Python installed:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

Use `linux-qt6` for the Qt 6 configuration. On Windows with a compatible Qt MSVC
kit and Visual Studio 2022:

```powershell
cmake --preset windows-msvc -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build --preset windows-msvc
ctest --preset windows-msvc
```

Replace the example Qt path with your installation. The [full guide](docs/DEVELOPMENT.md)
explains dependencies, output locations, runtime requirements, and tests.

## Professional development foundation

The repository separates UI presentation from diagnostic execution.
CMake presets and CTest provide repeatable local builds. GitHub Actions defines
Linux Qt 5, Linux Qt 6, and native Windows Qt 5 checks. Deterministic fake-tool
fixtures exercise behavior without phones or external accounts.

Contributions follow a small-branch, reviewable-PR workflow with explicit
acceptance criteria and recorded validation. CI logs, detailed findings, and
remaining gates belong in the [audit](docs/AUDIT.md),
[contributor guide](CONTRIBUTING.md), and
[device acceptance matrix](docs/DEVICE_TEST_MATRIX.md).

An additional [opt-in emulator validation workflow](docs/EMULATOR_TESTING.md)
exercises the actual desktop workflow against two disposable Android emulators.
It is separate from ordinary fake-tool tests. Emulator results cover the tested
Android/ADB path; they do not establish Motorola servicing or physical USB-driver
compatibility.

## Project documentation

| Document | Purpose |
| --- | --- |
| [Development guide](docs/DEVELOPMENT.md) | Toolchain, build/test commands, and daily workflow |
| [Architecture](docs/ARCHITECTURE.md) | Runtime boundaries and extension design |
| [Roadmap](docs/ROADMAP.md) | Ordered product work and acceptance criteria |
| [Engineering audit](docs/AUDIT.md) | Findings, evidence, fixes, and remaining work |
| [Device test matrix](docs/DEVICE_TEST_MATRIX.md) | Real device and Windows acceptance requirements |
| [Emulator validation](docs/EMULATOR_TESTING.md) | Opt-in disposable-emulator workflow and evidence limits |
| [Contributing](CONTRIBUTING.md) | Review and contribution conventions |
| [Security policy](SECURITY.md) | Project status and vulnerability reporting |
| [Privacy statement](PRIVACY.md) | Data handling, local ADB behavior, and report contents |
| [Disclaimer](DISCLAIMER.md) | Development status, authorized use, warranty and liability terms |
| [Copyright notice](COPYRIGHT.md) | Ownership scope and reserved rights |
| [License terms](LICENSE) | Current source-use terms and permissions |
| [Third-party notices](THIRD_PARTY_NOTICES.md) | Dependencies and required notices |
| [Licensing review](docs/LICENSING_REVIEW.md) | Rights evidence and decisions required before a license or release |
| [Attribution and history](docs/PROJECT_HISTORY.md) | Project origins and contributor credits |

## Project information

**Copyright © 2026 Michael Heald (Legatto2).** See the [copyright notice](COPYRIGHT.md)
and [license terms](LICENSE) for their scope and permissions. Review the
[privacy statement](PRIVACY.md) and [disclaimer](DISCLAIMER.md) before using a
development build.
