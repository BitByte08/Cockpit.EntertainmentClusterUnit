# Cockpit — BMW M Design Requirements Harness

> **Source**: Claude Design export (`docs/design/`) — BMW M instrument cluster & entertainment IVI  
> **Target**: Qt6/C++ application, Raspberry Pi 4/5, 800 × 480 display  
> **Implementation status**: tracked per section

---

## 1. Design System (BMW M)

### 1.1 Colors

| Token             | Hex       | Usage                          | Qt constant      |
|-------------------|-----------|--------------------------------|------------------|
| `kBgCanvas`       | `#000000` | Primary window background      | ✅ implemented   |
| `kBgSurface`      | `#0d0d0d` | Panel backgrounds              | ✅ implemented   |
| `kBgCard`         | `#1a1a1a` | Cards, selected items          | ✅ implemented   |
| `kBgElevated`     | `#262626` | Hairlines, unlit cells         | ✅ implemented   |
| `kHairline`       | `#3c3c3c` | Borders, dividers              | ✅ implemented   |
| `kMBlueLight`     | `#0066b1` | M stripe left third            | ✅ implemented   |
| `kMBlueDark`      | `#1c69d4` | M stripe center third, accent  | ✅ implemented   |
| `kMRed`           | `#e22718` | M stripe right, error, redline | ✅ implemented   |
| `kFg1`            | `#ffffff` | Primary text, lit cells        | ✅ implemented   |
| `kFg2`            | `#bbbbbb` | Secondary text                 | ✅ implemented   |
| `kFg3`            | `#7e7e7e` | Muted labels, captions         | ✅ implemented   |
| `kWarning`        | `#f4b400` | Warning amber (ABS, TCS, fuel) | ✅ implemented   |
| `kSuccess`        | `#0fa336` | Success green (CAN OK, BT)     | ✅ implemented   |

### 1.2 Typography

| Scale              | Size  | Weight | Transform | Tracking | Usage               |
|--------------------|-------|--------|-----------|----------|---------------------|
| Display XL         | 80px  | 700    | UPPERCASE | -0.5px   | Speed number        |
| Display LG         | 56px  | 700    | UPPERCASE | -0.5px   | Gear letter         |
| Display MD         | 40px  | 700    | UPPERCASE | -0.25px  | Sub-values          |
| Label Uppercase    | 14px  | 700    | UPPERCASE | +1.5px   | Panel labels        |
| Caption            | 12px  | 400    | —         | +0.5px   | Unit labels         |
| Mono               | —     | —      | —         | —        | Lap times, ODO      |

**Font**: Inter (substitute for BMW Type Next Latin).  
**Qt fallback order**: `Inter, Noto Sans, -apple-system, sans-serif`

### 1.3 M Tricolor Stripe
- Height: 3 px
- Hard stops (no smooth gradient): `#0066b1 | #1c69d4 | #e22718` (equal thirds)
- Position: between indicator bar and RPM bar

### 1.4 Geometry Rules
- **No border-radius** (except where explicitly specified: drive mode chip = 0px, speed limit = circle)
- **No drop shadows** — depth through surface contrast and hairline borders (`1px solid #262626`)
- **Motion**: `120ms / 200ms / 400ms` cubic-bezier(0.2, 0, 0, 1)
- **Tabular numbers**: `font-feature-settings: "tnum" 1` on all numeric displays

---

## 2. Cluster Application (800 × 480)

### 2.1 Layout Variants

Four layouts are defined. **Layout B (Center Speed)** is the primary implementation target.

#### Layout A — Classic Dual Dial
- RPM analog dial (left, 260 × 260)
- Speed analog dial (right, 260 × 260)
- Gear + drive mode chip (center)
- Bottom strip: ODO / TRIP | FuelBar | NowPlaying | OutsideTemp
- **Status**: ⬜ documented, not yet implemented

#### Layout B — Center Speed ✅ PRIMARY
- Top: full-width discrete-cell RPM bar (48 cells, 28 px high)
- Left panel: Gear indicator + FuelBar cells
- Center: giant speed numeral (≥ 150 px font)
- Right panel: Status indicators + CoolantTemp + OilPressure
- Bottom: CAN status + clock + version
- **Status**: ✅ implemented

