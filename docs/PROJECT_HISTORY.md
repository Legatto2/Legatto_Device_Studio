# Project history and archived client

Legatto Device Studio is the current diagnostics product, maintained by
**Michael Heald (Legatto2)**. Copyright for his new original contributions is
recorded in [COPYRIGHT.md](../COPYRIGHT.md), with rights reserved under the
project's [LICENSE](../LICENSE) notice.

The repository also preserves an earlier project and its history. Those
materials have separate authorship, dependencies, and operational risks. This
page keeps that background distinct from the current product's feature claims.

## Original project and credits

The inherited project was named **MotoTool_MTK**. It was a Qt 5/C++ desktop
servicing client with Windows-oriented helper tools, eight repeated device
workspaces, and external account/service integrations.

The recorded Git history includes:

| Historical record | Recorded attribution |
| --- | --- |
| [`3a414c3`](https://github.com/Legatto2/Legatto_Device_Studio/commit/3a414c3), initial source import | `ABC` |
| [`a114e87`](https://github.com/Legatto2/Legatto_Device_Studio/commit/a114e87), earlier progress commit | `x` |
| [`80e245e`](https://github.com/Legatto2/Legatto_Device_Studio/commit/80e245e0d0f2ed96f37713138887711e25e65762), later original-project progress | **Mubashir Iqbal**, [Mubshr07](https://github.com/Mubshr07) |
| Subsequent maintenance, containment, diagnostics replacement, and development pipeline | **Michael Heald (Legatto2)** and the recorded repository contributors |

These are attribution records, not a complete determination of ownership or a
license assignment. Renaming the repository or retaining an old file does not
make inherited work newly authored by the current maintainer. Original credits
and Git history are preserved.

No repository-wide reuse license for the inherited source/assets was found in
the inspected baseline trees or available license-file history. The current
rights-reserved notice covers new original contributions and does not relicense
that inherited material. [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) and
[LICENSING_REVIEW.md](LICENSING_REVIEW.md) record the separate permissions and
dependency questions.

## Development milestones

| Revision or increment | What it established |
| --- | --- |
| Original project history through `80e245e` | The inherited Qt client, artwork, helper-tool references, and service workflows |
| `2abae2f` | Repository state before the professional-foundation review; a security-policy change had been merged |
| `1a0007b` containment branch | Prior credential/preferences, remote-log-upload, charge-response, port-selection, and FRP containment work, preserved in later development |
| `0dab85b`, merged into main at `4c7741b` | A separate read-only diagnostics service/window, deterministic fake-tool tests, CMake/CTest, Windows/Linux CI, and engineering documentation |
| Emulator and identity increment | Legatto Device Studio branding throughout the current product/repository; separate disposable-emulator validation; privacy, copyright, and disclaimer notices |

The current repository is
[`Legatto2/Legatto_Device_Studio`](https://github.com/Legatto2/Legatto_Device_Studio).
Historical source links use the current repository URL with a pinned revision;
filenames inside those revisions retain their original spelling.

### Existing report compatibility

New reports identify `application` as `Legatto Device Studio`. Earlier reports
used the historical value `MotoTool_MTK`. The schema remains version 1 because
the report's fields and types have not changed. Consumers that compared that
metadata to one fixed product name need to accept the new value for new reports;
do not rewrite historical report metadata or treat the name change as new device
evidence.

The [engineering audit](AUDIT.md) records the original review scope, pinned
findings, inherited containment, implemented improvements, and validation limits.
Do not interpret a historical build result as evidence that the archived service
operations are safe or supported today.

## Current archive names and boundary

| Item | Current name or location |
| --- | --- |
| Default diagnostics application | `LegattoDeviceStudio`, source in `diagnostics/` |
| Opt-in archived application target | `LegacyServiceClient` |
| CMake option | `LEGATTO_BUILD_LEGACY=ON`, using Qt 5 |
| Archived qmake project | [`LegacyServiceClient.pro`](../LegacyServiceClient.pro) |
| Retained legacy resource metadata | [`LegacyServiceClient_resource.rc`](../LegacyServiceClient_resource.rc) |
| Archived implementation | Root-level `mainwindow`, `backendclass`, `metamodethread`, `globalvars`, and supporting files |
| Inherited resources | `resources.qrc`, `Incrypt.qss`, and the original images under the root and `pics/` |

The archived application retains the historical `0.0.3` version for engineering
reference. It is excluded from the default build. Renaming the target/files is
organizational housekeeping; it does not repair, validate, or endorse its
servicing behavior.

Legatto Device Studio does not load the archived client's live account state,
remote-service implementation, command dispatcher, or device-operation globals.
The default Windows executable no longer embeds its inherited icon. The new
application's limited read-only feature set is documented in the main README;
the archive's old buttons are not a supported product capability list.

## Why the archived client remains restricted

The audit found substantive problems with action effects, shell construction,
process output parsing, completion claims, watchdog/error handling, account
requests, mutable state, and shutdown. External helper/data provenance and model
compatibility were not established. Earlier containment reduces some exposure
but does not resolve all those findings.

Keep account services, billing, firmware changes, security-lock workflows, and
other unvalidated service actions outside the new runtime. Do not launch the
archived client in ordinary CI or use a successful compilation as authorization
to run it against a customer device. A future operation requires its own scoped
design, permissions, trusted dependencies, recovery behavior, and physical-device
acceptance evidence.

Use [AUDIT.md](AUDIT.md) for exact findings,
[HARDENING.md](HARDENING.md) for the inherited containment notes,
[ARCHITECTURE.md](ARCHITECTURE.md) for the current runtime boundaries, and
[DEVICE_TEST_MATRIX.md](DEVICE_TEST_MATRIX.md) for actual hardware acceptance.
