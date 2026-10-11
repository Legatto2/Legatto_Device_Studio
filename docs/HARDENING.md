# Hardening status: increment 1A

Base: `2abae2fe5360e4ecb73db0d5d4549001f8e5d2a3` (`main`, 2026-10-11).
This preserves the security-policy commit made after the initial audit.

## Implemented in this increment

1. Remove password persistence with username-only whitelist serialization and
   atomic `QSaveFile` migration. Reject linked, oversized, or malformed JSON
   settings rather than claiming successful cleanup. Never create a plaintext
   credential backup.
2. Encode each login form value with `QUrl::toPercentEncoding`.
3. Block FRP at the button, backend entry, and timer dispatch; remove its HTTP
   download and shell payload construction. No replacement bypass is implemented.
4. Suppress legacy debug streams in every application configuration and stop
   automatic server log uploads.
5. Repair the missing-return charge handler. Validate transport, HTTP status,
   explicit boolean error fields, operation ID and credit; abort after 15 seconds.
6. Rebuild serial-port selections using device identity and validate indexes.
7. Add read-only CI compilation and hardware-free Qt/Python regression tests.

The baseline CI build reproduced `control reaches end of non-void function` in
`BackEndClass::sendOperationDetailsToServerToReduceCredit()` with
`-Werror=return-type`. Current pass/fail evidence belongs to the specific CI run
and commit, not to an assumed local build.

## Remaining release blockers

- **External tooling and embedded credentials:** establish provenance and rights;
  remove remaining hardcoded third-party authentication material from command
  construction. Any real exposed credentials controlled by the operator require
  rotation with their provider. Suppressed debug output is not credential rotation
  and does not remove source-history exposure.
- **Process runner:** replace command-shell interpolation with validated absolute
  executables and separate arguments. Eliminate unnecessary elevation. Bound
  process lifetimes and handle exit status, cancellation and fragmented output.
- **Device safety:** bind ADB commands to the selected device, enforce exclusive
  per-device operation ownership, confirm destructive actions, and verify results
  by read-back. Serial COM names can be reused after reconnect; the new list
  handling is not a complete device identity guarantee.
- **META state machine:** repair stdin channel lifetime, error propagation,
  watchdog behavior and optimistic success reporting before enabling release use.
- **Operation/account model:** replace shared mutable globals with per-operation
  contexts and synchronized account state. IDs should become opaque strings or
  appropriate integers; money should use fixed-point units. The new ADB parser
  rejects IDs above 16,777,216 as a temporary safety boundary for the old float
  field. This may reject valid newer server IDs and does not fix the META paths.
- **Network architecture:** migrate remaining login/META calls away from nested
  event loops, add explicit deadlines, typed results and cancellation, and verify
  the live service contract. A timeout after a charge request does not prove the
  server did not debit credit; never blindly retry such requests.
- **Logging/privacy:** replace temporary global debug suppression with a redacted
  structured logger. Define consent, retention, upload policy and local protection.
  Old plaintext settings in backups and existing logs require separate handling.
- **Release engineering:** validate a native Windows kit and runtime, document
  drivers and binary hashes, complete packaging/signing and hardware/firmware
  coverage, and replace the security-policy template with accurate support data.

## What the tests do not prove

The application is compiled but not launched by CI. Qt tests instantiate only
helper widgets/files using synthetic values. They do not log in, debit credit,
invoke servicing binaries, access USB/serial devices, or verify device changes.
A green result is not approval for client-device use or release distribution.

## Review and rollback

Review the pull request before merging. Do not auto-merge or publish a release
from this increment. Preserve the base commit and review subsequent changes
against the tested revision. Reverting this increment restores insecure behavior;
do not use rollback as a way to re-enable the retired FRP workflow on live devices.
