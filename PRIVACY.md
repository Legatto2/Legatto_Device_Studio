# Legatto Device Studio privacy notice

**Development build: `0.1.0-dev` · Reviewed: 22 October 2026**

Legatto Device Studio processes diagnostic information on the computer where you
run it. The current application has no analytics, advertising, account login,
automatic report upload, crash-report upload, or automatic update-check feature.
You choose whether to inspect a device and whether to save a report.

This notice covers the new diagnostics application, currently built as
`LegattoDeviceStudio`. It does **not** describe the separate inherited
`LegacyServiceClient` servicing client, whose account, logging, and device-operation
risks are documented in [the audit](docs/AUDIT.md).

## Maintainer and contact status

The maintainer of the new original work is **Michael Heald (Legatto2)**. No
separate publishing entity, dedicated privacy contact, or customer-support data
service has been designated in this development release. Do not send private
device information to a public GitHub issue. For a security concern, follow the
private-channel guidance in [SECURITY.md](SECURITY.md).

Before offering a hosted support service or collecting customer information,
the publisher must publish its contact details and the applicable processing,
retention, and rights-request arrangements. This development notice does not
invent those arrangements or claim universal regulatory compliance.

## Information used by the application

| Information | Why it is used | Where it is used |
| --- | --- | --- |
| Selected ADB executable's local path, SHA-256, and parsed version | Identify the exact tool selected and detect changes | Application memory and tool details; hash/version are included in an exported report, the path is not |
| ADB device serial, authorization/connection state, model, product, and transport identifier | Display discovery results and revalidate the explicitly selected connection | Application memory, device table, and ADB communication; the raw serial is also passed in the client process arguments |
| Manufacturer, model, Android version, build ID, and reported security-patch date | Present the requested read-only inspection | Application memory, device details, report preview, and an export you choose to save |
| Application/schema version, UTC report time, and a session device pseudonym | Explain the report format and relate results within one application service session | Report preview and an export you choose to save |
| ADB process output and completion/error state | Parse responses and decide whether an operation succeeded | Bounded processing in memory; raw command transcripts are not included in the report or deliberately written to an application log |

The implementation is in [the diagnostics service](diagnostics/diagnosticscore.cpp)
and [window/export code](diagnostics/diagnosticswindow.cpp); the
[architecture guide](docs/ARCHITECTURE.md) explains their separation.

Discovery lists transports already known to the local ADB server. It can display
devices other than the one you intend to inspect; explicit selection is required
before property queries. A device serial may itself contain a network address
when the transport is TCP or wireless.

The inspection command allowlist does not request IMEIs, phone numbers, contacts,
messages, photographs, account passwords, authentication tokens, or a copy of
the device filesystem. Device-provided property strings can nevertheless contain
unexpected or identifying information. Review the visible report values before
sharing them.

## Connections and external tools

The application launches the native ADB executable you select. Its client
commands use the local server endpoint `127.0.0.1:5037`; that server communicates
with connected Android devices or emulators. An existing server can have USB,
TCP, or wireless connections. This is not a promise that no network traffic
occurs. The application does not initiate pairing through a user-facing feature,
but it can use a transport already known to the server. The
[Android ADB documentation](https://developer.android.com/tools/adb) describes
the client, server, device daemon, and connection modes.

Raw target serials are used in ADB arguments and protocol handling. Other
software with sufficient access to the computer, the ADB server, or the device
may observe them. They do not stay only inside the application window.

The official Platform-Tools link and the project-repository link in About open
their websites in your default browser only when you choose them. That browser,
those websites, the SDK distribution, and any independently configured device
connection operate under their own settings and policies.
The application does not control their history, logs, keys, or retention.

## Report contents and limits

The JSON export contains these fields:

- `schemaVersion`, `application`, `applicationVersion`, and `generatedAt`.
- `deviceId`, a pseudonym created from the serial and a randomly generated
  per-service salt, and `transport` with the value `adb`.
- `tool.version` and `tool.sha256`.
- `device.manufacturer`, `device.model`, `device.androidVersion`,
  `device.buildId`, and `device.securityPatch`. An unavailable allowed value is
  represented as `null`.

The schema has no raw serial, transport identifier, local tool path, account,
or raw command-output field. The same device has a stable pseudonym during one
service instance; a new instance uses a fresh salt. This reduces direct exposure
of the serial but does not make the report anonymous. A person with contextual
information may still recognize the device or connect reports through their
contents. It is not encryption or a guarantee of legal anonymisation. See the
[ICO's explanation of pseudonymisation](https://ico.org.uk/for-organisations/uk-gdpr-guidance-and-resources/data-sharing/anonymisation/pseudonymisation/)
for the distinction.

## Saving, retention, and deletion

The diagnostics code does not implement a persistent device database,
credentials store, inspection history, or background diagnostic log. Current
results are kept in application memory and displayed until replaced, cleared,
or the window closes. Closing the application is not a secure-memory-erasure
operation.

**Exported reports are ordinary, unencrypted JSON files.** They are written only
to the destination you choose, with the preview's contents. They remain there
until you delete or move them; the application has no automatic expiry or remote
deletion mechanism. A destination inside a shared or synchronized folder can
make the report available to other people or services under that folder's
settings. The application does not manage those copies or backups.

The operating system, Qt file dialogs, ADB, your browser, and other host tools may
retain their own file history, settings, authentication state, process metadata,
logs, backups, or crash/swap data. This application does not purge those records.
Uninstalling or closing it does not revoke Android debugging authorizations or
stop a shared ADB server. Manage those permissions using the device and SDK's
documented controls.

You can avoid an export, cancel an operation, close the window, and delete
reports you control. Cancellation prevents a fresh successful report for the
cancelled operation; it does not retract an earlier file or a copy already
shared with someone else.

## Sharing, support, and development evidence

The application does not send a diagnostic report to the maintainer. You decide
whether to share one outside the application. Review its contents, choose an
appropriate recipient/channel, and avoid public posts containing customer data.
A technician or organisation using the application remains responsible for its
own authorization, privacy notice, access controls, retention, and onward
sharing arrangements.

Ordinary automated tests use synthetic fixtures. The separately enabled
[emulator lane](docs/EMULATOR_TESTING.md) uses only its verified disposable AVDs;
its CI artifacts are sanitized development evidence with the retention specified
in that guide. This does not authorize collecting customer-phone logs for CI.
Information posted directly to GitHub is handled by GitHub and can be public;
see the [GitHub privacy statement](https://docs.github.com/en/site-policy/privacy-policies/github-general-privacy-statement).

## Changes to this notice

Update this notice alongside any change that introduces new report fields,
persistent storage, network destinations, telemetry, accounts, support uploads,
or automatic updates. Explain the changed behavior before enabling it. The
review date and source history identify this notice's scope; it is not a promise
about unreviewed forks, external tools, or the legacy application.
