# MotoTool_MTK engineering audit

Assessment date: **2026-10-22**. This report evaluates the repository and the
professional-foundation development increment. It is not a hardware certification.

## Executive assessment

MotoTool_MTK contains a useful starting point for a Motorola service application,
but its legacy implementation is not dependable enough for customer-device
operations. Device selection, subprocess execution, account requests, progress,
and completion reporting are tightly coupled. Several apparently routine actions
can enter a device-changing path, and success messages do not consistently follow
verified outcomes.

The recommended direction is an incremental replacement: establish an independently
testable diagnostics application, then admit additional operations only after their
execution, recovery, provenance, and device compatibility have been demonstrated.
Keep the inherited client available for engineering comparison. Its presence is
not a supported servicing capability of the new application.

The new default target is `MotoTool_Diagnostics`, version `0.1.0-dev`. The original
`MotoTool_MTK` client retains its `0.0.3` application version and is an optional
legacy build. This separation permits useful product development without silently
inheriting the old client's operational assumptions.

## Scope and evidence

| Item | Inspected state |
| --- | --- |
| Default branch before this work | `main` at `2abae2fe5360e4ecb73db0d5d4549001f8e5d2a3` |
| Prior, unmerged hardening | `hardening/phase-1-containment` at `1a0007bb2df7b56082a85269e3f236367f2290cf` |
| Development base | Existing hardening branch; its changes are preserved |
| Open work at initial inspection | No open issues or pull requests; three branches |
| Published releases at initial inspection | None |
| Original client | Qt Widgets/C++, Windows-oriented helpers, external services |
| Prior hardening source inventory | 34 tracked paths; 2,110-line main window, 1,685-line backend, 796-line META worker, and 3,141-line UI definition |
| UI inspection | 217 widgets, 28 layouts; fixed-size frameless main window |
| Source licensing evidence | No repository license file found in the inspected trees or license-file history |

Methods: source and Git history inspection; review of earlier hardening changes;
available GitHub workflow metadata; Qt/Android primary documentation; and
hardware-free build/test work. External service contracts, third-party tool
internals, device firmware, drivers, and billing behavior were not validated.
No legacy credentials or private device contents are reproduced here.

### Verification evidence

The original CodeQL Setup run at `80e245e` succeeded. The two visible Qt hardening
build/test runs at `ea2a4b0` and `453d4e0` failed. A subsequent workflow that applied
the reviewed containment changes succeeded, producing `1a0007b`; that application
workflow is not a successful build/test result for the resulting revision.

