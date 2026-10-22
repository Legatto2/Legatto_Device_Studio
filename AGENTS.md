# Development instructions

## Product and boundaries

MotoTool is being developed into a reliable desktop workspace for authorized
Android diagnostics and, after separate validation, servicing. The current
default target is `MotoTool_Diagnostics`. Read `docs/ARCHITECTURE.md` and
`docs/AUDIT.md` before changing execution behavior.

- `diagnostics/` contains the new application. `Service` is the sole command
  boundary; the window presents state and exports its already filtered report.
- Root-level `mainwindow`, `backendclass`, `metamodethread`, and `globalvars`
  are preserved legacy code. CMake builds them only with
  `MOTOTOOL_BUILD_LEGACY=ON` and Qt 5. Do not reconnect that backend to the new UI
  or infer that compilation validates its device operations.
- Preserve upstream attribution and inherited work. Distribution rights are
  unresolved; consult `THIRD_PARTY_NOTICES.md` before licensing or packaging.

## Work sequence

1. Inspect the current branch, worktree, relevant issue/PR, and affected code.
   Record the concrete behavior and acceptance criteria before editing.
2. Keep the change focused. Separate observed facts, hypotheses, and planned
   capabilities. Do not overwrite concurrent or unrelated edits.
3. Implement through existing modules. Prefer Qt ownership and signal/slot
   patterns, immutable per-operation state, and explicit typed outcomes.
4. Test the affected boundary using synthetic data and the compiled fake ADB.
   Run the relevant full CTest suite once integration is complete.
5. Review the diff, update user/developer documentation, and prepare a PR with
   actual test outcomes and limitations. Follow the user's existing merge and
   release authorization; do not infer a hardware release from a passing build.

## Execution and privacy requirements

- Diagnostics may invoke only the approved native ADB path with separate
  arguments and fixed read-only commands. Do not add arbitrary command text,
  shell wrappers, automatic elevation, executable downloads, or PATH fallback.
- Bind every device command to the explicitly selected serial and revalidate
  the connection. Never select the first attached device as a fallback.
- Bound process duration and output. Handle cancellation, startup failure,
  nonzero exit, crash, malformed data, and reconnects with one terminal result.
- Never log or publish passwords, tokens, IMEI values, raw customer serials,
  private filesystem paths, or unfiltered command output. Review report fields
  as an allowlist. Do not copy historic credential values into test fixtures.
- Tests and ordinary CI must not invoke real ADB/USB devices, legacy account
  services, billing endpoints, or servicing binaries. Physical-device tests
  require a dedicated documented test session and the matrix in
  `docs/DEVICE_TEST_MATRIX.md`.
- A device-changing feature needs its own reviewed operation plan, supported
  device/firmware evidence, recovery behavior, and acceptance tests before it is
  exposed to users. Do not treat a successful process exit as verified repair.

## Build and verification

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug

cmake --preset linux-qt6
cmake --build --preset linux-qt6
ctest --preset linux-qt6
```

On Windows use the `windows-msvc` configure/build/test presets and the matching
Qt MSVC kit. See `CONTRIBUTING.md` for setup and the optional legacy build. CTest
sets `MOTOTOOL_TEST_ADB` to the fake fixture; do not substitute a production ADB.

Keep dependencies and GitHub Actions pinned where supported, workflow tokens
read-only by default, secrets out of PR workflows, and evidence tied to a commit
and toolchain. Report compilation, synthetic tests, UI checks, Windows checks,
and physical-device tests separately. Never mark an unrun check as passed.
