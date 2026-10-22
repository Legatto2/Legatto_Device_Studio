# Authorship, dependencies, and distribution status

## New original work

**Copyright © 2026 Michael Heald (Legatto2), for new original contributions to
Legatto Device Studio.** The owner has chosen **rights reserved**, recorded in
[LICENSE](LICENSE) and [COPYRIGHT.md](COPYRIGHT.md).
[LICENSING_REVIEW.md](docs/LICENSING_REVIEW.md) provides the confirmed decision and
release obligations. This attribution does not claim ownership of the inherited
material below or of dependencies.

## Repository source

This repository contains inherited source and assets. Its history includes early
commits under `ABC` and `x`, and commit
[`80e245e`](https://github.com/Legatto2/Legatto_Device_Studio/commit/80e245e0d0f2ed96f37713138887711e25e65762)
attributed to **Mubashir Iqbal** ([Mubshr07](https://github.com/Mubshr07)), followed
by maintenance and hardening in `Legatto2/Legatto_Device_Studio`. These records establish
attribution evidence, not a complete ownership determination. The modernization
does not claim original authorship of inherited work.

The consolidated historical credits and the boundary around the archived client
are in [PROJECT_HISTORY.md](docs/PROJECT_HISTORY.md).

No repository-wide license grant was found in the inspected source trees or
license-file history. The modernization does not relicense inherited code,
artwork, names, icons, or third-party utilities. No MIT, Apache, GPL, proprietary,
or other project-wide license is asserted by this notice.

Before distributing a release, the repository owner must establish permissions
for inherited material and choose terms only for material they can license.
Preserve author credits and notices. Public GitHub visibility alone does not
establish broad redistribution permission; see
[GitHub's licensing guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/licensing-a-repository).

## Dependency inventory

| Component | Use | Acquisition / rights status |
| --- | --- | --- |
| Qt Core and Widgets | Diagnostics runtime/UI | Obtain from Qt or an established distribution; comply with that distribution's terms |
| Qt Test | Ordinary fake-tool tests and the opt-in emulator test | Development dependency with applicable Qt terms |
| Qt Network and SerialPort | Optional legacy client | Not dependencies of the new diagnostics service; review terms before packaging |
| Android SDK Platform-Tools / ADB | Operator-selected diagnostics executable | Obtain separately from the official Android distribution; no binary included here |
| Android Emulator, Command-Line Tools, and Google APIs system image | Opt-in disposable-emulator validation | Separately installed SDK packages with applicable package terms; no SDK/AVD disk distribution in CI artifacts |
| CMake, Ninja, compiler, Python | Build/test tools | Development dependencies |
| GitHub Actions | Hosted build/test automation | Revisions declared in workflows; action terms and runner allowances are separate |
| Original icons, banner, portrait, stylesheet | Inherited legacy resources | Provenance/redistribution permission not established by Git presence |
| Legacy `M3`, `ADB`, `DATA` helper/data trees | Historical servicing | Referenced but not supplied as a verified bundle; rights, versions, hashes, and provenance unresolved |
| Legacy external APIs | Historical account/servicing | Ownership, availability, contract, and permission unvalidated; not used by new diagnostics |

The default diagnostics target no longer embeds the inherited `pics/appIcon.ico`.
That resource is retained only in the explicitly enabled legacy target. Removing
it from the new executable does not establish rights for other inherited assets.

### Qt

Qt has several licensing routes, module-specific terms, and third-party components.
Use the actual version/module distribution's notices rather than assuming a single
label covers all components. See the official
[Qt licensing overview](https://doc.qt.io/qt-6/licensing.html) and
[third-party component documentation](https://doc.qt.io/qt-6/licenses-used-in-qt.html).

This source repository does not contain a Qt runtime package. A future installer
must include required notices and satisfy its selected distribution's obligations;
this inventory does not replace those notices.

### Android Platform-Tools

Use [Google's distribution](https://developer.android.com/tools/releases/platform-tools)
or Android Studio's SDK Manager. The application records the chosen ADB file's
version and SHA-256 for traceability. Those values do not certify the publisher;
the application does not download replacements automatically.

Platform-Tools terms are separate from this repository. No right to bundle
executable/data files is inferred from their names in source.

## Release evidence

For each distributed dependency/asset, record its owner/source, version/revision,
acquisition URL, hash, permission evidence, required notices, and whether it is
bundled or separately installed. Keep source and binary decisions distinct.
Unresolved entries block the affected release package; compilation does not
resolve them.
