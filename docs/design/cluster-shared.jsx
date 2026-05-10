// cluster-shared.jsx — shared widgets used across cluster layouts
// Atomic, low-fidelity-friendly building blocks.  All read accent + state from
// props (not context) so they compose freely inside any cluster layout.

const tri = "#0066b1, #1c69d4, #e22718";

// ── RPM Bar ──────────────────────────────────────────────────────────────────
// Sporty-tone signature: discrete cells light up against a black track.
// `rpm` is 0..8000, `redline` defaults at 7000.  `cells` typically 30–60.
function RpmBar({ rpm, max = 8000, redline = 7000, cells = 40, height = 16, vertical = false }) {
  const lit = Math.round((rpm / max) * cells);
  const items = [];
  for (let i = 0; i < cells; i++) {
    const cellRpm = ((i + 1) / cells) * max;
    let cls = "rpm-cell";
    if (i < lit) {
      if (cellRpm > redline) cls += " red";
      else if (cellRpm > redline * 0.85) cls += " warn";
      else cls += " on";
    }
    items.push(<div key={i} className={cls} />);
  }
  return (
    <div className="rpm-bar"
         style={{ flexDirection: vertical ? "column-reverse" : "row",
                  height: vertical ? "100%" : height,
                  width: vertical ? height : "100%" }}>
      {items}
    </div>
  );
}

// ── Gear Indicator ───────────────────────────────────────────────────────────
function Gear({ value = "D", manual = null, size = 96, color = "#fff" }) {
  // value: P/R/N/D ; manual: 1..8 (current gear when in D/S)
  return (
    <div style={{ display: "flex", alignItems: "baseline", gap: 8, color }}>
      <div className="cp-num" style={{ fontSize: size, lineHeight: 0.85 }}>{value}</div>
      {manual != null && (
        <div className="cp-num" style={{ fontSize: size * 0.45, color, opacity: 0.85 }}>
          {manual}
        </div>
      )}
    </div>
  );
}

// ── Speed Dial (round) ───────────────────────────────────────────────────────
function SpeedDial({ speed = 0, max = 320, unit = "km/h", size = 280, accent = "#fff", label = "SPEED" }) {
  const cx = size / 2, cy = size / 2, r = size / 2 - 12;
  const startAng = 135, endAng = 405;          // sweep 270deg
  const span = endAng - startAng;
  const ticks = [];
  const major = 8;
  for (let i = 0; i <= major; i++) {
    const a = startAng + (i / major) * span;
    const rad = (a * Math.PI) / 180;
    const x1 = cx + Math.cos(rad) * (r - 4);
    const y1 = cy + Math.sin(rad) * (r - 4);
    const x2 = cx + Math.cos(rad) * (r - 18);
    const y2 = cy + Math.sin(rad) * (r - 18);
    ticks.push(<line key={i} x1={x1} y1={y1} x2={x2} y2={y2} stroke="#fff" strokeWidth="2" />);
  }
  const minor = 40;
  const minorTicks = [];
  for (let i = 0; i <= minor; i++) {
    if (i % (minor / major) === 0) continue;
    const a = startAng + (i / minor) * span;
    const rad = (a * Math.PI) / 180;
    const x1 = cx + Math.cos(rad) * (r - 4);
    const y1 = cy + Math.sin(rad) * (r - 4);
    const x2 = cx + Math.cos(rad) * (r - 11);
    const y2 = cy + Math.sin(rad) * (r - 11);
    minorTicks.push(<line key={i} x1={x1} y1={y1} x2={x2} y2={y2} stroke="#3c3c3c" strokeWidth="1" />);
  }
  // arc filled to current speed
  const fillFrac = Math.min(1, speed / max);
  const aEnd = startAng + fillFrac * span;
  const arcPath = describeArc(cx, cy, r - 8, startAng, aEnd);

  // tick numbers
  const labels = [];
  for (let i = 0; i <= major; i++) {
    const a = startAng + (i / major) * span;
    const rad = (a * Math.PI) / 180;
    const lx = cx + Math.cos(rad) * (r - 36);
    const ly = cy + Math.sin(rad) * (r - 36);
    const v = Math.round((i / major) * max);
    labels.push(
      <text key={i} x={lx} y={ly + 5} fill="#7e7e7e" fontSize="13"
            textAnchor="middle" fontFamily="Inter, sans-serif" fontWeight="700">{v}</text>
    );
  }

  return (
    <div style={{ position: "relative", width: size, height: size }}>
      <svg width={size} height={size}>
        {minorTicks}
        {ticks}
        {labels}
        {fillFrac > 0 && (
          <path d={arcPath} stroke={accent} strokeWidth="3" fill="none" />
        )}
      </svg>
      <div style={{
        position: "absolute", inset: 0, display: "flex",
        flexDirection: "column", alignItems: "center", justifyContent: "center"
      }}>
        <div className="cp-num" style={{ fontSize: size * 0.32, lineHeight: 1, color: "#fff" }}>{speed}</div>
        <div className="cp-cap" style={{ fontSize: 11, color: "#7e7e7e", marginTop: 4 }}>{unit}</div>
        <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", marginTop: 2, opacity: 0.6 }}>{label}</div>
      </div>
    </div>
  );
}