#### Layout C — Minimal Digital
- No dials. Speed (168 px) | Gear | RPM cells in 3 columns
- Mid: Mode / Outside / Oil / Range metrics
- Bottom: FuelBar | TirePressure | NowPlaying
- **Status**: ⬜ documented, not yet implemented

#### Layout D — M Track
- Massive RPM bar across full width (64 cells)
- M stripe separator
- Left: lap time, best lap, delta, sector progress
- Center: speed (168 px) + gear + drive mode
- Right: G-force compass (SVG, concentric rings)
- Bottom: telemetry strip (Oil / Water / Fuel% / Outside / Track label)
- **Status**: ⬜ documented, not yet implemented

### 2.2 Vehicle State Presets

| State  | Speed   | RPM   | Gear | Fuel | Temp |
|--------|---------|-------|------|------|------|
| idle   | 0 km/h  | 800   | P/N  | 76%  | 88°C |
| drive  | 87 km/h | 3,400 | D4   | 68%  | 96°C |
| track  | 218km/h | 7,250 | S5   | 42%  | 104°C|

### 2.3 Component Specifications

#### RpmBarWidget (NEW)
- `int cells = 48` — discrete blocks
- Gap between cells: 2 px
- Cell color when lit:
  - Normal (< 85% of redline): `#ffffff`
  - Warn (85–100% of redline): `#f4b400`
  - Red (> redline): `#e22718`
- Unlit cell: `#1a1a1a`
- Height: 28 px default
- RPM label row above bar: "RPM × 1000" (left) + formatted value (right)
- **CAN source**: `0x400` byte[2..3] big-endian RPM field

#### FuelBarWidget (NEW)
- `int cells = 16`
- Gap: 2 px
- Lit cell: `#ffffff`; last 2 cells if fuel ≤ 15 %: `#e22718`
- Unlit: `#262626`
- Height: 8 px default
- Shows: label + pct above bar, range string below
- **CAN source**: `0x501` byte[2] (0–100)

#### SpeedDisplay (existing QLabel, restyled)
- Font: 96 pt bold (approx 128 px height at 96 DPI)
- Color: `#ffffff` (< 80 km/h) → `#f4b400` (80–129) → `#e22718` (≥ 130)
- "KM / H" unit label: 11 px, uppercase, letter-spacing 4 px, color `#7e7e7e`

#### GearDisplay (existing QLabel, restyled)
- Font: 80 pt bold
- Colors:
  - N (neutral): `#ffffff`
  - 1–6 (drive): `#1c69d4` (M blue)
  - R (reverse): `#e22718` (M red)
- Description text (NEUTRAL / GEAR 4 / REVERSE): 9 px uppercase, `#7e7e7e`

#### RPM Analog Dial — GaugeWidget (restyled)
- Background: pure black outer, `#0a0a0a` inner ring
- Red zone arc: `#e22718`
- Major tick: `#ffffff`; minor tick: `#3c3c3c`
- Needle: `#ffffff` (< 60%) → `#f4b400` (60–82%) → `#e22718` (> 82%)
- Center hub: metallic gradient; red center dot `#e22718`
- Label accent: `#1c69d4` (M blue, was teal)

#### ArcGaugeWidget (restyled)
- Background arc: `#1a1a1a`
- Fill arc colors (fuel mode): green `#0fa336` → yellow `#f4b400` → red `#e22718`
- Fill arc colors (temp mode): blue `#1c69d4` → green `#0fa336` → yellow `#f4b400` → red `#e22718`
- Label color: `#1c69d4`

#### IndicatorWidget (restyled)
- Background fill: `#000000`
- Dim (inactive) color: `#262626`
- Colors by type:
  - TurnLeft / TurnRight: `#0fa336`
  - HighBeam: `#1c69d4`
  - CheckEngine: `#e22718`
  - OilPressure: `#e22718`
  - ABS / TCS: `#f4b400`
  - FuelWarn: `#f4b400`

### 2.4 Screen Layout — Layout B Detail

