# Voltline Cluster Design System

A design system for **digital instrument clusters on electric bicycles and motorcycles**, running as a full-screen app on handlebar-mounted tablets, plus the rider's phone companion app.

**Voltline is a working brand name invented for this system.** No company assets, codebase, Figma file, deck, or logo were supplied with the brief — the entire system (name, palette, type pairing, components) is authored from scratch and is meant to be adjusted or rebranded once real materials exist. Nothing here is a recreation of an existing product.

## Sources given
- Brief (chat only): "Develop a digital instrument cluster UI/UX for electric bicycles and motorcycles, designed to run on tablets or mobile devices."
- Figma files: none provided.
- Repositories / codebases: none provided.
- Decks, screenshots, brand guidelines, logo files: none provided.

External conventions the system deliberately follows (not invented): automotive telltale colour law — green for turn/ready, blue reserved for high beam, amber for degraded, red for stop-riding — as used in ECE/ISO instrument-cluster practice.

## Products
1. **Cluster display** — 8" landscape tablet, 1280×800, mounted on the vehicle. Read at a glance at speed, in gloves, in sun and at night. `ui_kits/cluster/`
2. **Companion app** — phone, 390×844. Charge, lock, ride history, setup. `ui_kits/companion-app/`

---

## Content fundamentals

**Voice: a competent co-rider, not a brand.** The cluster states facts; the app explains consequences. Nothing is cheerful, nothing apologises.

- **Person.** Address the rider as *you* only when there is an action for them: "Pull over when safe." Never *I*, never *we*, never the vehicle in first person. The bike is *the bike*, or its name.
- **Length.** Cluster strings: 1–4 words ("Motor over temperature", "Ready"). App strings: one sentence, ~12 words max. A fault always pairs a condition with a rider action: *"Power limited. Pull over when safe."*
- **Casing.** Sentence case for anything a rider reads as language ("Auto headlight", "Turn right onto Kanaalweg"). UPPERCASE + 0.08em tracking for field labels, units and mode names ("DISTANCE", "ECO"). Never all-caps sentences.
- **Units.** Lowercase and separated from the number: `18.4 km`, `9.2 Wh/km`, `52.4 V`. Speed never carries decimals; distance and energy take one. Percent has no space: `62%` in prose, split numeral/label in readouts.
- **Numbers are the copy.** Where a number can carry the meaning, cut the sentence. "48 km left", not "You have around 48 kilometres of range remaining."
- **Faults.** Plain-language title, one-line consequence, mono fault code for the technician: *Motor over temperature / Power limited. Pull over when safe. / E-412.* Never "Oops", never "Something went wrong".
- **Uncertainty is stated, not hidden.** "Range estimate may drop sharply on hills." If an estimate is untrustworthy, omit it rather than soften it.
- **No emoji. Ever.** Not in the cluster, not in the app, not in notifications. Icons carry the same job with a fixed meaning.
- **No exclamation marks**, no marketing adjectives ("amazing range"), no gamified praise ("Nice ride!"). A ride summary reports; it does not congratulate.

Examples — good vs. avoid:

| Good | Avoid |
|---|---|
| Bike locked | Your bike is now safely locked! 🔒 |
| Battery below 12% | Uh oh — you're running low on juice |
| Book service within 300 km | Service recommended soon |
| Firmware 2.8.1 ready. Installs next time the bike is parked and charging. | A new update is available for your ride |

---

## Visual foundations

**The screen is an instrument, not a page.** Everything is fixed: no scrolling on the cluster, no reflow, no surprises in element position. The rider learns where a value lives and looks there without reading.

- **Colour.** Night-first. A 14-step ink ramp from `--ink-950` (void) up to `--ink-050` (primary text) carries all structure; one brand accent — *current* cyan `#00E5D0` — marks whatever is live or selected. Signal colours (green / blue / amber / red) are legally-flavoured and single-purpose: they are never used decoratively, never tinted, never applied to a surface larger than a lamp, badge or bar. A day scope (`[data-theme="day"]`) inverts surfaces and text for direct sunlight; it swaps values, never tokens, and is applied at the root of a screen — never mixed mid-layout.
- **Type.** Three families. *Barlow Semi Condensed* for every numeral a rider glances at (condensed so 3-digit speeds fit at 132–180px), *Archivo* for labels and prose, *JetBrains Mono* for codes, serials, firmware and timestamps. All numerals are tabular so digits never jitter as values change. Readouts: −0.02em tracking, 0.86 line-height. Labels: uppercase, 0.08em. Minimum on-cluster size is 11px, and only for units.
- **Spacing & layout.** 2px base, 4px rhythm. Cluster gutter 24px, tile gap 12px, app gutter 20px. The cluster is a fixed three-band layout: 44px telltale rail, content, 64px view bar. Content is a 3-column grid (300 / flex / 300). Touch targets: 44px minimum, 56px for anything used while moving or gloved.
- **Backgrounds.** Flat. No photography, no illustration, no gradient washes, no texture, no grain. The only gradient in the system is functional — the `conic-gradient` that draws the arc gauge. Full-bleed imagery would cost contrast and legibility in sunlight, so it is not used.
- **Surfaces & cards.** Every container is `Panel`: `--surface-panel` fill, 1px `--line-hairline` border, 20px radius, and an inset 1px top highlight instead of a drop shadow. Radii step by function: 6 chip, 10 control, 14 tile, 20 panel, 28 screen, pill for badges and tracks. Never a coloured left border.
- **Elevation.** Cockpit surfaces glow, they don't float. Real drop shadows appear only on the modal scrim (`--shadow-modal`) and the slider knob. Emphasis is a coloured glow instead: `--glow-accent`, `--glow-caution`, `--glow-critical`, plus a per-lamp `drop-shadow` on active telltales.
- **Transparency & blur.** Used exactly twice: a 72%-opacity scrim with 12px backdrop blur behind `Dialog`, and 10–16% colour washes behind badges and alert banners. No frosted panels, no translucent chrome — vibration and low contrast make them unreadable.
- **Borders.** 1px hairlines everywhere; 2px only to mark selection (`RideModeSelector`). Dashed 1px marks a region this system does not own (the map area).
- **Animation.** Short and non-decorative. 80ms telltale on/off and press, 140ms hover and chip select, 220ms panels, 400ms mode/screen change, one 900ms arc sweep at power-on. Easing is `cubic-bezier(.2,0,0,1)` standard and `(.33,1,.68,1)` for numeric settle — **nothing bounces, nothing springs, nothing loops** except the two legally-required blinks: turn indicators at 800ms, caution at 1200ms, both hard `steps(1,end)`.
- **States.** Hover is a surface-lightening step (raised → next ink), never an opacity change on text. Press is `scale(.97)` at 80ms — colour stays put so a gloved rider sees movement, not a hue shift. Selected is the accent fill or a 2px accent border plus accent text. Disabled is `--ink-800` surface + `--text-disabled`, never below 3:1 contrast. Focus is a 2px `--current-400` ring at 2px offset (keyboard/rotary-encoder navigation on bench tools).
- **Imagery.** There is none, by design. Where a photo or map would go, the host SDK renders it and this system supplies only chrome.

