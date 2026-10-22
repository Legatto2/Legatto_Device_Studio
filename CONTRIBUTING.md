# Contributing to Legatto Device Studio

Start with a reproducible problem and a small, reviewable change. The development
target is `LegattoDeviceStudio`; the older `LegacyServiceClient` client is preserved for
review and must be explicitly enabled at build time. Its presence in the source
tree is not evidence that its servicing operations are safe to use.

## Development loop

1. Read [the architecture](docs/ARCHITECTURE.md), [development guide](docs/DEVELOPMENT.md),
   [security policy](SECURITY.md), and any instructions in the affected directory.
2. Create one focused issue or use an existing one. State the observed problem,
   acceptance criteria, scope, and expected validation. Avoid duplicate tracking.
3. Branch from the current reviewed base. Preserve unrelated changes and record
   the source revision used for comparisons.
4. Implement the smallest maintainable change. Reuse the diagnostics service;
   keep command execution out of UI event handlers. Use argument arrays, explicit
   device selection, bounded output, finite deadlines, and typed outcomes.
5. Build and run the applicable tests. Ordinary command-execution tests use the
   compiled fake-ADB fixture. Genuine SDK tools are permitted only in the
   separately enabled [disposable-emulator lane](docs/EMULATOR_TESTING.md), with
   its explicit opt-in and ownership guards; customer phones are never CI fixtures.
6. Inspect the diff for secrets, generated output, licensing, and unintended
   behavior. Update the relevant documentation when user behavior changes.
7. Open a pull request with exact validation results and remaining limitations.
   Merge only after owner review and required checks; releases are a separate
   decision with separate evidence.

## Common commands

With CMake 3.21+, Ninja, a C++17 compiler, Qt 5.15 development libraries, and Python:

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
```

For Qt 6 diagnostics, replace `linux-debug` with `linux-qt6`. The legacy client
requires a separate Qt 5 build. To compile it for inspection:

```sh
cmake --preset linux-debug -DLEGATTO_BUILD_LEGACY=ON
cmake --build --preset linux-debug
ctest --preset linux-debug
```

On Windows with Visual Studio 2022 x64 and a matching Qt 5 MSVC kit:

```powershell
cmake --preset windows-msvc -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
cmake --build --preset windows-msvc
ctest --preset windows-msvc
```

The Windows build/test presets select `Release`. Do not mix a MinGW Qt kit with
the MSVC generator. Keep personal SDK locations in the ignored
`CMakeUserPresets.json`, not in committed presets. The original qmake entry point
remains available for legacy compilation; it builds the legacy application.

## Review and test standards

Tests should prove observable behavior at the relevant boundary. For process
changes, cover launch failure, nonzero exit, crash, timeout, cancellation,
fragmented output, oversized output, repeated requests, and device ambiguity.
For parsing, include empty, malformed, contradictory, and unexpected data. Do not
write tests that simply reproduce the implementation or claim device support
from fake-process success.

CI runs Linux Qt 5, Linux Qt 6 diagnostics, and native Windows Qt 5 builds. It
compiles the legacy Qt 5 target but does not launch it, invoke its external tools,
or call its online services. Test evidence includes the source SHA and toolchain;
artifacts contain logs and test results, not a supported installer. Full Windows
device/driver behavior and physical-device results require their own evidence.

Keep actions pinned to reviewed commit SHAs. Dependabot proposes grouped action
updates weekly, capped at three open update PRs. Qt package versions, Python
installer dependencies, OS images, and compilers still need deliberate review;
an action pin alone does not make the entire build reproducible.

## Intended repository protection

The owner should verify these settings in a GitHub ruleset for `main`:

- Require pull requests and successful `Linux Qt 5`, `Linux Qt 6`, and
  `Windows Qt 5` checks for the reviewed revision.
- Block force pushes and branch deletion; require resolved review conversations.
- For a team with another qualified maintainer, require an independent code-owner
  approval and dismiss stale approvals after code changes. A sole owner cannot
  approve their own pull request; do not configure an impossible approval gate.
- Keep workflow tokens read-only by default and keep PR workflows off persistent
  self-hosted machines with device access or reusable credentials.

`CODEOWNERS` routes reviews; it does not enforce protection by itself. These are
documented expectations, not a claim that GitHub settings have been changed.

## Privacy, ownership, and releases

Never attach credentials, IMEI values, serial numbers, customer logs, firmware,
or third-party binaries to public issues. Use synthetic fixtures and review
report exports before sharing. Follow [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
for source and dependency provenance. No contributor may invent a license grant
for inherited code or redistribute external utilities based only on their
presence in a development workflow.

The owner has chosen rights reserved for new original contributions under
[LICENSE](LICENSE) and [COPYRIGHT.md](COPYRIGHT.md). A contribution does not
silently change that choice or transfer an unrelated author's rights. Confirm
the applicable permission for material before proposing its inclusion.

Publishing a release requires resolved distribution rights, verified dependencies,
the required Windows and device coverage, an accurate support matrix, and an
owner-approved release decision. A green compiler/test pipeline is necessary
evidence; it is not a substitute for those release requirements.
