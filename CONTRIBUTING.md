# Contributing to Thermo

Thermo is created by Kairos Medical Informatics Ltd and welcomes outside contributions under the existing [MIT licence](LICENSE). Retain relevant copyright and third-party notices.

## Before starting

Read the [README](README.md), [PRD](PRD.md), and [roadmap](ROADMAP.md). For substantial new features, open an issue to agree on the intended experience before writing code. Small fixes can go straight to a pull request.

Our first audience is Omarchy users. Preserve the quiet desktop rail, stable hover, readable badge, and honest sensor states. Prefer focused changes over adding a general monitoring dashboard.

## Local checks

Install the README's Arch dependencies, then run:

```bash
./bin/build
./bin/test
./bin/check-runtime
```

Unit tests use temporary hwmon fixtures and temporary configuration. The runtime smoke check also isolates its temporary socket directory, so it does not control your running Thermo instance. It covers startup, the autostart desktop file, IPC controls, shutdown, and JSON invocation.

The CI workflow builds in a fresh Arch container, runs the checks as an unprivileged user, packages the application, and tests package installation, reinstall, and removal. Its downloadable artifacts are development packages, not a published release.

## Manual Wayland checks

For interface changes, check rail stability during repeated hovering, badge/gap transitions, clicking versus dragging, saved position, and clicks through transparent areas. Check enabled and disabled fan rails, zero RPM and unavailable data, light/dark themes and Classic Glass, and live theme changes. Verify both minimum and increased font sizes for clipping. For Settings changes, check the 520-pixel minimum width, page/card padding, wrapped descriptions, scrolling, and persistent action buttons. Check monitor fallback and preference restoration separately from actual physical hotplug tests.

See [the interaction validation record](docs/INTERACTION-VALIDATION.md) for the latest event tests and compositor checks. Offscreen checks cannot validate Wayland input masks or compositor placement. For sensor-state changes, include failure and recovery cases in fixtures; never replace missing readings with invented temperatures.

## Submitting a pull request

Explain the user-visible problem, the resulting behaviour, and how you verified it. Include a cropped screenshot for visual changes. State the hardware and compositor used for manual checks and list any limitations. Do not include personal configuration, build products, or private desktop content.

Use the bug-report template for regressions and feature-request template for proposals. Do not promise compatibility with hardware you have not tested.
