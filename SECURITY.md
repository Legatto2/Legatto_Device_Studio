# Security policy

## Project status

Legatto Device Studio, built as `LegattoDeviceStudio`, is a development
application in this repository; the inherited `LegacyServiceClient` client is
retained for engineering review. There is no supported production release or verified customer-device
compatibility promise. The previous template version table did not describe
this project and has been removed.

Diagnostics is limited to explicit, read-only ADB inspection. The optional legacy
client still has execution, device-safety, account, privacy, and dependency risks
described in [the audit](docs/AUDIT.md). Compiling it does not make those workflows
supported.

## Reporting

If **Security → Report a vulnerability** is available on the repository, use it
to submit a private report. Availability must be checked on GitHub; this file
does not claim private reporting has been enabled.

If no private channel is available, open a minimal issue asking the maintainer
to establish one. Do not include credentials, customer/device identifiers,
sensitive logs, executable exploit details, or private data in a public issue.
No private email address or guaranteed response time is asserted until published
by the maintainer.

A useful private report gives the affected revision, application target,
platform/tool versions, expected and observed behavior, impact, and a minimal
reproduction with synthetic data where possible. Distinguish source findings
from physical-device or service reproductions.

## Development boundaries

- Put new commands in the restricted service with explicit target selection,
  separate arguments, bounded execution, and checked results.
- Treat device/tool output as untrusted. Validate structure/size before use in
  UI, reports, filenames, or subsequent commands.
- Do not commit account secrets, provider credentials, or customer transcripts
  to code, tests, issues, or workflow artifacts.
- A computed executable hash is not proof of publisher authenticity.
- Preserve separation between diagnostics and the unvalidated legacy client.
- Future device-changing operations need specific preflight, authorization,
  verification, and recovery behavior.

## Historical data and credentials

Earlier hardening removed active plaintext-password storage from preferences and
disabled automatic remote log uploads. It did not erase old files, backups, logs,
history, or provider records. Handle real exposed credentials with their issuing
provider; changing source or suppressing debug output is not credential rotation.

See [third-party notices](THIRD_PARTY_NOTICES.md) for provenance gaps and the
[device test matrix](docs/DEVICE_TEST_MATRIX.md) for validation boundaries.
The [privacy notice](PRIVACY.md) describes the diagnostics data flow and export
limits; the [development disclaimer](DISCLAIMER.md) records operational and
liability limitations without replacing mandatory rights.