// ── Round RPM Dial (mirror of SpeedDial) ─────────────────────────────────────
function RpmDial({ rpm = 0, max = 8000, redline = 7000, size = 280, label = "RPM × 1000" }) {
  const cx = size / 2, cy = size / 2, r = size / 2 - 12;
  const startAng = 135, endAng = 405;
  const span = endAng - startAng;
  const ticks = [];
  const major = 8;
  for (let i = 0; i <= major; i++) {
    const a = startAng + (i / major) * span;
    const rad = (a * Math.PI) / 180;
    const x1 = cx + Math.cos(rad) * (r - 4);
    const y1 = cy + Math.sin(rad) * (r - 4);
    const x2 = cx + Math.cos(rad) * (r - 18);
    const y2 = cy + Math.sin(rad) * (r - 18);
    const isRed = i >= (redline / max) * major;
    ticks.push(<line key={i} x1={x1} y1={y1} x2={x2} y2={y2} stroke={isRed ? "#e22718" : "#fff"} strokeWidth="2" />);
  }
  const labels = [];
  for (let i = 0; i <= major; i++) {
    const a = startAng + (i / major) * span;
    const rad = (a * Math.PI) / 180;
    const lx = cx + Math.cos(rad) * (r - 36);
    const ly = cy + Math.sin(rad) * (r - 36);
    const v = i;
    const isRed = i >= (redline / max) * major;
    labels.push(
      <text key={i} x={lx} y={ly + 5} fill={isRed ? "#e22718" : "#7e7e7e"} fontSize="13"
            textAnchor="middle" fontFamily="Inter, sans-serif" fontWeight="700">{v}</text>
    );
  }
  // redline arc
  const aRedlineStart = startAng + (redline / max) * span;
  const redArcPath = describeArc(cx, cy, r - 8, aRedlineStart, endAng);
  // current rpm arc
  const fillFrac = Math.min(1, rpm / max);
  const aEnd = startAng + fillFrac * span;
  const arcPath = describeArc(cx, cy, r - 8, startAng, aEnd);
  const overRed = rpm > redline;

  return (
    <div style={{ position: "relative", width: size, height: size }}>
      <svg width={size} height={size}>
        {ticks}
        {labels}
        <path d={redArcPath} stroke="#e22718" strokeWidth="2" fill="none" opacity="0.5" />
        {fillFrac > 0 && (
          <path d={arcPath} stroke={overRed ? "#e22718" : "#fff"} strokeWidth="3" fill="none" />
        )}
      </svg>
      <div style={{
        position: "absolute", inset: 0, display: "flex",
        flexDirection: "column", alignItems: "center", justifyContent: "center"
      }}>
        <div className="cp-num" style={{ fontSize: size * 0.28, lineHeight: 1, color: overRed ? "#e22718" : "#fff" }}>
          {(rpm / 1000).toFixed(1)}
        </div>
        <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e", marginTop: 4, opacity: 0.7 }}>{label}</div>
      </div>
    </div>
  );
}

