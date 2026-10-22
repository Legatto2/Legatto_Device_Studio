# Legatto Device Studio development disclaimer

**Development build: `0.1.0-dev` · Reviewed: 22 October 2026**

Legatto Device Studio is development software for authorized Android device
diagnostics. It is not a supported production servicing release, a guarantee of
device compatibility, or a certification that a device is healthy or secure.

## No warranty

**To the maximum extent permitted by applicable law, this software and its
documentation are provided "as is" and "as available", without express, implied,
or statutory warranties, including warranties of merchantability, fitness for a
particular purpose, title, non-infringement, accuracy, reliability, availability,
or freedom from errors.** No guarantee is made that use will be uninterrupted,
that every defect will be corrected, or that every tool, driver, device, firmware,
or host configuration will work correctly.

## Use at your own risk

To the extent permitted by applicable law, you assume the risks of evaluating,
installing, building, running, or modifying this development software. Use only
devices you own or are authorized to access. Verify the selected device and
tool, protect important data, and decide whether the test environment is
appropriate before proceeding. Do not use an unvalidated workflow where failure
could cause unacceptable loss or interruption.

The current diagnostics application issues a restricted set of read-only ADB
queries. It does not expose firmware flashing, data erasure, bootloader unlock,
identifier modification, or the inherited servicing workflows. Read-only intent
does not guarantee that software defects, a malicious or incompatible selected
executable, operating-system/driver behavior, or a failing device cannot cause
harm. A tool hash identifies a file; it does not authenticate its publisher.

Reported values come from the device. They are not a trusted security attestation,
a forensic conclusion, a repair guarantee, or proof that all advertised patches
are installed. A successful emulator test or source build does not establish
physical-device compatibility or safe operation of the legacy client.

## Limitation of liability

**To the maximum extent permitted by applicable law, Michael Heald (Legatto2)
and the contributors to this development work disclaim liability for direct,
indirect, incidental, special, consequential, exemplary, or other loss or damage
arising from or related to its use, modification, distribution, or inability to
be used, including device or hardware damage, an unusable device, loss or
corruption of data, loss of access, downtime, security incidents, lost revenue,
lost profits, business interruption, and other financial loss, even if advised
that such loss or damage might occur.**

This limitation applies only where and to the extent the law allows. **Nothing
in this notice excludes or restricts non-excludable liability, mandatory consumer
protections, or other rights and remedies that applicable law does not permit to
be excluded or restricted.** A disclaimer does not create absolute immunity
from legal responsibility.

## Third parties, permissions, and scope

This project does not claim endorsement by Motorola, Lenovo, MediaTek, Google,
Android, or Qt. Names of products and companies identify interfaces, devices, or
dependencies. Their licenses, trademarks, and other rights remain separate.

This notice does not grant a software license, settle inherited rights, override
a dependency's license, or authorize access to someone else's device. Copyright
for the new original contributions is attributed to Michael Heald (Legatto2);
upstream attribution is preserved in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). Licensing decisions are recorded
in [LICENSING_REVIEW.md](docs/LICENSING_REVIEW.md).

The separate inherited `LegacyServiceClient` application is preserved for engineering
review and has material unresolved risks. See [AUDIT.md](docs/AUDIT.md).
Read [PRIVACY.md](PRIVACY.md) before exporting or sharing diagnostic information.
Any future commercial distribution or service agreement needs terms appropriate
to its publisher, customers, and applicable law; this notice does not invent
an agreement, jurisdiction, or acceptance mechanism.