---

## Iconography

- **Set: Lucide (outline, 2px stroke), loaded from `unpkg.com/lucide-static@0.400.0`.** ⚠️ *Substitution:* no icon set was supplied with the brief, so Lucide stands in for a real brand set. Its even 2px stroke and rounded caps match the type's tone and stay legible at 20px on a vibrating display. Replace via the single `CDN` constant in `components/core/Icon.jsx` when a licensed set exists.
- **No icons are vendored into `assets/`** — they are fetched at render time. If you need offline builds, download the Lucide SVGs into `assets/icons/` and repoint that constant.
- **Always via `<Icon name="…" />`,** which renders the glyph as a CSS mask so it inherits `currentColor` and can therefore take signal colours. Never `<img>`, never inline hand-drawn SVG paths.
- **Sizes:** 13 (inside a badge/label), 20 (inline, default), 24 (control), 26–40 (telltale lamps). Never below 13.
- **Telltale glyphs are approximations.** Real clusters use ISO 7000 / ECE symbols (turn arrows, high beam, malfunction indicator). Lucide has no ISO set, so `chevron-left/right`, `lightbulb`, `octagon-alert`, `battery-warning` and `thermometer` stand in. **These must be replaced with certified ISO glyphs before any homologated build.**
- **Emoji: never.** Unicode characters as icons: never (`°` and `%` are typography, not icons).
- **No logo exists.** No brand mark was supplied, so nothing was drawn. Wherever a mark belongs, the word *Voltline* is set in Archivo 700, 0.22em tracking, uppercase — see `guidelines/brand-wordmark.card.html`. `assets/` is intentionally empty.

---

## Index

Root
- `styles.css` — the only file consumers link; `@import`s everything below.
- `readme.md` — this guide. `SKILL.md` — Agent Skills wrapper. `thumbnail.html` — homepage tile.

`tokens/` — `fonts.css` (Google Fonts CDN), `colors.css`, `typography.css`, `spacing.css`, `shape.css`, `motion.css`, `base.css`.

`guidelines/` — 19 specimen cards: colours (ink, brand, signal, energy, semantic, text, day scope), type (families, readout scale, UI scale, numeral rules), spacing (scale, layout & hit targets), shape (radii, elevation), motion (durations, easing), brand (wordmark, telltale colour law).

Components — 20, each with `.jsx`, `.d.ts`, `.prompt.md`, one `@dsCard` per directory:
- `components/core/` — **Icon**, **Button**, **IconButton**, **Panel**, **Badge**
- `components/forms/` — **Switch**, **SegmentedControl**, **Slider**, **TextField**
- `components/feedback/` — **AlertBanner**, **Toast**, **Dialog**
- `components/cluster/` — **SpeedReadout**, **ArcGauge**, **BatteryGauge**, **PowerFlowBar**, **Telltale**, **TelltaleRail**, **TripStat**, **RideModeSelector**

UI kits
- `ui_kits/cluster/` — RideScreen, TripScreen, NavScreen, DiagnosticsScreen, SettingsScreen + `index.html`.
- `ui_kits/companion-app/` — BikeScreen, RidesScreen, RideDetailScreen, SetupScreen + `index.html`.

`assets/` — empty; see Iconography for why.

### Intentional additions
- **Icon** — a wrapper, not a design decision: it is the only sanctioned way to render a glyph so that signal colours apply. Without it every consumer would inline SVG and lose `currentColor`.

### Known substitutions to resolve
1. Fonts are loaded from the Google Fonts CDN; no binaries are vendored. Supply licensed `.woff2` files and `@font-face` rules for offline/embedded builds.
2. Lucide stands in for a brand icon set; telltale glyphs are not ISO-certified.
3. No logo, no photography, no map style.