// ── arc helper ───────────────────────────────────────────────────────────────
function polarToCart(cx, cy, r, deg) {
  const rad = (deg * Math.PI) / 180;
  return { x: cx + r * Math.cos(rad), y: cy + r * Math.sin(rad) };
}
function describeArc(cx, cy, r, start, end) {
  if (end - start < 0.01) return "";
  const s = polarToCart(cx, cy, r, end);
  const e = polarToCart(cx, cy, r, start);
  const large = end - start <= 180 ? "0" : "1";
  return `M ${e.x} ${e.y} A ${r} ${r} 0 ${large} 1 ${s.x} ${s.y}`;
}

// ── Fuel / battery / temp bar ────────────────────────────────────────────────
function FuelBar({ pct = 76, label = "FUEL", range = "412 km", icon = "fuel" }) {
  const ticks = 16;
  const lit = Math.round((pct / 100) * ticks);
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 4 }}>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "center" }}>
        <div className="cp-cap" style={{ fontSize: 10, color: "#7e7e7e" }}>{label}</div>
        <div className="cp-num" style={{ fontSize: 13, color: "#fff" }}>{pct}%</div>
      </div>
      <div style={{ display: "flex", gap: 2, height: 6 }}>
        {Array.from({ length: ticks }).map((_, i) => (
          <div key={i} style={{
            flex: 1,
            background: i < lit ? (i < 2 ? "#e22718" : "#fff") : "#262626",
          }} />
        ))}
      </div>
      <div className="cp-mono" style={{ fontSize: 11, color: "#7e7e7e" }}>{range}</div>
    </div>
  );
}

// ── Tire pressure quad (4 corners) ───────────────────────────────────────────
function TirePressure({ values = [2.4, 2.4, 2.5, 2.5], unit = "bar", size = 76 }) {
  // values: [FL, FR, RL, RR]
  return (
    <div style={{ position: "relative", width: size, height: size * 1.45 }}>
      {/* car silhouette */}
      <svg width={size} height={size * 1.45} viewBox="0 0 60 88" style={{ position: "absolute", inset: 0 }}>
        <rect x="14" y="6" width="32" height="76" rx="2" fill="none" stroke="#3c3c3c" strokeWidth="1" />
        <rect x="20" y="14" width="20" height="22" fill="none" stroke="#3c3c3c" strokeWidth="1" />
        <rect x="20" y="50" width="20" height="22" fill="none" stroke="#3c3c3c" strokeWidth="1" />
      </svg>
      {/* tire pressure values */}
      {values.map((v, i) => {
        const positions = [
          { top: 4, left: -6 },
          { top: 4, right: -6 },
          { bottom: 4, left: -6 },
          { bottom: 4, right: -6 },
        ];
        const low = v < 2.2;
        return (
          <div key={i} style={{
            position: "absolute", ...positions[i],
            display: "flex", flexDirection: "column", alignItems: "center"
          }}>
            <div className="cp-num" style={{ fontSize: 13, color: low ? "#e22718" : "#fff", lineHeight: 1 }}>
              {v.toFixed(1)}
            </div>
            <div className="cp-cap" style={{ fontSize: 7, color: "#7e7e7e", marginTop: 1 }}>{unit}</div>
          </div>
        );
      })}
    </div>
  );
}

