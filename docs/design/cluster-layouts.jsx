// cluster-layouts.jsx — four cluster application variants, each at 800x480.
//
// Each layout accepts a `state` prop ("idle" | "drive" | "track") that
// determines speed, RPM, gear, and surface metrics (lap time on track, etc).
// Theme + drive-mode are read from data-* attributes on the screen wrapper
// (set by the App from tweaks).

// ── State presets ────────────────────────────────────────────────────────────
const STATES = {
  idle: {
    speed: 0, rpm: 800, gear: "P", manualGear: null,
    odo: "12,847 km", trip: "0.0 km",
    fuel: 76, range: "412 km", oilTemp: 92, coolant: 88, outsideTemp: 14,
    tires: [2.4, 2.4, 2.5, 2.5],
    label: "STATIONARY", warn: false,
  },
  drive: {
    speed: 87, rpm: 3400, gear: "D", manualGear: 4,
    odo: "12,894 km", trip: "47.3 km",
    fuel: 68, range: "362 km", oilTemp: 102, coolant: 96, outsideTemp: 18,
    tires: [2.4, 2.4, 2.5, 2.5],
    label: "DRIVING", warn: false,
  },
  track: {
    speed: 218, rpm: 7250, gear: "S", manualGear: 5,
    odo: "13,028 km", trip: "94.6 km",
    fuel: 42, range: "189 km", oilTemp: 118, coolant: 104, outsideTemp: 22,
    tires: [2.5, 2.5, 2.7, 2.7],
    lapCurrent: "1:38.42", lapBest: "1:36.18", lapDelta: "+2.24",
    sector: 2, gforceLat: 0.92, gforceLong: -1.18,
    label: "TRACK · NÜRBURGRING", warn: true,
  },
};

// ───────────────────────────────────────────────────────────────────────────────
// Layout A — Classic Dual Dial (RPM left, Speed right)
// ───────────────────────────────────────────────────────────────────────────────
function ClusterClassic({ state = "drive", theme = "night", mode = "sport" }) {
  const s = STATES[state];
  return (
    <div className="cp-screen" data-theme={theme} data-mode={mode}>
      <StatusRow time="14:28" showWarnings={s.warn} />
      <div className="cp-mstripe" style={{ height: 2 }} />

      {/* Two big dials */}
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center", padding: "20px 32px 0" }}>
        <RpmDial rpm={s.rpm} max={8000} redline={7000} size={260} />
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 18 }}>
          <Gear value={s.gear} manual={s.manualGear} size={88} />
          <DriveModeChip mode={mode === "comfort" ? "COMFORT" : mode === "sport" ? "SPORT" : "TRACK"} />
        </div>
        <SpeedDial speed={s.speed} max={300} size={260} accent="#fff" />
      </div>

      {/* Bottom strip: trip, fuel, temp, media */}
      <div style={{
        position: "absolute", bottom: 0, left: 0, right: 0,
        borderTop: "1px solid #262626", padding: "12px 24px",
        display: "grid", gridTemplateColumns: "1fr 1fr 1.6fr 1fr", gap: 24, alignItems: "center"
      }}>
        <div>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>ODOMETER</div>
          <div className="cp-num" style={{ fontSize: 16, color: "#fff" }}>{s.odo}</div>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", marginTop: 6 }}>TRIP</div>
          <div className="cp-num" style={{ fontSize: 13, color: "#bbbbbb" }}>{s.trip}</div>
        </div>
        <FuelBar pct={s.fuel} range={s.range} />
        <div style={{ borderLeft: "1px solid #262626", paddingLeft: 16 }}>
          <NowPlaying compact />
        </div>
        <OutsideTemp value={s.outsideTemp} />
      </div>
    </div>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Layout B — Center Speed (single big speedo, RPM bar above, side widgets)
