# Thermo product requirements

## Purpose and audience

Created by Kairos Medical Informatics Ltd, Thermo gives Omarchy users a quiet, readable view of hardware temperature and fan activity. It should feel at home on the desktop and remain simple enough to understand at a glance.

The initial audience is Omarchy users, with verified behaviour currently limited to an Intel MacBook. Wider hardware support follows evidence from testing rather than compatibility claims.

## Current product

The shipped implementation provides a fixed right-edge rail, a stable hover badge with a smoked glass finish, selectable hardware temperature, fan RPM, Celsius/Fahrenheit, vertical dragging, Settings, and login autostart. The default height is 127 pixels. Settings uses the Omarchy accent and window rounding; the badge retains a fixed palette.

Temperature failure handling is incomplete: some paths show stale or fallback values. The following trustworthy-data requirements are targets, not claims about the current implementation.

## Experience requirements and acceptance criteria

| Area | Requirement | Acceptance criterion | Status |
| --- | --- | --- | --- |
| Ambient display | Remain unobtrusive behind normal windows | Hover changes neither rail position nor width; transparent areas do not block unrelated desktop interaction | Implemented; further regression coverage needed |
| Readout | Remain readable and easy to reach | Badge stays open over rail, connecting gap, and badge; sensor updates do not shift it | Implemented and manually checked |
| Measurements | Clearly distinguish readings from failures | Missing, invalid, disconnected, and stale temperature data are identified; no invented temperature is displayed | Next milestone |
| Fan reporting | Report available hardware fans without controlling them | Each discovered fan has a label and RPM; zero is valid; invalid or absent data is unavailable | Implemented with fixture tests |
| Theme integration | Follow Omarchy while preserving thermal meaning | Theme changes update surfaces, text, borders, and typography without restart; Classic Glass remains selectable; temperature remains blue-to-red | Planned |
| Accessibility | Support legible and accessible inspection | Maintain readable contrast and allow keyboard access and reduced motion | Planned; not yet audited |
| Installation | Be reproducible for a new user | Build, package, install, launch, autostart, upgrade, and uninstall succeed on a clean supported environment | Scripts exist; clean-machine validation planned |
| Resource use | Stay lightweight during routine use | Record idle CPU, memory, and wakeups on the reference machine and compare future changes against that baseline | Measurement planned; no numerical claims yet |

## Product boundaries

Keep the rail and badge central. Do not add fan control, automatic thermal intervention, accounts, cloud services, or a general-purpose monitoring dashboard in this roadmap. The product reports computer hardware readings; it is not a medical thermometer.

Use real Omarchy theme tokens with readable fallbacks. Do not infer thermal colours solely from theme colour names, which may not correspond to the named hue. Preserve existing preferences when features evolve.

## Definition of a release-ready product

A new user can understand the product from the README, install it using tested instructions, obtain honest readings or a clear unavailable state, and find a straightforward route to reporting issues or contributing. Automated checks cover data handling; manual Wayland checks cover hover, dragging, placement, scaling, and theme changes. Compatibility and known limitations are explicit.

Adoption will be evaluated through installation feedback, reported compatibility, resolved defects, and useful outside contributions. No usage telemetry is required for these milestones.

## Delivery sequence

See [ROADMAP.md](ROADMAP.md). Requirements are intentionally separated from shipped capabilities; each milestone updates both documents as evidence accumulates.
