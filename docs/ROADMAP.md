# Product and engineering roadmap

The goal is a reliable Motorola diagnostics and servicing workbench: clear device
identity, explainable actions, accurate outcomes, recoverable failures, and a
maintainable development process. More operations are useful when their
compatibility and effects are understood.

This roadmap is ordered by dependency, not invented release dates. "Implemented"
and "validated on hardware" are separate statuses.

## 1. Professional foundation

**Current development increment.**

Deliver a separate diagnostics application, restricted asynchronous ADB service,
explicit tool/device selection, limited report export, CMake/CTest, Windows/Linux
CI, and accurate contributor/security/product documentation. Preserve the earlier
containment branch and the optional legacy build.

Acceptance:

- Diagnostics builds without legacy networking, serial, or account code.
- Opening its window does not contact an account service or run a device-changing command.
- Device commands name the selected serial; stale or unsupported targets fail
  without selecting a substitute.
- Startup failure, nonzero exit, fragmented output, timeout, cancellation, excess
  output, and repeated operations have hardware-free test coverage.
- Failed/cancelled requests cannot produce a successful fresh inspection report;
  exports exclude raw serial/path/transcript fields.
- Windows/Linux build and test outcomes are tied to the reviewed commit.

Exit evidence: reviewed pull request, applicable CI, test logs, and explicit
remaining limitations. Device support or a distributable release is not implied.

## 2. Technician-ready diagnostics

- Guided installation and connection troubleshooting with official tool/driver links.
- Distinct missing-tool, unauthorized, offline, disconnected, and unsupported states.
- A tested inventory for model, Android/build, and justified read-only health data.
  Add properties only when their meaning and privacy impact are documented.
- Report preview and controlled export suitable for a support case.
- DPI, keyboard, contrast, long-label, and screen-reader validation on Windows.
- Clear detection of tool changes and a reviewed tool version policy.

Acceptance: physical-device results in [DEVICE_TEST_MATRIX.md](DEVICE_TEST_MATRIX.md),
including multiple devices, disconnects/reconnects, cancellation, and export review.
Normal diagnostics must not require changing system drivers or elevation.

Exit evidence: a documented Windows environment and representative model/firmware
rows; defects closed or disclosed. Do not claim all Motorola or all MediaTek
devices are supported.

## 3. Validated service operations

Add one narrowly scoped operation at a time, based on an operator need and a
documented supported interface. Rebooting into a supported mode or retrieving an
approved backup can be evaluated before higher-impact operations. An old UI button
is not sufficient justification for enabling a feature.

Each operation requires:

- An explicit target and compatible device/firmware profile.
- A visible plan describing effects, inputs, prerequisites, and expected results.
- Necessary operator authorization and operation-specific confirmation for data changes.
- Provenance and distribution rights for every helper/data file.
- Exclusive ownership of the selected device while running.
- A deadline and cancellation policy describing what can and cannot stop.
- Result verification and a documented recovery path.
- Negative tests and physical-device evidence for the intended combination.

Legacy FRP, security-lock, identifier-changing, carrier, and META workflows are
not brought forward by default. Their old command strings and services are not
a supported API. A replacement needs separate requirements, rights, compatibility,
and safety review.

Exit evidence: operation-specific tests, device rows, review, and user documentation.
Do not combine unvalidated operations into a "full support" claim.

## 4. Dependable installation and releases

- Establish source/artwork licensing and retain attribution.
- Choose a Windows compiler/Qt kit and document runtime contents.
- Package the proper Qt libraries/plugins and third-party notices.
- Generate a manifest with source revision, dependencies, hashes, and test evidence.
- Test first launch and uninstall/cleanup in a clean Windows environment.
- Make a signing decision and document download verification.
- Provide upgrade notes and a rollback policy that does not re-enable retired paths.

Acceptance: a clean machine runs the actual package using documented steps;
dependencies/rights are recorded; published features match verified behavior.
Rebuilding source locally is not the release acceptance test.

No paid service is required by the development design. Distribution rights,
runner allowances, and signing costs must be evaluated for the chosen channel;
do not advertise an unverified cost or licensing entitlement.

## 5. Scale after reliability is established

- Reusable device/profile models instead of eight copied tabs.
- Per-device queues with ownership and no conflicting operations.
- Structured local operation history with limited retention.
- Optional support-case references and report bundles.
- Evidence-backed capability profiles and troubleshooting guidance.
- Measured discovery/inspection performance and a bounded memory budget.

Acceptance: concurrent jobs, disconnects, cancellation, retention, and cleanup
pass repeatable tests. Features should reduce operator effort or improve verified
outcomes; cloud services, AI layers, and plugin complexity are not prerequisites.

## Working backlog

Keep a small number of active workstreams. Use one issue per concrete outcome
and one reviewable PR per increment. Link defects to audit IDs and proving tests.
Do not generate speculative issues merely to make the project look busy.

| Workstream | Immediate next evidence | Depends on |
| --- | --- | --- |
| Foundation | Reviewed code and Windows/Linux CI | Current increment |
| Device validation | Real diagnostics matrix results | Foundation |
| Rights/dependencies | Provenance and permission decisions | Owner/provider evidence |
| First service operation | Requirements, adapter, recovery, hardware results | Foundation and device validation |
| Release package | Clean-machine test and manifest | Rights and supported capabilities |

See [CONTRIBUTING.md](../CONTRIBUTING.md) and [DEVELOPMENT.md](DEVELOPMENT.md)
for the day-to-day workflow.
