# Thermo roadmap

Thermo is created by Kairos Medical Informatics Ltd. Our first audience is Omarchy users. This roadmap describes ordered milestones, not promised dates.

## 1. Documentation and attribution — delivered in this change

- Publish an accurate README with screenshot, setup, compatibility, troubleshooting, uninstall, and contribution information.
- Establish the PRD and this roadmap, distinguishing shipped features from requirements.
- Credit the company in the README and Settings About section while preserving existing licence notices.

Completion: documentation matches the current code and scripts; About is readable and links to the repository; the build and existing tests pass. Fresh-machine installation remains part of milestone 4.

## 2. Trustworthy readings — delivered

- Remove invented temperature fallbacks and distinguish valid, unavailable, and stale readings.
- Handle missing sensors, malformed readings, sensor disappearance, and recovery explicitly, including composite temperatures.
- Document how unavailable readings appear in the rail, badge, Settings, and JSON output.

Completion: fixture-based tests cover failures and recovery; the interface never presents an invented value as a live measurement. Specify any JSON compatibility changes before implementation.

## 3. Full Omarchy theme integration — next

- Follow background, foreground, muted text, accents, borders, typography, and compatible style tokens.
- Apply theme changes live and provide a Classic Glass appearance option.
- Preserve the blue-to-red thermal scale and ensure readable fallbacks when theme tokens are missing or unsuitable.

Completion: verify light and dark themes, low-contrast palettes, missing files, and live theme replacement. Hover geometry remains stable and existing preferences are preserved.

## 4. Polished public release — planned

- Validate dependencies, installation, upgrades, autostart, and uninstall on a clean supported environment.
- Publish a versioned release with a reproducible Arch package and clear compatibility information; consider AUR publication after packaging is ready.
- Add automated build/data checks, contribution guidance, issue templates, and a short demonstration.
- Validate desktop fit and accessibility: display scaling, monitor selection, either edge, keyboard inspection, and reduced motion.
- Establish an idle resource-use baseline and retain a lightweight footprint.

Completion: a new user can follow the published instructions successfully; released artifacts correspond to a source tag; documented automated and manual checks pass. Wider hardware claims require testing on that hardware.

## 5. Temperature trends and history — planned

- Add an unobtrusive warming/cooling indicator and optional compact recent-history sparkline.
- Keep recent history local and bounded; represent missing measurements as gaps.

Completion: optional history does not move hover targets, misrepresent missing readings, or turn the badge into a dashboard. Retention and sampling details will be specified before implementation.

## Beyond this roadmap

Broader Linux desktop support and further hardware integrations can follow demonstrated demand. Fan control, cloud accounts, and automatic thermal intervention are outside the current product direction.
