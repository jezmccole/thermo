# Release readiness

This is a verification checklist, not a claim that a public release is ready.

## Automated checks

The Build, test and package workflow covers a fresh Arch dependency installation, build, fixture tests, isolated startup/IPC/autostart-file/JSON checks, package installation and reinstall, and package removal. CI is headless; it cannot verify a real Omarchy login or a version-to-version migration.

## Required before tagging a release

- [ ] CI passes for the exact commit to be tagged.
- [ ] Install on a fresh supported Omarchy system; verify launch and login autostart.
- [ ] Upgrade from the previous installed version and verify saved preferences survive.
- [ ] Verify Wayland hover, badge transitions, input masks, click versus drag, both rails, and warning/recovery states.
- [ ] Verify light/dark themes, live replacement, Classic Glass, font sizes, and display scaling.
- [ ] Complete monitor/edge options, keyboard inspection, and reduced-motion work listed in milestone 4.
- [ ] Record idle CPU, RSS memory, and wakeups on the reference machine; assess changes against that baseline.
- [ ] Capture a short demonstration without private desktop content.
- [ ] Update application/package versions together and document JSON/API changes.
- [ ] Build artifacts from the source tag; include source, package, checksum, installation instructions, and known limitations in release notes.
- [ ] Verify uninstall and cleanup instructions.

Development CI package artifacts are not public release assets. Do not mark milestone 4 complete until the outstanding manual and accessibility work is done.