// ───────────────────────────────────────────────────────────────────────────────
function ClusterCenter({ state = "drive", theme = "night", mode = "sport" }) {
  const s = STATES[state];
  return (
    <div className="cp-screen" data-theme={theme} data-mode={mode}>
      <StatusRow time="14:28" showWarnings={s.warn} />

      {/* Top — RPM bar across full width */}
      <div style={{ padding: "8px 32px 0" }}>
        <div style={{ display: "flex", justifyContent: "space-between", marginBottom: 4 }}>
          <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>RPM × 1000</span>
          <span className="cp-num" style={{ fontSize: 12, color: "#fff" }}>{(s.rpm / 1000).toFixed(1)}</span>
        </div>
        <div style={{ display: "flex", gap: 8, alignItems: "center" }}>
          <span className="cp-num" style={{ fontSize: 10, color: "#7e7e7e", width: 14 }}>0</span>
          <div style={{ flex: 1 }}>
            <RpmBar rpm={s.rpm} max={8000} redline={7000} cells={48} height={20} />
          </div>
          <span className="cp-num" style={{ fontSize: 10, color: "#e22718", width: 14, textAlign: "right" }}>8</span>
        </div>
      </div>

      {/* Middle — left widgets, big center number, right widgets */}
      <div style={{ display: "grid", gridTemplateColumns: "1fr 2fr 1fr", padding: "16px 32px 0", alignItems: "center" }}>
        <div style={{ display: "flex", flexDirection: "column", gap: 18 }}>
          <div>
            <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>GEAR</div>
            <Gear value={s.gear} manual={s.manualGear} size={64} />
          </div>
          <FuelBar pct={s.fuel} range={s.range} />
        </div>

        <div style={{ display: "flex", flexDirection: "column", alignItems: "center" }}>
          <div className="cp-num" style={{ fontSize: 200, color: "#fff", lineHeight: 0.85, letterSpacing: "-0.05em" }}>
            {s.speed}
          </div>
          <div className="cp-cap" style={{ fontSize: 11, color: "#7e7e7e", marginTop: 4 }}>KM / H</div>
        </div>

        <div style={{ display: "flex", flexDirection: "column", gap: 14, alignItems: "flex-end" }}>
          <DriveModeChip mode={mode === "comfort" ? "COMFORT" : mode === "sport" ? "SPORT" : "TRACK"} />
          <OutsideTemp value={s.outsideTemp} />
          <div style={{ textAlign: "right" }}>
            <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>OIL TEMP</div>
            <div className="cp-num" style={{ fontSize: 16, color: s.oilTemp > 110 ? "#f4b400" : "#fff" }}>
              {s.oilTemp}°C
            </div>
          </div>
        </div>
      </div>

      {/* Bottom — media + odo */}
      <div style={{
        position: "absolute", bottom: 0, left: 0, right: 0,
        borderTop: "1px solid #262626", padding: "10px 24px",
        display: "grid", gridTemplateColumns: "1fr 1fr", gap: 20, alignItems: "center"
      }}>
        <NowPlaying compact />
        <div style={{ textAlign: "right" }}>
          <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", marginRight: 8 }}>ODO</span>
          <span className="cp-num" style={{ fontSize: 14, color: "#fff" }}>{s.odo}</span>
          <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", margin: "0 8px 0 16px" }}>TRIP</span>
          <span className="cp-num" style={{ fontSize: 14, color: "#bbbbbb" }}>{s.trip}</span>
        </div>
      </div>
    </div>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Layout C — Minimal Digital (no dials, big numbers, dense info)
// ───────────────────────────────────────────────────────────────────────────────
function ClusterMinimal({ state = "drive", theme = "night", mode = "sport" }) {
  const s = STATES[state];
  return (
    <div className="cp-screen" data-theme={theme} data-mode={mode}>
      <StatusRow time="14:28" showWarnings={s.warn} />

      {/* Massive split: Speed | Gear | RPM */}
      <div style={{
        display: "grid", gridTemplateColumns: "1.6fr 0.7fr 1.2fr",
        padding: "8px 36px 0", gap: 20, alignItems: "flex-start"
      }}>
        <div>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", letterSpacing: "1.5px" }}>SPEED · KM/H</div>
          <div className="cp-num" style={{ fontSize: 168, color: "#fff", lineHeight: 0.85, letterSpacing: "-0.05em" }}>
            {s.speed}
          </div>
        </div>
        <div style={{ borderLeft: "1px solid #262626", paddingLeft: 18 }}>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>GEAR</div>
          <Gear value={s.gear} manual={s.manualGear} size={120} />
        </div>
        <div style={{ borderLeft: "1px solid #262626", paddingLeft: 18 }}>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>RPM</div>
          <div className="cp-num" style={{ fontSize: 56, color: s.rpm > 7000 ? "#e22718" : "#fff", lineHeight: 1 }}>
            {s.rpm.toLocaleString()}
          </div>
          <div style={{ height: 10, marginTop: 6 }}>
            <RpmBar rpm={s.rpm} max={8000} redline={7000} cells={32} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", marginTop: 4 }}>
            <span className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>0</span>
            <span className="cp-cap" style={{ fontSize: 8, color: "#e22718" }}>8K</span>
          </div>
        </div>
      </div>

      {/* Mid row of metrics */}
      <div style={{
        display: "grid", gridTemplateColumns: "repeat(4, 1fr)",
        padding: "10px 36px", gap: 18, marginTop: 10
      }}>
        <Metric label="MODE" value={mode.toUpperCase()} accent={mode === "track" ? "#e22718" : "#fff"} />
        <Metric label="OUTSIDE" value={`${s.outsideTemp}°`} />
        <Metric label="OIL" value={`${s.oilTemp}°C`} accent={s.oilTemp > 110 ? "#f4b400" : "#fff"} />
        <Metric label="RANGE" value={s.range} />
      </div>

      {/* Bottom — fuel + media + tire */}
      <div style={{
        position: "absolute", bottom: 0, left: 0, right: 0,
        borderTop: "1px solid #262626", padding: "10px 28px",
        display: "grid", gridTemplateColumns: "1.4fr 0.6fr 1.4fr", gap: 20, alignItems: "center"
      }}>
        <FuelBar pct={s.fuel} range={s.range} />
        <div style={{ display: "flex", justifyContent: "center", borderLeft: "1px solid #262626", borderRight: "1px solid #262626", padding: "0 10px" }}>
          <TirePressure values={s.tires} size={48} />
        </div>
        <NowPlaying compact />
      </div>
    </div>
  );
}

function Metric({ label, value, accent = "#fff" }) {
  return (
    <div>
      <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>{label}</div>
      <div className="cp-num" style={{ fontSize: 22, color: accent, lineHeight: 1.2 }}>{value}</div>
    </div>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Layout D — M Track (RPM bar on top, lap time + g-force, race tone)
// ───────────────────────────────────────────────────────────────────────────────
function ClusterMTrack({ state = "track", theme = "night", mode = "track" }) {
  const s = STATES[state];
  const isTrack = state === "track";

  return (
    <div className="cp-screen" data-theme={theme} data-mode={mode} style={{ background: "#000" }}>
      {/* Massive RPM bar across the very top */}
      <div style={{ height: 36, padding: "8px 16px 0" }}>
        <RpmBar rpm={s.rpm} max={8000} redline={7000} cells={64} height={28} />
      </div>
      <div style={{ display: "flex", justifyContent: "space-between", padding: "2px 16px 0" }}>
        <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>0</span>
        <span className="cp-num" style={{ fontSize: 14, color: s.rpm > 7000 ? "#e22718" : "#fff" }}>
          {s.rpm.toLocaleString()} RPM
        </span>
        <span className="cp-cap" style={{ fontSize: 9, color: "#e22718" }}>REDLINE 7000</span>
      </div>
      <div className="cp-mstripe" style={{ height: 3, marginTop: 6 }} />

      {/* Center — speed mega + gear */}
      <div style={{ display: "grid", gridTemplateColumns: "1fr 1.4fr 1fr", padding: "14px 24px 0", alignItems: "flex-start" }}>
        {/* Left — lap time block */}
        <div style={{ borderRight: "1px solid #262626", paddingRight: 16 }}>
          <div className="cp-cap" style={{ fontSize: 9, color: "#e22718" }}>LAP {isTrack ? "12" : "—"}</div>
          <div className="cp-num cp-mono" style={{ fontSize: 30, color: "#fff", lineHeight: 1.1, marginTop: 2 }}>
            {isTrack ? s.lapCurrent : "—:——.——"}
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", marginTop: 8 }}>
            <div>
              <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>BEST</div>
              <div className="cp-num cp-mono" style={{ fontSize: 14, color: "#bbb" }}>
                {isTrack ? s.lapBest : "—"}
              </div>
            </div>
            <div>
              <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>DELTA</div>
              <div className="cp-num cp-mono" style={{ fontSize: 14, color: isTrack ? "#e22718" : "#7e7e7e" }}>
                {isTrack ? s.lapDelta : "—"}
              </div>
            </div>
          </div>
          {isTrack && (
            <div style={{ marginTop: 10 }}>
              <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>SECTOR</div>
              <div style={{ display: "flex", gap: 4, marginTop: 4 }}>
                {[1, 2, 3].map((n) => (
                  <div key={n} style={{
                    flex: 1, height: 4,
                    background: n < s.sector ? "#0fa336" : n === s.sector ? "#f4b400" : "#262626",
                  }} />
                ))}
              </div>
            </div>
          )}
        </div>

        {/* Center — speed */}
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center" }}>
          <div className="cp-num" style={{ fontSize: 168, color: "#fff", lineHeight: 0.82, letterSpacing: "-0.06em" }}>
            {s.speed}
          </div>
          <div className="cp-cap" style={{ fontSize: 11, color: "#7e7e7e", marginTop: 2 }}>KM / H</div>
          <div style={{ display: "flex", gap: 18, marginTop: 8 }}>
            <Gear value={s.gear} manual={s.manualGear} size={42} />
            <DriveModeChip mode="TRACK" />
          </div>
        </div>

        {/* Right — G-force compass */}
        <div style={{ borderLeft: "1px solid #262626", paddingLeft: 16 }}>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>G-FORCE</div>
          <GForce lat={isTrack ? s.gforceLat : 0} long={isTrack ? s.gforceLong : 0} size={120} />
          <div style={{ display: "flex", justifyContent: "space-between", marginTop: 6 }}>
            <div>
              <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>LAT</div>
              <div className="cp-num cp-mono" style={{ fontSize: 13, color: "#fff" }}>
                {isTrack ? s.gforceLat.toFixed(2) : "0.00"}
              </div>
            </div>
            <div>
              <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>LONG</div>
              <div className="cp-num cp-mono" style={{ fontSize: 13, color: "#fff" }}>
                {isTrack ? s.gforceLong.toFixed(2) : "0.00"}
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* Bottom — telemetry strip */}
      <div style={{
        position: "absolute", bottom: 0, left: 0, right: 0,
        borderTop: "1px solid #262626", padding: "8px 20px",
        display: "grid", gridTemplateColumns: "repeat(5, 1fr)", gap: 12, alignItems: "center"
      }}>
        <Metric label="OIL" value={`${s.oilTemp}°`} accent={s.oilTemp > 110 ? "#f4b400" : "#fff"} />
        <Metric label="WATER" value={`${s.coolant}°`} />
        <div>
          <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>FUEL</div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 4 }}>
            <span className="cp-num" style={{ fontSize: 18, color: s.fuel < 25 ? "#f4b400" : "#fff" }}>{s.fuel}</span>
            <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>%</span>
          </div>
          <div style={{ display: "flex", gap: 1, height: 3, marginTop: 2 }}>
            {Array.from({ length: 12 }).map((_, i) => (
              <div key={i} style={{ flex: 1, background: i < (s.fuel / 100) * 12 ? "#fff" : "#262626" }} />
            ))}
          </div>
        </div>
        <Metric label="OUTSIDE" value={`${s.outsideTemp}°`} />
        <div style={{ textAlign: "right" }}>
          <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>{s.label}</div>
          <div className="cp-num cp-mono" style={{ fontSize: 12, color: "#bbb", marginTop: 2 }}>
            {s.trip}
          </div>
        </div>
      </div>
    </div>
  );
}

function GForce({ lat = 0, long = 0, size = 120 }) {
  // lat: -1 (left) to +1 (right), long: -1 (decel/braking) to +1 (accel)
  const cx = size / 2, cy = size / 2;
  const max = 1.5;
  const x = cx + (lat / max) * (size / 2 - 14);
  const y = cy - (long / max) * (size / 2 - 14);
  return (
    <svg width={size} height={size} viewBox={`0 0 ${size} ${size}`}>
      {/* concentric rings */}
      {[1, 2, 3].map((n) => (
        <circle key={n} cx={cx} cy={cy} r={(size / 2 - 6) * (n / 3)} fill="none" stroke="#262626" strokeWidth="1" />
      ))}
      <line x1="6" y1={cy} x2={size - 6} y2={cy} stroke="#262626" strokeWidth="1" />
      <line x1={cx} y1="6" x2={cx} y2={size - 6} stroke="#262626" strokeWidth="1" />
      {/* dot */}
      <circle cx={x} cy={y} r="6" fill="#e22718" />
      <circle cx={x} cy={y} r="3" fill="#fff" />
      {/* trail dots */}
      <circle cx={x - (lat * 4)} cy={y + (long * 4)} r="3" fill="#e22718" opacity="0.4" />
    </svg>
  );
}

Object.assign(window, { ClusterClassic, ClusterCenter, ClusterMinimal, ClusterMTrack, STATES });