```
┌─────────────────────────────────────────────────────────────┐
│ [TL][BEAM]    [ENG][OIL][ABS][TCS]    [FUEL][TR]  H=44px  │ indicator bar
│ ████████████████████████████████████████████████████ H=3px │ M stripe
│ RPM × 1000  [48 cells]████████░░░░░ 3,400          H=44px │ RPM bar row
│────────────────────────────────────────────────────────────│ hairline
│  W=280px       │         W=flex        │       W=240px     │
│  GEAR          │                       │   IGN: ON         │
│   D 4          │        8 7            │   ENG: ON         │
│                │                       │   LIGHT: OFF      │
│  FUEL    68 %  │       KM / H          │ ─────────────────  │
│  ████████░░░░  │                       │   COOLANT         │
│  362 km        │                       │   96 °C           │
│                │                       │   OIL PRESS       │
│                │                       │   50              │
│────────────────────────────────────────────────────────────│ hairline
│  CAN: can0 ●   14:28        update         v0.0.5  H=36px │ status bar
└─────────────────────────────────────────────────────────────┘
 Total fixed: 44+3+44+1+1+36 = 129 px. Main area = H - 129 px.
```

### 2.5 CAN Signal → UI Mapping (Layout B)

| CAN ID | Signal             | UI element                      |
|--------|--------------------|---------------------------------|
| 0x300  | switch_status      | Turn blink, high beam, IGN/ENG  |
| 0x301  | gear               | gearLabel_ (D/R/N/1–6)          |
| 0x400  | speed (×10 BE)     | speedValueLabel_ + color        |
| 0x400  | rpm (BE)           | rpmBar_ + rpmValueLabel_        |
| 0x401  | warning_flags      | checkEngineInd_, oilInd_        |
| 0x500  | gear/ABS/TCS       | gearLabel_, absInd_, tcsInd_    |
| 0x501  | temp, oil, fuel    | tempLabel_, oilPressureLabel_,  |
|        |                    | fuelBar_, fuelPctLabel_,        |
|        |                    | fuelWarnInd_, oilInd_           |

---

## 3. Entertainment Application (800 × 480)

**Status**: ⬜ documented, not yet implemented — requires new `apps/entertainment/` Qt application.

### 3.1 Chrome (IVIChrome)
- Left rail: 56 px wide, pure black, `border-right: 1px solid #262626`
  - M tricolor stripe (vertical, 3×28 px) at top
  - App icons: NAV, MEDIA, PHONE, CAR, M Drive (active = `border: 1px solid #fff`)
  - Settings icon at bottom
- Top status bar: 28 px, semi-transparent black (`rgba(0,0,0,0.6)`)
  - Left: time (Inter 700, 12 px) + date
  - Right: outside temp + BT status + 4G + DriveModeChip

### 3.2 Screen 1 — Navigation (Active Route)