- [Historical Qt baseline run 38138740782](https://github.com/Legatto2/MotoTool_MTK/actions/runs/38138740782)
- [Historical Qt hardening run 38139822822](https://github.com/Legatto2/MotoTool_MTK/actions/runs/38139822822)

During this audit the inherited application compiled locally with **Qt 5.15.13,
GCC 13.3, and `-Werror=return-type`**; other compiler warnings remain. Its Qt helper
suite reported **36 passes, zero failures/skips** including test lifecycle/data
rows, and the Python source-guard suite passed **10 tests**. The legacy application
was not launched. These are local source/helper checks, not Windows, device,
external-service, billing, or release-package tests.

The integrated professional-foundation source was also compiled and tested
locally with GCC 13.3, CMake 3.28.3, and Ninja. The Qt 5 configuration included the
preserved legacy target; the Qt 6 configuration built the new diagnostics target.
Both configurations passed all four CTest suites:

| Suite | Qt 5.15.13 | Qt 6.4.2 |
| --- | --- | --- |
| Existing security helpers | 36 passed | 36 passed |
| Diagnostics process/service cases | 43 passed | 43 passed |
| Diagnostics UI cases | 8 passed | 8 passed |
| Python source regression guards | 10 passed | 10 passed |

Qt counts include lifecycle and data rows. No case failed or was skipped in these
Linux runs. Tests use a compiled synthetic ADB fixture and temporary files; they
do not launch a real ADB server, contact account services, or operate phones.
The actual diagnostics window was additionally rendered and visually inspected
at 1120×820 and 760×600; the narrow layout stacks its panels and scrolls vertically.

Native Windows outcomes belong to the pull request's exact commit and associated
GitHub Actions evidence. A configured Windows job is not a Windows pass until it
has run. The inherited link/ACL helper test explicitly skips Windows; its separate
native acceptance work remains in the device test matrix. Physical-device,
driver, external-service, billing, and production-package checks remain unrun.

## Prioritized findings

Priorities are engineering release priorities, not CVSS scores. **P0** blocks
customer-device use of the affected behavior; **P1** must be resolved before that
workflow is supported; **P2** concerns maintainability and operator usability.
References are pinned to the inherited hardening revision so later edits do not
invalidate the evidence.

| ID | Priority | Finding and consequence | Evidence | Disposition |
| --- | --- | --- | --- | --- |
| MT-01 | P0 | The old information workflow can reach a power-off operation when its port option and model conditions match. Its label does not describe the complete effect. | [Information dispatch](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.cpp#L764-L774), [reboot handler](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.cpp#L339-L352) | New diagnostics has its own restricted command set; the old information path is not reused. |
| MT-02 | P0 | Legacy execution constructs shell commands from mutable data and depends on third-party tools with unestablished provenance. Command construction also contains embedded third-party authentication material. | [Backend implementation](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.cpp), [META implementation](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp) | Keep outside the new runtime. Replace operation by operation; establish rights and revoke any real exposed credentials through their provider. No secret values or exploitation instructions are included here. |
| MT-03 | P0 | Legacy success/progress can be emitted before checking process termination or reading resulting device state. An operator can mistake an attempted step for a completed repair. | [META verification/completion](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp#L427-L454) | New diagnostics distinguishes failed, cancelled, timed-out, and completed requests. Future write operations need independent verification. |
| MT-04 | P1 | Process startup is inside `qDebug()` stream expressions. Suppressing debug output can suppress required side effects. | [META startup](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp#L145-L159), [build definitions](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/MotoTool_MTK.pro) | Legacy defect remains tracked. Do not restore sensitive debug output to make the code execute. New execution does not depend on logging expressions. |
| MT-05 | P1 | META treats stdout deliveries as complete responses, indexes assumed fields, stops its watchdog on stderr, and closes stdin before later protocol writes. Fragmentation/errors can break its state machine. | [Output handling](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp#L179-L200), [stdin lifetime](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp#L417-L433) | Replace with a framed protocol adapter and explicit terminal results before reuse. New diagnostics buffers bounded output and checks completion. |
| MT-06 | P1 | Legacy ADB commands lack a selected-device binding; eight UI tabs do not establish exclusive device ownership. COM names can be reused after reconnect. | [ADB discovery](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.cpp#L1372-L1429), [selection checks](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L1082-L1095) | New diagnostics requires explicit selection and serial-bound requests after a fresh snapshot. Future concurrency needs stronger identity and per-device ownership. |
| MT-07 | P1 | Login and META service requests use nested event loops without consistent deadlines, typed errors, or cancellation. Retrying is ambiguous after a possible charge. | [Login sequence](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L117-L230), [META services](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/metamodethread.cpp#L573-L786) | Diagnostics has no account-service dependency. Future adapters need deadlines, validated schemas, reconciliation, and idempotency. |
| MT-08 | P1 | Account/operation globals are shared across workers. Money and operation IDs use floating-point values; the inherited parser bounds only one entry point. | [Shared state](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/globalvars.h#L96-L151), [transitional parser](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/securityhelpers.cpp#L115-L137) | Diagnostics owns separate per-operation state. Future billing needs opaque IDs, fixed-point amounts, and server-authoritative reconciliation. |
| MT-09 | P1 | Local logs retain raw UI output in a working-directory file. Disabling uploads/debug streams does not redact old or current logs. | [Local logger](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/class_logfile.cpp#L3-L53) | New reports use an explicit schema without raw serial/path/transcript fields. Legacy retention remains separate. |
| MT-10 | P1 | Helper binaries, model data, service implementations, and driver packages are absent. Authenticity, compatibility, redistribution rights, and update policy are unestablished. | [Expected paths](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.h#L48-L75), [source tree](https://github.com/Legatto2/MotoTool_MTK/tree/1a0007bb2df7b56082a85269e3f236367f2290cf) | Diagnostics uses an operator-selected Platform-Tools installation. A hash identifies a file, not its publisher. See [notices](../THIRD_PARTY_NOTICES.md). |
| MT-11 | P1 | Drop-down model names and a successful compile do not prove supported device/firmware combinations. No reproducible hardware acceptance record exists. | [Legacy UI](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.ui), [earlier boundaries](HARDENING.md) | Add evidence to [DEVICE_TEST_MATRIX.md](DEVICE_TEST_MATRIX.md). No hardware-validated support is claimed. |
| MT-12 | P2 | Eight duplicated workflows couple UI slots to the active tab. Fixed sizes, a custom title bar, and repeated state changes impede maintenance and accessibility. | [Window setup](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L9-L45), [worker setup](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L977-L1048), [tab coupling](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L1295-L1357) | New resizable diagnostics UI has one device-selection model. Future concurrency should use jobs rather than copied tabs. |
| MT-13 | P2 | Read-only discovery requires an IMEI/model selection and elevated PowerShell. This obscures connection problems and adds unrelated steps. | [Button](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L346-L362), [validation](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L1053-L1080), [elevation](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/backendclass.cpp#L1399-L1429) | Diagnostics separates tool setup, connection state, and inspection, using direct execution. |
| MT-14 | P1 | Release engineering is incomplete: no package, established native Windows evidence, accurate prior security policy, or valid checked-in resource metadata. | [Original resource file](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/MotoTool_MTK_resource.rc), [old policy](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/SECURITY.md) | This increment adds CI and accurate documentation. Packaging, signing decisions, rights, clean-machine validation, and device evidence remain release gates. |
| MT-15 | P1 | Worker shutdown is attached to custom close buttons; the destructor only deletes UI. Thread waits have no timeout, and ordinary window-close handling is not unified with cancellation. | [Destructor](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L78-L81), [custom shutdown](https://github.com/Legatto2/MotoTool_MTK/blob/1a0007bb2df7b56082a85269e3f236367f2290cf/mainwindow.cpp#L261-L304) | Keep the legacy lifetime separate; new UI/service shutdown needs explicit cancellation/cleanup tests. |

## Inherited, added, and deferred

### Preserved from the previous hardening branch

- Username-only preferences and migration away from actively stored plaintext passwords.
- Correct encoding of individual login form values.
- Disabled FRP UI/backend paths and removal of the unauthenticated downloaded-command path.
- Disabled automatic remote log uploads and suppressed legacy debug streams.
- A bounded, typed ADB charge-response helper.
- Serial-port snapshot refresh and bounds checking.
- Initial Qt helper tests, source guards, and a Linux build workflow.

These are valuable containment changes; they do not validate the remaining legacy
workflows. Logging suppression exposes MT-04: containment code being present is
not proof that every inherited operation still works.

MT-04 was additionally confirmed against the installed Qt 5 logging macro with a
harmless compiled counter probe: the side effect inside the disabled debug stream
was not executed. No legacy device process was launched for that check.

### Professional-foundation increment

- A separate diagnostics application and restricted ADB service.
- Explicit tool setup/device selection without an account-service dependency.
- Asynchronous execution with bounded time/output, cancellation, and typed results.
- Read-only property collection and a limited export schema.
- CMake/CTest and native Windows/Linux build coverage.
- Contributor workflow, dependency updates, issue/PR templates, accurate security
  policy, architecture, roadmap, and device-validation records.

Current outcomes must be associated with actual test/CI evidence. This report does
not turn unexecuted checks into passes or claim that the isolated legacy findings
have been fixed inside the legacy application.

### Remaining release work

1. Establish source, artwork, tool, and redistribution rights.
2. Complete native Windows clean-machine testing of the actual package.
3. Record representative physical-device and Android-build diagnostics results.
4. Validate disconnects, timeouts, multiple devices, cancellation, privacy, and
   high-DPI accessibility on Windows.
5. Produce a traceable package with dependency notices, hashes, and a signing
   decision. Do not bundle unidentified legacy binaries.
6. Add operation-specific preflight, preview, recovery, verification, and
   device/firmware coverage before enabling future device-changing capabilities.

## Product direction

The useful differentiator is a technician trusting which phone was selected,
what will run, why it stopped, whether it completed, and what evidence can be
shared safely. Prioritize connection troubleshooting, capability discovery,
validated device profiles, operation history, and dependable installation.
Adding more buttons before these foundations increases support cost.

The five workstreams in [ROADMAP.md](ROADMAP.md) have acceptance criteria rather
than invented dates. [ARCHITECTURE.md](ARCHITECTURE.md) defines boundaries for
future changes.

## Primary references

- [Qt QProcess](https://doc.qt.io/qt-6/qprocess.html): executable paths, arguments,
  asynchronous signals, and blocking behavior.
- [Android Debug Bridge](https://developer.android.com/tools/adb): discovery,
  connection states, and explicit target selection.
- [Android Platform-Tools](https://developer.android.com/tools/releases/platform-tools):
  official distribution and change history.
- [Qt licensing](https://doc.qt.io/qt-6/licensing.html): module and third-party terms.
- [GitHub repository licensing](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository):
  public visibility and source licensing are distinct.

These sources inform the design; they do not certify this project's devices,
services, source rights, or release readiness.
