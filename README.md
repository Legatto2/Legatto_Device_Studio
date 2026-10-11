# MotoTool_MTK

Legacy Windows-oriented Qt/C++ desktop client for Motorola servicing workflows.
The application source identifies itself as 0.0.3. **This is not a production-ready
or hardware-validated release. Do not use it on customer equipment.**

## First hardening increment

This branch preserves the existing UI and most legacy workflows while addressing
specific containment defects. It does not establish that the remaining operations
are safe or compatible with current devices.

- FRP is disabled in the UI and backend. The unauthenticated HTTP retrieval and
  response-to-shell execution path have been removed, not replaced or re-enabled.
- Login preferences store only an optional username. Legacy plaintext passwords
  are discarded from the active settings file during startup migration. Passwords
  and tokens are never passed to the new preferences writer. Unchecking the
  checkbox forgets the saved username immediately.
- Login form values are percent-encoded individually, including literal `+` and
  `&` characters. The login screen is explicitly shown at startup.
  Embedded username/password defaults have been removed from the UI resource.
- Legacy `qDebug()` output is disabled in both debug and release application builds.
  This is temporary containment, not a replacement for structured redacted logging.
- Automatic remote log uploads are disabled. Existing local logs are not deleted
  or scrubbed and may still contain sensitive identifiers.
- The ADB charge-response boundary now has a 15-second deadline, transport/HTTP
  checks, strict success-field validation, and a return value on every path.
- Serial-port choices are rebuilt from the current snapshot, preserve selection
  by name on reorder, and clear disconnected selections instead of silently
  selecting another device. Selection bounds are checked before legacy indexing.

Migration does not securely erase historical disk blocks, backups, or old logs.
If migration cannot safely replace the file, login is disabled and an error is
shown. Resolve the settings-file issue and restart; do not treat an error as a
successful cleanup.

## Build and test

The CI baseline uses Qt 5.15.13 and GCC 13 on Ubuntu 24.04 to compile the Qt code
without launching the application. The runtime remains Windows-oriented. Linux
compilation is not a Windows packaging, driver, external-tool, or hardware test.
The project has not been ported or validated against Qt 6.

From a configured Qt 5 build environment:

```sh
mkdir build
cd build
qmake ../MotoTool_MTK.pro CONFIG+=debug "QMAKE_CXXFLAGS+=-Werror=return-type"
make -j2
```

On Windows, use the matching build command for your Qt kit, such as `mingw32-make`
or `nmake`, rather than mixing compilers with a different Qt binary distribution.

Run the hardware-free tests:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
mkdir build-tests
cd build-tests
qmake ../tests/security_tests.pro CONFIG+=debug "QMAKE_CXXFLAGS+=-Werror=return-type"
make -j2
QT_QPA_PLATFORM=offscreen ./security_tests -txt
```

The Qt tests compile the same preferences, form-encoding, charge-parser, and
port-selection implementations used by the application. They use temporary
files and synthetic data, not phones, real credentials, or remote services.
The Python tests are source-level regression guards, not functional hardware tests.

## Dependencies and distribution

The original application expects separately installed utilities/data under `M3`,
`ADB`, and `DATA`, and access to external services. Those dependencies and service
implementations are not supplied here. Do not download unknown replacement
binaries or assume this source grants rights to distribute external components.
Source and third-party licensing must be established before redistribution.

See [docs/HARDENING.md](docs/HARDENING.md) for remaining release blockers and
validation boundaries. The existing security-policy file is retained unchanged;
its template version table is not a validated support promise for this application.
