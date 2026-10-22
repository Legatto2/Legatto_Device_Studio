# Device and Windows acceptance matrix

This file defines hardware acceptance evidence. **No physical-device or packaged
Windows acceptance run has been recorded for the professional-foundation increment.**
Hardware-free Qt tests and CI builds do not change that status.

The separate [emulator integration record](EMULATOR_TESTING.md) tests genuine
ADB against disposable Android Virtual Devices. It provides real-tool evidence
for the tested emulator environment, while every physical-device and packaged
Windows result below remains independent. Passing emulator checks must not be
copied into hardware rows or presented as Motorola/MediaTek compatibility.

## Recording a run

Record source commit, artifact hash, OS version/architecture, compiler/Qt kit,
ADB version/hash, driver provider/version, device model, Android/build, connection
mode, scenario, expected/actual result, and evidence reference. Use a case ID or
session pseudonym. Do not commit serial numbers, IMEIs, customer names, passwords,
tokens, personal screenshots, or raw device logs.

An interrupted test is **Blocked**, **Inconclusive**, or **Failed**, as appropriate.
"The window opened" is not a pass for a complete inspection.

| Run ID | Source/artifact | Windows / Qt / compiler | ADB / driver | Model / Android build | Scenario | Result | Evidence |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Not recorded | — | — | — | — | — | Not run | — |

## Required diagnostics scenarios

| Scenario | Expected behavior | Hardware status |
| --- | --- | --- |
| Launch without Platform-Tools | Window opens; tool guidance appears; no device operation starts | Not run |
| Missing, invalid, or changed executable | Clear error; no PATH fallback or substitute executable | Not run |
| Path with spaces/non-ASCII characters | Correct direct native-executable invocation | Not run |
| No device | Empty discovery state; inspection unavailable | Not run |
| Debugging unauthorized | State visible; inspection blocked | Not run |
| Offline/recovery/sideload | State visible; unsupported operations unavailable | Not run |
| One authorized device | Explicit selection yields only that device's allowed properties | Not run |
| Two authorized devices | Every property command binds to the selected target | Not run |
| List reorder | Selection follows its identifier or clears; never switches silently | Not run |
| Selected device disconnects | Bounded failure/cancellation; no stale success report | Not run |
| Different device reconnects | New selection required; no misattributed old report | Not run |
| State changes during inspection | Clear failure/partial-state handling; no fabricated success | Not run |
| Cancellation | One terminal result; controls recover; no overlapping request | Not run |
| ADB stalls/nonzero exit | Bounded error; recoverable UI; no success indication | Not run |
| Existing Android Studio session | No automatic reset of the shared ADB server | Not run |
| Export | Correct schema; no raw serial/path/account secret/transcript | Not run |
| Repeated inspection | Fresh results, process cleanup, bounded memory | Not run |
| Window closes while busy | Predictable shutdown and subprocess cleanup | Not run |
| 100%, 150%, 200% scaling | Actions/status remain visible and usable | Not run |
| Keyboard/screen reader | Logical focus and identifiable controls/messages | Not run |
| Clean Windows machine | Package launches without development-kit assumptions | Not run |

Use deterministic fake processes for executable errors, malformed/fragmented
output, timeouts, output bounds, duplicate devices, identity changes, and argument
binding before physical-device checks. Fault injection on real phones is
unnecessary when a fake can establish the behavior.

## Historical model names are not support evidence

The inherited UI lists E6I, E7I, G20, E6s XT2053, G8 Power Lite XT2055,
E7 Play XT2095, E20 XT2155, and E40 XT2159. These are historical labels.
No support status is inferred from their presence.

A capability record identifies model **and** firmware/build, operation, tool/driver
versions, and passing evidence. Unknown combinations remain unknown.

## Future device-changing operations

Record the initial state and backup/recovery requirements. Revalidate target and
plan immediately before execution. Exercise failures with simulators first.
Verify final state independently from tool exit status: a successful process
exit alone is not proof that a device change took effect correctly.

Cancellation does not mean rollback. Do not automatically retry an uncertain
write/charge; the recovery procedure must determine what happened first.