Components:
- **Map base**: SVG with terrain fill, road grid, park polygon, water polygon
- **Route path**: thick blue (#0066b1 × 14 px) + white overlay (6 px)
- **Destination pin**: red circle (#e22718) + white center
- **Vehicle position**: blue triangle (#1c69d4) inside semi-transparent circle

Overlays:
- **Maneuver card** (top): white icon box (64×64) + distance (38 px numeral) + street name + lane assist
- **ETA card** (bottom-left): arrival time (30 px numeral) + minutes + km + traffic status
- **Speed limit roundel** (bottom-right): circular white with red border, numeral
- **Speed badge** (top-right): current speed

### 3.3 Screen 2 — Navigation (Route Search)

- Left panel (320 px): search input + 3 route options with time/distance/traffic
- Right panel: map with 3 route paths (selected in white, others dark)
- Route option selected: `border-left: 3px solid #fff; background: #1a1a1a`
- CTA button: full-width white button "ROUTE STARTEN →"

### 3.4 Screen 3 — Vehicle Info

- Left (1.4fr): top-down SVG car silhouette + tire pressure overlays
  - Tires: FL, FR, RL, RR with bar value + temperature
  - Low pressure (< 2.2 bar): red color
- Right (1fr): fluid status list (Oil, Coolant, Fuel, Brake)
  - Per-fluid: label + value + status badge + 24-cell bar
  - Service next row

### 3.5 Screen 4 — M Drive Setup

- Header: "M DRIVE" + M stripe (3 px, 200 px wide)
- Mode selector: 4 tabs (COMFORT / SPORT / SPORT+ / TRACK) — active = white bg, black text
- Settings grid (3×2): MOTOR / FAHRWERK / LENKUNG / DSC / M xDRIVE / BREMSE
  - Each cell: dark bg, caption + value
  - DSC=OFF in track mode: accent red
- CTA row: M1 SPEICHERN (outline) + M2 AKTIV (filled white)

---

## 4. Implementation Checklist

### Cluster App (apps/cluster/)

- [x] BMW M color constants (kBgCanvas, kMBlueLight, kMRed, etc.)
- [x] M tricolor stripe divider widget
- [x] `RpmBarWidget` — discrete cell RPM bar
- [x] `FuelBarWidget` — discrete cell fuel bar
- [x] Layout B — Center Speed redesign in `MainWindow`
- [x] Speed color thresholds (white → amber → red)
- [x] Gear display (N=white, 1-6=M blue, R=red)
- [x] Right panel: IGN/ENG/LIGHT status + Coolant + Oil Pressure
- [x] Status bar: clock + CAN + version
- [x] `GaugeWidget` BMW M colors
- [x] `ArcGaugeWidget` BMW M colors
- [x] `IndicatorWidget` BMW M colors (background, dim, active colors)
- [ ] Layout A — Classic Dual Dial (GaugeWidget-based, toggle)
- [ ] Layout C — Minimal Digital
- [ ] Layout D — M Track (needs GForceWidget, lap timer)
- [ ] Odometer / trip from CAN data (not currently in protocol)
- [ ] Outside temperature from CAN data
- [ ] NowPlaying strip (requires media integration)
- [ ] Day/Night theme toggle
- [ ] Drive mode toggle (Comfort/Sport/Track via M button or CAN)

### Entertainment App (apps/entertainment/) — NOT YET STARTED

- [ ] Project scaffolding (CMake, Qt6 app)
- [ ] IVIChrome: side rail + status bar
- [ ] EntNav: map SVG + route overlay + overlays
- [ ] EntNavSearch: route list panel + map
- [ ] EntVehicleInfo: car silhouette + tire overlays + fluid bars
- [ ] EntMDrive: mode selector + settings grid

---

## 5. Open Questions / Design Decisions

| # | Question | Decision |
|---|----------|----------|
| 1 | Which layout variant to ship by default? | **Layout B** (Center Speed) — most distinctive |
| 2 | Layout switcher via hardware button? | ⬜ TBD — could use a GPIO or CAN message |
| 3 | Entertainment app: separate process or same process? | ⬜ TBD — recommend separate for isolation |
| 4 | Outside temperature source? | ⬜ Need new CAN ID (suggest 0x502) |
| 5 | Odometer / trip data source? | ⬜ Need new CAN ID (suggest 0x503) |
| 6 | Font: bundle Inter or rely on system? | System Inter on Arch/RPi; fallback to Noto |
| 7 | Drive mode CAN signal? | ⬜ Need new CAN ID for Comfort/Sport/Track |

---

## 6. Design Files

All original Claude Design export files are in `docs/design/`:

| File | Description |
|------|-------------|
| `docs/design/README.md` | Export README (read first) |
| `docs/design/chats/chat1.md` | Full design conversation transcript |
| `docs/design/index.html` | Main design entry point |
| `docs/design/styles.css` | Screen base styles |
| `docs/design/design-system/colors_and_type.css` | BMW M design tokens |
| `docs/design/cluster-shared.jsx` | Shared widgets (RpmBar, Gear, SpeedDial, etc.) |
| `docs/design/cluster-layouts.jsx` | All 4 cluster layouts |
| `docs/design/entertainment.jsx` | All 4 entertainment screens |
| `docs/design/dashboard.jsx` | Full cockpit mockup context |
| `docs/design/app.jsx` | App entry + tweaks panel |
