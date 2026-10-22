# Licensing, attribution, and publication review

**Project: Legatto Device Studio · Review date: 22 October 2026**

The new original development work belongs to **Michael Heald (Legatto2)**.
The credit is **Copyright © 2026 Michael Heald (Legatto2), for new original
contributions**. [COPYRIGHT.md](../COPYRIGHT.md) records this separately from
inherited authorship and third-party rights.

The owner has explicitly chosen **rights reserved** for the new original work.
[LICENSE](../LICENSE) records that decision without granting MIT, Apache, GPL,
or other general reuse/distribution permission. This file records the facts,
confirmed decision, and dependency/publication obligations. It is not a claim
to ownership of inherited or third-party material.

## Source evidence

The inspected baseline is commit
[`4c7741b`](https://github.com/Legatto2/Legatto_Device_Studio/commit/4c7741b), with the emulator
and branding work reviewed separately in the current change. Repository history
is evidence of attribution and the evolution of files; it is not a complete
record of assignments, permissions, or every component's original source.

| Material | Evidence | Current conclusion |
| --- | --- | --- |
| New diagnostics, tests, workflow, and documentation work | Current development contributions and recorded maintainer attribution | Copyright Michael Heald (Legatto2), 2026; owner has confirmed rights reserved |
| Inherited application source | Early commits attributed to `ABC` and `x`; commit [`80e245e`](https://github.com/Legatto2/Legatto_Device_Studio/commit/80e245e0d0f2ed96f37713138887711e25e65762) attributed to Mubashir Iqbal ([Mubshr07](https://github.com/Mubshr07)) | Preserve attribution; no blanket upstream license grant found |
| Repository-wide license | No `LICENSE`/`COPYING` grant in the tracked baseline or inspected available license-file history; no source SPDX/license header found in the reviewed code | Do not label the whole inherited repository MIT, Apache, GPL, or otherwise licensed by inference |
| Legacy Windows metadata at baseline | Historical `MotoTool_MTK_resource.rc` had an empty legal-copyright field; the current retained file is `LegacyServiceClient_resource.rc` | An empty historical field grants no rights and does not identify a publisher |
| Legacy artwork and styling | `resources.qrc` lists `Incrypt.qss`, checkbox/dropdown images, banner, logo, portrait, and `pics/appIcon.ico`; their history predates the new work | Attribution/permission is not established merely by inclusion in Git |
| Default Windows diagnostics icon at baseline | `cmake/app-icon.rc.in` points to `pics/appIcon.ico`; baseline CMake embeds it in the default diagnostics executable as well as legacy | Removed from the default diagnostics target in this rebrand; retained only by opt-in legacy compilation |
| Legacy helper binaries/data/services | References exist, but no verified redistributable runtime bundle or provider permission record was established | Keep outside the default product and any new licensing claim |

Public availability on GitHub is not a general software-license grant. GitHub's
terms allow viewing/forking within the platform, while broader reuse needs its
own applicable permission or legal basis. See
[GitHub's licensing guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository).

The rebrand must keep the new default application separate from the inherited
client and avoid presenting old artwork as newly authored. Replacing an icon or
separating build targets does not by itself settle rights in other source files.
Retain [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md) and historical credits.

## Confirmed new-work rights decision

**Rights reserved.** No broad permission to use, copy, modify, redistribute,
sublicense, or sell the new original contributions is granted by the project
notice. Existing valid grants, statutory exceptions, and applicable platform
rights remain unaffected. A public source repository is not a product EULA.

This decision applies to Michael Heald's new original contributions; it does not
change upstream ownership or the licenses of Qt, Android SDK packages, or other
dependencies. Qt's LGPL route does not require selecting GPL for the
application's own original code, but its applicable obligations still need to
be satisfied if that route is used.

Keep README, About, copyright, packaging, and contribution notices consistent
with [LICENSE](../LICENSE). Any future permission grant needs an explicit owner
decision and a defined component scope. Do not apply new terms retroactively to
upstream material the owner has not licensed. Keep branding/trademark decisions
separate from code permissions. A name search is not trademark registration or
legal clearance.

## Dependency and distribution review

| Component | Actual role | Required release decision |
| --- | --- | --- |
| Qt Core and Widgets, with transitive runtime/platform plugins | Default application's runtime/UI | Select the actual Qt kit/version and licensing route; inventory every shipped DLL/library/plugin and its notices |
| Qt Test | Ordinary and optional emulator tests | Development dependency unless test binaries are distributed |
| Qt Network and SerialPort | Optional legacy target only | Do not add these or legacy binaries to the new product package by accident |
| Android Platform-Tools / ADB | Operator-selected separate executable | Keep separate acquisition documented; review terms for any future bundling proposal |
| Android Emulator, Command-Line Tools, and API 35 Google APIs x86-64 image | Dedicated disposable-emulator test lane | Record package revisions and applicable SDK/package terms; do not publish AVD disks or SDK images as project artifacts |
| CMake, Ninja, Python, compiler/build tools, GitHub Actions | Development and CI | Preserve their own terms/notices where distributed; source pinning is not a license grant |
| New branding assets | New product identity | Use original or licensed material; record provenance and scope of rights |
| Inherited assets/helpers | Optional legacy review | Exclude from a new product package until the affected permissions are resolved |

### Qt is a separate licensing decision

Qt offers commercial and open-source routes, and some modules have different
terms. Consult the [official licensing overview](https://doc.qt.io/qt-6/licensing.html)
for the selected version. CMake's `STATIC` declarations for this project's own
libraries do not determine whether the selected Qt kit links Qt statically or
dynamically; verify the resulting binary and package.

For an LGPL route, the release process must preserve the required library
notices/license texts, arrange applicable corresponding-source delivery or
offers, and respect the user's required ability to modify/replace/relink the
covered library. A proprietary EULA must not prohibit reverse engineering where
the LGPL requires it for debugging library modifications. Static Qt linking
requires a deliberate compliance plan; it is not made compliant by including
one text file. See [Qt's obligations guidance](https://www.qt.io/development/open-source-lgpl-obligations)
and the [LGPLv3 text, especially section 4](https://doc.qt.io/qt-6/lgpl.html).

Qt's own third-party components can add notices. Inventory what is actually
shipped, including platform, image, font, and other runtime plugins; do not copy
a latest-version dependency list and imply it describes a different build.
[Qt documents this component-specific requirement](https://doc.qt.io/qt-6/licenses-used-in-qt.html).
A paid Qt license is one possible route, not an automatic prerequisite for this
project's development or distribution.

### Android SDK packages are not project-owned

Platform-Tools, the emulator, and system images are separate packages. Their
applicable licenses can distinguish SDK-distribution terms from individual
open-source component terms. Do not infer a right to redistribute Google's whole
binary distribution from the existence of ADB source code or the fact that a
download costs nothing. Keep the current separate-install model unless an actual
package/permission review supports bundling. See the
[Android SDK agreement](https://developer.android.com/studio/terms) and the
[Platform-Tools distribution page](https://developer.android.com/tools/releases/platform-tools).

## Privacy publication decisions

[PRIVACY.md](../PRIVACY.md) describes observed development-code behavior: local
processing, explicit tool/device selection, the five allowed properties,
pseudonymous exports, no implemented telemetry/upload feature, and user-controlled
plain JSON files. It distinguishes the application from ADB, the browser,
GitHub, the host operating system, and the legacy client. It does not claim
that the entire computer is offline, that a report is anonymous, or that every
device-provided string is non-sensitive.

Michael Heald (Legatto2) is the identified maintainer. Before collecting support
data or offering a hosted/customer service, publish the actual entity/contact,
processing purposes and applicable legal basis, recipients/transfers,
retention/deletion arrangements, and rights-request procedure for that service.
Do not invent a company address, privacy email, jurisdiction, response promise,
or regulatory certification. Applicable requirements depend on the deployment
and people served. The [ICO's privacy-information guidance](https://ico.org.uk/for-organisations/uk-gdpr-guidance-and-resources/individual-rights/the-right-to-be-informed/what-privacy-information-should-we-provide/)
is a primary example for UK-GDPR contexts, not a conclusion that UK law governs
every use of this project.

## Disclaimer and liability limits

[DISCLAIMER.md](../DISCLAIMER.md) provides the requested no-warranty,
assumption-of-risk, and liability language for development software, including
device damage, data loss, interruption, and financial loss. All exclusions are
qualified by applicable law, and non-excludable rights are preserved. It does
not claim that the owner can never be liable or invent a binding click-through
agreement.

Mandatory law can limit exclusions. As a concrete example, the UK's
[Consumer Rights Act 2015, section 65](https://www.legislation.gov.uk/ukpga/2015/15/section/65)
restricts excluding negligence liability for death or personal injury in its
consumer context; [section 47](https://www.legislation.gov.uk/ukpga/2015/15/section/47)
limits excluding specified digital-content liabilities. These illustrate why
absolute immunity is not promised; they do not establish which law governs a
particular deployment. Obtain jurisdiction-specific legal review before relying
on commercial distribution terms or a liability exclusion.

## Concrete publication gate

| Decision/evidence | Status |
| --- | --- |
| New original-work owner/credit | Michael Heald (Legatto2), 2026 |
| Permission model for new original work | Owner confirmed rights reserved; recorded in LICENSE without a general reuse grant |
| Inherited attribution | Preserved; broader upstream permissions unresolved |
| Inherited icon in new default executable | Removed from the default target; legacy resource kept separate |
| Brand name and trademark scope | Legatto Device Studio selected; no registration/clearance claim |
| Application privacy behavior | Source-reviewed development notice drafted; external-tool/host behavior separately scoped |
| Privacy contact/customer-service terms | Not designated; do not solicit customer data through an invented channel |
| Warranty/liability notice | Drafted with mandatory-rights limitation; no absolute-immunity claim |
| Runtime package inventory and required notices | Must be produced for the actual release artifact |
| Physical-device/package acceptance | Governed by the separate test matrix; legal notices do not establish compatibility |

Keep source and binary release decisions separate. A passing CI run, a new name,
an About dialog, or a disclaimer does not grant dependency rights or turn
development code into a validated service product.
