# Legatto Device Studio identity

**Product name:** Legatto Device Studio

**Description:** Read-only Android diagnostics

**Maintainer of new original work:** Michael Heald (Legatto2)

The current product helps an operator choose a trusted local ADB executable,
explicitly select an authorized Android device, read a small set of device
properties, and review or export a concise JSON report. Its name and public copy
should describe that behavior. Do not advertise the inherited client's unlocking
or servicing claims as features of this application.

## Repository About

The GitHub About field was updated on 22 October 2026 to:

> Legatto Device Studio: a Qt desktop tool for read-only Android device diagnostics, explicit device selection, and privacy-conscious JSON reports.

[Screenshot of the renamed repository and saved About field](images/github-about-renamed-20261022.jpg)

## Complete repository and application rename

The repository is now
[`Legatto2/Legatto_Device_Studio`](https://github.com/Legatto2/Legatto_Device_Studio).
The desktop title, About dialog, executable/build target, command-line identity,
report metadata, default report filename, documentation, build options, and test
environment use the Legatto name. The new target is `LegattoDeviceStudio` and
the build/environment prefix is `LEGATTO_`.

Existing scripts must migrate to the current names; the old target/options are
not advertised as compatibility aliases. Reconfigure a fresh CMake build
directory so an old cache cannot silently preserve an obsolete option.

The archived client is now built as `LegacyServiceClient`, with
`LegacyServiceClient.pro` and `LegacyServiceClient_resource.rc`. Original names
are retained only where needed to explain historical attribution, old report
metadata, or source evidence at a pinned commit. See
[PROJECT_HISTORY.md](PROJECT_HISTORY.md).

The JSON report remains schema version 1: fields and types are unchanged.
Consumers that previously compared `application` to the historical
`MotoTool_MTK` should accept `Legatto Device Studio` for new reports. Do not
reinterpret old report metadata.

## Naming and claims

An initial public-web check of the exact name and related Android software names
found no obvious exact-name product conflict on 22 October 2026. This name choice
is not a trademark search, registration, or legal clearance. No domain,
trademark registration, vendor affiliation, or endorsement is claimed.

Use the [privacy notice](../PRIVACY.md), [development disclaimer](../DISCLAIMER.md),
and [licensing review](LICENSING_REVIEW.md) alongside product copy. Credit the new
original work in [COPYRIGHT.md](../COPYRIGHT.md), apply the confirmed
[rights-reserved notice](../LICENSE), and keep inherited credits in the project
history rather than presenting them as authorship of the new product.

The default diagnostics target no longer embeds the inherited Windows icon.
An original product icon and distributable artwork can be added in a separate
reviewed design change, with their source and rights recorded.