// ── Now Playing strip (current media) ────────────────────────────────────────
function NowPlaying({ title = "RUN", artist = "AWOLNATION", source = "SPOTIFY", elapsed = "1:42", total = "4:08", compact = false }) {
  const elapsedFrac = 1.7 / 4.13;
  return (
    <div style={{ display: "flex", alignItems: "center", gap: compact ? 10 : 14 }}>
      <div style={{
        width: compact ? 40 : 52, height: compact ? 40 : 52, flexShrink: 0,
        background: "linear-gradient(135deg, #1c69d4 0%, #0066b1 100%)",
        position: "relative", overflow: "hidden"
      }}>
        <svg width="100%" height="100%" viewBox="0 0 52 52">
          <circle cx="26" cy="26" r="6" fill="#000" opacity="0.6" />
          <circle cx="26" cy="26" r="2" fill="#fff" />
        </svg>
      </div>
      <div style={{ flex: 1, minWidth: 0, display: "flex", flexDirection: "column", gap: 3 }}>
        <div style={{ display: "flex", justifyContent: "space-between", gap: 8 }}>
          <div className="cp-cap" style={{ fontSize: 13, color: "#fff", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>
            {title}
          </div>
          <div className="cp-mono" style={{ fontSize: 10, color: "#7e7e7e", flexShrink: 0 }}>
            {elapsed} / {total}
          </div>
        </div>
        <div style={{ fontSize: 11, color: "#bbbbbb", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap", fontWeight: 300 }}>
          {artist}
        </div>
        {!compact && (
          <div style={{ height: 2, background: "#262626", marginTop: 2 }}>
            <div style={{ width: `${elapsedFrac * 100}%`, height: "100%", background: "#fff" }} />
          </div>
        )}
        {!compact && (
          <div className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e", letterSpacing: "1.5px" }}>{source}</div>
        )}
      </div>
    </div>
  );
}

// ── Outside temp ─────────────────────────────────────────────────────────────
function OutsideTemp({ value = 18, unit = "°C", label = "OUTSIDE" }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "flex-end" }}>
      <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>{label}</div>
      <div className="cp-num" style={{ fontSize: 22, color: "#fff", lineHeight: 1.1 }}>
        {value}<span style={{ fontSize: 13, color: "#7e7e7e", marginLeft: 2 }}>{unit}</span>
      </div>
    </div>
  );
}

// ── Status row (top of cluster: time, signal, etc) ───────────────────────────
function StatusRow({ time = "14:28", showWarnings = false }) {
  return (
    <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", padding: "10px 20px", color: "#7e7e7e" }}>
      <div style={{ display: "flex", alignItems: "center", gap: 14 }}>
        <span className="cp-num" style={{ fontSize: 13, color: "#fff" }}>{time}</span>
        {/* signal bars */}
        <svg width="14" height="10" viewBox="0 0 14 10" fill="currentColor">
          <rect x="0" y="7" width="2" height="3" />
          <rect x="3" y="5" width="2" height="5" />
          <rect x="6" y="3" width="2" height="7" />
          <rect x="9" y="1" width="2" height="9" opacity="0.3" />
        </svg>
        <span className="cp-cap" style={{ fontSize: 9 }}>4G</span>
      </div>
      <div style={{ display: "flex", alignItems: "center", gap: 14 }}>
        {showWarnings && (
          <span style={{ display: "flex", alignItems: "center", gap: 4, color: "#f4b400" }}>
            <svg width="12" height="12" viewBox="0 0 12 12" fill="currentColor">
              <path d="M6 0 12 11 0 11Z" />
              <rect x="5.4" y="4" width="1.2" height="4" fill="#000" />
              <rect x="5.4" y="9" width="1.2" height="1.2" fill="#000" />
            </svg>
            <span className="cp-cap" style={{ fontSize: 9 }}>SVC DUE</span>
          </span>
        )}
        <span className="cp-cap" style={{ fontSize: 9 }}>BMW M</span>
      </div>
    </div>
  );
}

// ── Drive mode chip ─────────────────────────────────────────────────────────
function DriveModeChip({ mode = "SPORT" }) {
  const colorMap = { COMFORT: "#1c69d4", SPORT: "#fff", "SPORT+": "#e22718", TRACK: "#e22718" };
  const fg = colorMap[mode] || "#fff";
  return (
    <div style={{
      display: "inline-flex", alignItems: "center", gap: 6,
      border: `1px solid ${fg}`, padding: "4px 10px", color: fg
    }}>
      <span style={{ width: 8, height: 8, background: fg }} />
      <span className="cp-cap" style={{ fontSize: 10 }}>{mode}</span>
    </div>
  );
}

Object.assign(window, {
  RpmBar, Gear, SpeedDial, RpmDial, FuelBar, TirePressure,
  NowPlaying, OutsideTemp, StatusRow, DriveModeChip,
});
