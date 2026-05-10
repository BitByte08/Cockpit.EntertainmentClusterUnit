// entertainment.jsx — Entertainment / IVI app screens (800x480)
// Focus: full-screen navigation. Plus Vehicle Info screen.

// ── Entertainment chrome (top status + side rail) ────────────────────────────
function IVIChrome({ children, theme = "night", mode = "sport", activeApp = "nav" }) {
  return (
    <div className="cp-screen" data-theme={theme} data-mode={mode}>
      {/* Side rail — vertical M stripe + app icons */}
      <div style={{
        position: "absolute", top: 0, bottom: 0, left: 0, width: 56,
        background: "#000",
        borderRight: "1px solid #262626",
        display: "flex", flexDirection: "column", alignItems: "center",
        padding: "10px 0",
        zIndex: 5,
      }}>
        <div className="cp-mstripe-vert" style={{ width: 3, height: 28, marginBottom: 14 }} />
        {[
          { id: "nav",   icon: <NavIcon /> },
          { id: "media", icon: <MediaIcon /> },
          { id: "phone", icon: <PhoneIcon /> },
          { id: "car",   icon: <CarIcon /> },
          { id: "drive", icon: <MIcon /> },
        ].map((a) => (
          <button key={a.id} style={{
            width: 40, height: 40, marginBottom: 6,
            background: activeApp === a.id ? "#1a1a1a" : "transparent",
            border: activeApp === a.id ? "1px solid #fff" : "1px solid transparent",
            borderRadius: 0, color: activeApp === a.id ? "#fff" : "#7e7e7e",
            display: "flex", alignItems: "center", justifyContent: "center",
            cursor: "pointer", padding: 0,
          }}>
            {a.icon}
          </button>
        ))}
        <div style={{ flex: 1 }} />
        <button style={{
          width: 40, height: 40, background: "transparent", border: "none",
          color: "#7e7e7e", cursor: "pointer", padding: 0,
        }}>
          <SettingsIcon />
        </button>
      </div>

      {/* Top status bar */}
      <div style={{
        position: "absolute", top: 0, left: 56, right: 0, height: 28,
        background: "rgba(0,0,0,0.6)", backdropFilter: "blur(8px)",
        display: "flex", alignItems: "center", justifyContent: "space-between",
        padding: "0 16px", zIndex: 4, borderBottom: "1px solid #262626",
      }}>
        <div style={{ display: "flex", alignItems: "center", gap: 12, color: "#fff" }}>
          <span className="cp-num" style={{ fontSize: 12 }}>14:28</span>
          <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>WED · 8 MAY</span>
        </div>
        <div style={{ display: "flex", alignItems: "center", gap: 14, color: "#fff" }}>
          <span className="cp-cap" style={{ fontSize: 9 }}>18°</span>
          <span className="cp-cap" style={{ fontSize: 9, color: "#0fa336" }}>● BT</span>
          <span className="cp-cap" style={{ fontSize: 9 }}>4G</span>
          <DriveModeChip mode={mode === "comfort" ? "COMFORT" : mode === "sport" ? "SPORT" : "TRACK"} />
        </div>
      </div>

      {/* Content area */}
      <div style={{ position: "absolute", top: 28, bottom: 0, left: 56, right: 0 }}>
        {children}
      </div>
    </div>
  );
}

// ── Icons (1.5px stroke, square caps — BMW M discipline) ─────────────────────
const stroke = { stroke: "currentColor", strokeWidth: 1.5, strokeLinecap: "square", strokeLinejoin: "miter", fill: "none" };
const NavIcon = () => (
  <svg width="22" height="22" viewBox="0 0 22 22" {...stroke}>
    <path d="M3 18 L11 3 L19 18 L11 14 Z" />
  </svg>
);
const MediaIcon = () => (
  <svg width="22" height="22" viewBox="0 0 22 22" {...stroke}>
    <circle cx="11" cy="11" r="8" />
    <circle cx="11" cy="11" r="2" />
  </svg>
);
const PhoneIcon = () => (
  <svg width="22" height="22" viewBox="0 0 22 22" {...stroke}>
    <path d="M5 4 L9 4 L10.5 8 L8 10 Q10 14 14 14 L16 11.5 L20 13 L20 17 Q20 19 18 19 Q9 19 5 13 Q3 9 3 5 Q3 4 5 4 Z" />
  </svg>
);
const CarIcon = () => (
  <svg width="22" height="22" viewBox="0 0 22 22" {...stroke}>
    <path d="M3 12 L4 8 L18 8 L19 12 L19 16 L3 16 Z" />
    <circle cx="6.5" cy="16" r="1.5" />
    <circle cx="15.5" cy="16" r="1.5" />
  </svg>
);
const MIcon = () => (
  <svg width="22" height="22" viewBox="0 0 22 22" fill="none">
    <path d="M3 4 L3 18 M3 4 L7 14 L11 4 L11 18" stroke="currentColor" strokeWidth="1.8" strokeLinecap="square" fill="none" />
    <rect x="14" y="4" width="2" height="14" fill="#0066b1" />
    <rect x="16" y="4" width="2" height="14" fill="#1c69d4" />
    <rect x="18" y="4" width="2" height="14" fill="#e22718" />
  </svg>
);
const SettingsIcon = () => (
  <svg width="20" height="20" viewBox="0 0 20 20" {...stroke}>
    <circle cx="10" cy="10" r="3" />
    <path d="M10 2 L10 4 M10 16 L10 18 M2 10 L4 10 M16 10 L18 10 M4 4 L5.5 5.5 M14.5 14.5 L16 16 M4 16 L5.5 14.5 M14.5 5.5 L16 4" />
  </svg>
);

// ───────────────────────────────────────────────────────────────────────────────
// Entertainment 1 — Navigation (full screen, route active)
// ───────────────────────────────────────────────────────────────────────────────
function EntNav({ theme = "night", mode = "sport", state = "drive" }) {
  return (
    <IVIChrome theme={theme} mode={mode} activeApp="nav">
      <div style={{ position: "absolute", inset: 0, overflow: "hidden" }}>
        {/* Map base */}
        <svg width="100%" height="100%" viewBox="0 0 744 452" preserveAspectRatio="xMidYMid slice"
             style={{ position: "absolute", inset: 0 }}>
          <defs>
            <pattern id="terrain" x="0" y="0" width="744" height="452" patternUnits="userSpaceOnUse">
              <rect width="744" height="452" fill="#0a0e15" />
              <radialGradient id="rg1" cx="0.3" cy="0.4" r="0.7">
                <stop offset="0" stopColor="#1a2230" />
                <stop offset="1" stopColor="#0a0e15" />
              </radialGradient>
              <rect width="744" height="452" fill="url(#rg1)" />
            </pattern>
          </defs>
          <rect width="744" height="452" fill="url(#terrain)" />

          {/* Park polygon */}
          <path d="M 60 280 L 240 240 L 280 360 L 100 400 Z" fill="#0d1810" stroke="#1a2a18" strokeWidth="1" opacity="0.8" />
          {/* Water */}
          <path d="M 480 380 L 740 360 L 740 460 L 480 460 Z" fill="#0a1628" stroke="#0e2238" strokeWidth="1" />

          {/* Minor roads — thin grey */}
          <g stroke="#2a3140" strokeWidth="1.2" fill="none">
            <path d="M 0 90 L 744 110" />
            <path d="M 0 200 L 744 210" />
            <path d="M 0 310 L 744 330" />
            <path d="M 200 0 L 220 452" />
            <path d="M 380 0 L 400 452" />
            <path d="M 560 0 L 580 452" />
            <path d="M 80 0 L 100 452" />
          </g>
          <g stroke="#3a4254" strokeWidth="2" fill="none">
            <path d="M 0 250 L 744 260" />
            <path d="M 460 0 L 480 452" />
          </g>

          {/* Active route — wide white over thicker accent */}
          <path d="M 380 420 Q 380 340 420 300 T 500 240 Q 540 200 540 150 T 580 80 L 660 30"
                stroke="#0066b1" strokeWidth="14" fill="none" strokeLinecap="square" opacity="0.9" />
          <path d="M 380 420 Q 380 340 420 300 T 500 240 Q 540 200 540 150 T 580 80 L 660 30"
                stroke="#fff" strokeWidth="6" fill="none" strokeLinecap="square" />

          {/* Destination pin */}
          <g transform="translate(660, 30)">
            <circle r="14" fill="#e22718" />
            <circle r="6" fill="#fff" />
          </g>

          {/* Vehicle position triangle */}
          <g transform="translate(380, 420) rotate(-15)">
            <circle r="22" fill="rgba(0,102,177,0.22)" />
            <path d="M 0 -14 L 11 9 L 0 4 L -11 9 Z" fill="#1c69d4" stroke="#fff" strokeWidth="1.5" />
          </g>

          {/* Street labels */}
          <text x="120" y="175" fill="#7e7e7e" fontSize="9" fontFamily="Inter" fontWeight="500">KÖNIGSALLEE</text>
          <text x="500" y="345" fill="#7e7e7e" fontSize="9" fontFamily="Inter" fontWeight="500">B1</text>
          <text x="610" y="55" fill="#fff" fontSize="11" fontFamily="Inter" fontWeight="700">ZIEL</text>
        </svg>

        {/* TOP — Next maneuver card */}
        <div style={{
          position: "absolute", top: 12, left: 12, right: 12,
          background: "rgba(0,0,0,0.85)",
          backdropFilter: "blur(12px)",
          border: "1px solid #262626",
          padding: "14px 18px",
          display: "flex", alignItems: "center", gap: 16,
        }}>
          {/* Maneuver icon — sharp left turn arrow */}
          <div style={{
            width: 64, height: 64, background: "#fff",
            display: "flex", alignItems: "center", justifyContent: "center", flexShrink: 0,
          }}>
            <svg width="44" height="44" viewBox="0 0 44 44" fill="none">
              <path d="M30 38 L30 18 L14 18 M14 18 L22 10 M14 18 L22 26"
                    stroke="#000" strokeWidth="3.5" strokeLinecap="square" strokeLinejoin="miter" />
            </svg>
          </div>
          <div style={{ flex: 1 }}>
            <div style={{ display: "flex", alignItems: "baseline", gap: 6 }}>
              <span className="cp-num" style={{ fontSize: 38, color: "#fff", lineHeight: 1 }}>320</span>
              <span className="cp-cap" style={{ fontSize: 12, color: "#7e7e7e" }}>M</span>
            </div>
            <div className="cp-cap" style={{ fontSize: 12, color: "#fff", marginTop: 4 }}>
              LINKS ABBIEGEN · KURFÜRSTENDAMM
            </div>
          </div>
          {/* Lane assist */}
          <div style={{ display: "flex", gap: 4, padding: "0 8px", borderLeft: "1px solid #262626" }}>
            {["str", "left", "left", "str"].map((dir, i) => (
              <div key={i} style={{
                width: 22, height: 36,
                background: dir === "left" ? "#1a1a1a" : "transparent",
                border: "1px solid #3c3c3c",
                display: "flex", alignItems: "center", justifyContent: "center",
              }}>
                <svg width="14" height="18" viewBox="0 0 14 18" fill="none">
                  {dir === "left" ? (
                    <path d="M11 16 L11 9 L4 9 M4 9 L8 5 M4 9 L8 13" stroke={i === 1 ? "#fff" : "#7e7e7e"} strokeWidth="1.5" />
                  ) : (
                    <path d="M7 16 L7 4 M7 4 L4 7 M7 4 L10 7" stroke="#7e7e7e" strokeWidth="1.5" />
                  )}
                </svg>
              </div>
            ))}
          </div>
        </div>

        {/* BOTTOM-LEFT — ETA + arrival */}
        <div style={{
          position: "absolute", bottom: 12, left: 12,
          background: "rgba(0,0,0,0.85)", backdropFilter: "blur(12px)",
          border: "1px solid #262626", padding: "12px 18px",
          minWidth: 240,
        }}>
          <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>ANKUNFT · BERLIN MITTE</div>
          <div style={{ display: "flex", alignItems: "baseline", gap: 14, marginTop: 6 }}>
            <span className="cp-num" style={{ fontSize: 30, color: "#fff", lineHeight: 1 }}>15:14</span>
            <span className="cp-num" style={{ fontSize: 16, color: "#bbb" }}>46 min</span>
          </div>
          <div style={{ display: "flex", gap: 16, marginTop: 8 }}>
            <span className="cp-num" style={{ fontSize: 13, color: "#fff" }}>34<span style={{ fontSize: 10, color: "#7e7e7e", marginLeft: 2 }}>KM</span></span>
            <span className="cp-cap" style={{ fontSize: 10, color: "#0fa336" }}>● FREIE FAHRT</span>
          </div>
          <div className="cp-mstripe" style={{ height: 2, marginTop: 10 }} />
        </div>

        {/* BOTTOM-RIGHT — speed limit + zoom */}
        <div style={{ position: "absolute", bottom: 12, right: 12, display: "flex", flexDirection: "column", gap: 6, alignItems: "flex-end" }}>
          {/* Speed limit roundel */}
          <div style={{
            width: 56, height: 56, borderRadius: "50%",
            background: "#fff", border: "4px solid #e22718",
            display: "flex", alignItems: "center", justifyContent: "center",
          }}>
            <span className="cp-num" style={{ fontSize: 22, color: "#000" }}>80</span>
          </div>
          {/* Zoom control */}
          <div style={{ display: "flex", flexDirection: "column", border: "1px solid #3c3c3c", background: "rgba(0,0,0,0.7)" }}>
            <button style={{ width: 32, height: 32, background: "transparent", border: "none", color: "#fff", fontSize: 16, cursor: "pointer" }}>+</button>
            <div style={{ height: 1, background: "#3c3c3c" }} />
            <button style={{ width: 32, height: 32, background: "transparent", border: "none", color: "#fff", fontSize: 16, cursor: "pointer" }}>−</button>
          </div>
        </div>

        {/* Speed badge top-right */}
        <div style={{
          position: "absolute", top: 92, right: 12,
          background: "rgba(0,0,0,0.85)", border: "1px solid #262626",
          padding: "6px 10px", display: "flex", alignItems: "baseline", gap: 4,
        }}>
          <span className="cp-num" style={{ fontSize: 22, color: "#fff" }}>{STATES[state].speed}</span>
          <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>KM/H</span>
        </div>
      </div>
    </IVIChrome>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Entertainment 2 — Navigation, route overview / search results
// ───────────────────────────────────────────────────────────────────────────────
function EntNavSearch({ theme = "night", mode = "comfort" }) {
  return (
    <IVIChrome theme={theme} mode={mode} activeApp="nav">
      <div style={{ display: "grid", gridTemplateColumns: "320px 1fr", height: "100%" }}>
        {/* Left search panel */}
        <div style={{ background: "#0d0d0d", borderRight: "1px solid #262626", display: "flex", flexDirection: "column" }}>
          <div style={{ padding: "14px 16px", borderBottom: "1px solid #262626" }}>
            <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>SUCHE</div>
            <div style={{
              marginTop: 8, height: 36, border: "1px solid #fff",
              display: "flex", alignItems: "center", padding: "0 12px", gap: 10,
            }}>
              <svg width="14" height="14" viewBox="0 0 14 14" {...stroke}>
                <circle cx="6" cy="6" r="5" />
                <path d="M10 10 L13 13" />
              </svg>
              <span className="cp-cap" style={{ fontSize: 11, color: "#fff" }}>BERLIN</span>
              <span style={{ flex: 1 }} />
              <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>3 ROUTEN</span>
            </div>
          </div>
          {/* Route options */}
          {[
            { name: "SCHNELLSTE", time: "46 min", dist: "34 km", traffic: "FREIE FAHRT", trafficColor: "#0fa336", selected: true },
            { name: "ÖKO", time: "52 min", dist: "31 km", traffic: "−18% VERBR.", trafficColor: "#1c69d4", selected: false },
            { name: "KURZ", time: "58 min", dist: "28 km", traffic: "BAUSTELLE", trafficColor: "#f4b400", selected: false },
          ].map((r, i) => (
            <div key={i} style={{
              padding: "14px 16px",
              borderBottom: "1px solid #262626",
              borderLeft: r.selected ? "3px solid #fff" : "3px solid transparent",
              background: r.selected ? "#1a1a1a" : "transparent",
            }}>
              <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline" }}>
                <span className="cp-cap" style={{ fontSize: 11, color: "#fff" }}>{r.name}</span>
                <span className="cp-num" style={{ fontSize: 18, color: "#fff" }}>{r.time}</span>
              </div>
              <div style={{ display: "flex", justifyContent: "space-between", marginTop: 6 }}>
                <span className="cp-cap" style={{ fontSize: 9, color: r.trafficColor }}>● {r.traffic}</span>
                <span className="cp-num" style={{ fontSize: 12, color: "#bbb" }}>{r.dist}</span>
              </div>
            </div>
          ))}
          {/* CTA */}
          <div style={{ marginTop: "auto", padding: 16 }}>
            <button style={{
              width: "100%", height: 40, background: "#fff", color: "#000",
              border: "none", borderRadius: 0, cursor: "pointer",
              letterSpacing: "1.5px", fontWeight: 700, fontSize: 12,
            }}>
              ROUTE STARTEN →
            </button>
          </div>
        </div>

        {/* Right map */}
        <div style={{ position: "relative", overflow: "hidden" }}>
          <svg width="100%" height="100%" viewBox="0 0 424 452" preserveAspectRatio="xMidYMid slice"
               style={{ position: "absolute", inset: 0 }}>
            <rect width="424" height="452" fill="#0a0e15" />
            <radialGradient id="rg2" cx="0.5" cy="0.5" r="0.8">
              <stop offset="0" stopColor="#1a2230" />
              <stop offset="1" stopColor="#0a0e15" />
            </radialGradient>
            <rect width="424" height="452" fill="url(#rg2)" />

            {/* Roads */}
            <g stroke="#2a3140" strokeWidth="1" fill="none">
              <path d="M 0 80 L 424 100" />
              <path d="M 0 200 L 424 220" />
              <path d="M 0 340 L 424 360" />
              <path d="M 80 0 L 100 452" />
              <path d="M 220 0 L 240 452" />
              <path d="M 340 0 L 360 452" />
            </g>
            <g stroke="#3a4254" strokeWidth="2" fill="none">
              <path d="M 0 280 L 424 290" />
            </g>

            {/* Three route paths, one selected */}
            <path d="M 60 400 Q 120 320 180 280 T 290 180 Q 340 120 380 60"
                  stroke="#3a4254" strokeWidth="6" fill="none" />
            <path d="M 60 400 Q 80 280 140 240 T 250 160 Q 320 100 380 60"
                  stroke="#3a4254" strokeWidth="6" fill="none" />
            {/* selected */}
            <path d="M 60 400 Q 100 360 160 320 T 270 220 Q 320 160 380 60"
                  stroke="#0066b1" strokeWidth="10" fill="none" opacity="0.9" />
            <path d="M 60 400 Q 100 360 160 320 T 270 220 Q 320 160 380 60"
                  stroke="#fff" strokeWidth="4" fill="none" />

            {/* origin */}
            <g transform="translate(60,400)">
              <circle r="9" fill="#1c69d4" />
              <circle r="3" fill="#fff" />
            </g>
            {/* destination */}
            <g transform="translate(380,60)">
              <circle r="11" fill="#e22718" />
              <circle r="4" fill="#fff" />
            </g>
          </svg>

          {/* Stripe header */}
          <div style={{ position: "absolute", top: 0, left: 0, right: 0 }}>
            <div className="cp-mstripe" style={{ height: 3 }} />
          </div>
        </div>
      </div>
    </IVIChrome>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Entertainment 3 — Vehicle Info / Status (tire pressure, performance, fluids)
// ───────────────────────────────────────────────────────────────────────────────
function EntVehicleInfo({ theme = "night", mode = "sport", state = "drive" }) {
  const s = STATES[state];
  return (
    <IVIChrome theme={theme} mode={mode} activeApp="car">
      <div style={{ display: "grid", gridTemplateColumns: "1.4fr 1fr", height: "100%" }}>
        {/* LEFT — car silhouette + tires */}
        <div style={{ padding: 20, borderRight: "1px solid #262626", display: "flex", flexDirection: "column" }}>
          <div className="cp-cap" style={{ fontSize: 11, color: "#fff" }}>FAHRZEUG · M3 COMPETITION</div>
          <div className="cp-mstripe" style={{ height: 2, marginTop: 6, width: 80 }} />

          <div style={{ flex: 1, display: "flex", alignItems: "center", justifyContent: "center", position: "relative", margin: "10px 0" }}>
            {/* Car top-down silhouette */}
            <svg width="200" height="320" viewBox="0 0 200 320" fill="none">
              {/* body */}
              <path d="M 60 30 Q 60 16 80 16 L 120 16 Q 140 16 140 30 L 144 130 L 144 200 L 140 290 Q 140 304 120 304 L 80 304 Q 60 304 60 290 L 56 200 L 56 130 Z"
                    stroke="#fff" strokeWidth="1.5" fill="#0d0d0d" />
              {/* windshield */}
              <path d="M 70 60 L 130 60 L 134 100 L 66 100 Z" fill="#1a2a3a" stroke="#3c3c3c" strokeWidth="1" />
              {/* rear glass */}
              <path d="M 68 240 L 132 240 L 134 270 L 66 270 Z" fill="#1a2a3a" stroke="#3c3c3c" strokeWidth="1" />
              {/* hood line */}
              <line x1="70" y1="40" x2="130" y2="40" stroke="#3c3c3c" strokeWidth="1" />
              {/* M kidney grilles */}
              <rect x="78" y="20" width="18" height="12" fill="#000" stroke="#3c3c3c" strokeWidth="1" />
              <rect x="104" y="20" width="18" height="12" fill="#000" stroke="#3c3c3c" strokeWidth="1" />
              {/* tires */}
              {[
                { x: 30, y: 90 },   // FL
                { x: 156, y: 90 },  // FR
                { x: 30, y: 220 },  // RL
                { x: 156, y: 220 }, // RR
              ].map((t, i) => (
                <g key={i}>
                  <rect x={t.x - 8} y={t.y - 22} width="22" height="44" fill="#1a1a1a" stroke="#3c3c3c" strokeWidth="1" />
                </g>
              ))}
            </svg>

            {/* Tire pressure overlays */}
            {[
              { v: s.tires[0], pos: { top: 80, left: 10 } },
              { v: s.tires[1], pos: { top: 80, right: 10 } },
              { v: s.tires[2], pos: { top: 220, left: 10 } },
              { v: s.tires[3], pos: { top: 220, right: 10 } },
            ].map((t, i) => {
              const low = t.v < 2.2;
              return (
                <div key={i} style={{
                  position: "absolute", ...t.pos,
                  display: "flex", flexDirection: "column", alignItems: "center", gap: 2,
                }}>
                  <span className="cp-num" style={{ fontSize: 18, color: low ? "#e22718" : "#fff", lineHeight: 1 }}>
                    {t.v.toFixed(1)}
                  </span>
                  <span className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>BAR</span>
                  <span className="cp-num" style={{ fontSize: 9, color: "#7e7e7e" }}>{[42, 42, 38, 38][i]}°C</span>
                </div>
              );
            })}
          </div>
        </div>

        {/* RIGHT — fluids + range + service */}
        <div style={{ padding: 20, display: "flex", flexDirection: "column", gap: 14 }}>
          <div className="cp-cap" style={{ fontSize: 11, color: "#fff" }}>STATUS</div>

          {/* Fluids list */}
          <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
            <FluidRow label="MOTORÖL" value={`${s.oilTemp}°C`} pct={88} status="OK" />
            <FluidRow label="KÜHLMITTEL" value={`${s.coolant}°C`} pct={94} status="OK" />
            <FluidRow label="KRAFTSTOFF" value={s.range} pct={s.fuel} status={s.fuel < 25 ? "LOW" : "OK"} />
            <FluidRow label="BREMSE" value="92%" pct={92} status="OK" />
          </div>

          {/* Service */}
          <div style={{ marginTop: "auto", borderTop: "1px solid #262626", paddingTop: 12 }}>
            <div className="cp-cap" style={{ fontSize: 9, color: s.warn ? "#f4b400" : "#7e7e7e" }}>
              {s.warn ? "● SERVICE FÄLLIG" : "NÄCHSTER SERVICE"}
            </div>
            <div className="cp-num" style={{ fontSize: 16, color: "#fff", marginTop: 2 }}>
              {s.warn ? "0 km" : "8,400 km"} · {s.warn ? "JETZT" : "MAR 2027"}
            </div>
          </div>
        </div>
      </div>
    </IVIChrome>
  );
}

function FluidRow({ label, value, pct, status }) {
  return (
    <div>
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline" }}>
        <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>{label}</span>
        <div style={{ display: "flex", gap: 8, alignItems: "baseline" }}>
          <span className="cp-num" style={{ fontSize: 14, color: "#fff" }}>{value}</span>
          <span className="cp-cap" style={{ fontSize: 8, color: status === "OK" ? "#0fa336" : "#f4b400" }}>● {status}</span>
        </div>
      </div>
      <div style={{ display: "flex", gap: 1, height: 4, marginTop: 4 }}>
        {Array.from({ length: 24 }).map((_, i) => (
          <div key={i} style={{ flex: 1, background: i < (pct / 100) * 24 ? (status === "LOW" ? "#f4b400" : "#fff") : "#262626" }} />
        ))}
      </div>
    </div>
  );
}

// ───────────────────────────────────────────────────────────────────────────────
// Entertainment 4 — M Drive setup (Comfort/Sport/Track configuration)
// ───────────────────────────────────────────────────────────────────────────────
function EntMDrive({ theme = "night", mode = "track" }) {
  const settings = mode === "comfort"
    ? { eng: "EFFICIENT", chassis: "COMFORT", steer: "COMFORT", dsc: "ON", xdrive: "4WD", brake: "COMFORT" }
    : mode === "sport"
    ? { eng: "SPORT", chassis: "SPORT", steer: "SPORT", dsc: "MDM", xdrive: "4WD SPORT", brake: "SPORT" }
    : { eng: "SPORT+", chassis: "SPORT+", steer: "SPORT+", dsc: "OFF", xdrive: "2WD", brake: "SPORT+" };

  return (
    <IVIChrome theme={theme} mode={mode} activeApp="drive">
      <div style={{ padding: 24, height: "100%", display: "flex", flexDirection: "column" }}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "baseline", gap: 16 }}>
          <h1 className="cp-cap" style={{ fontSize: 32, color: "#fff", margin: 0, letterSpacing: "-0.5px" }}>
            M DRIVE
          </h1>
          <span className="cp-cap" style={{ fontSize: 11, color: "#7e7e7e" }}>· KONFIGURATION</span>
          <div style={{ flex: 1 }} />
          <DriveModeChip mode={mode === "comfort" ? "COMFORT" : mode === "sport" ? "SPORT" : "TRACK"} />
        </div>
        <div className="cp-mstripe" style={{ height: 3, marginTop: 8, width: 200 }} />

        {/* Mode selector */}
        <div style={{ display: "flex", gap: 0, marginTop: 16, border: "1px solid #fff" }}>
          {["COMFORT", "SPORT", "SPORT+", "TRACK"].map((m) => {
            const active = m.toLowerCase().replace("+", "") === mode || (m === "TRACK" && mode === "track");
            return (
              <div key={m} style={{
                flex: 1, padding: "10px 0", textAlign: "center",
                background: active ? "#fff" : "transparent",
                color: active ? "#000" : "#fff",
                borderRight: m === "TRACK" ? "none" : "1px solid #fff",
              }}>
                <span className="cp-cap" style={{ fontSize: 11 }}>{m}</span>
              </div>
            );
          })}
        </div>

        {/* Settings grid */}
        <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 1, marginTop: 16, background: "#262626", flex: 1 }}>
          {[
            { l: "MOTOR", v: settings.eng },
            { l: "FAHRWERK", v: settings.chassis },
            { l: "LENKUNG", v: settings.steer },
            { l: "DSC", v: settings.dsc, accent: settings.dsc === "OFF" ? "#e22718" : "#fff" },
            { l: "M xDRIVE", v: settings.xdrive },
            { l: "BREMSE", v: settings.brake },
          ].map((it, i) => (
            <div key={i} style={{ background: "#0d0d0d", padding: "14px 18px" }}>
              <div className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>{it.l}</div>
              <div className="cp-cap" style={{ fontSize: 18, color: it.accent || "#fff", marginTop: 4 }}>{it.v}</div>
            </div>
          ))}
        </div>

        <div style={{ display: "flex", gap: 10, marginTop: 14 }}>
          <button style={{ flex: 1, height: 40, background: "transparent", border: "1px solid #fff", color: "#fff", letterSpacing: "1.5px", fontSize: 11, fontWeight: 700, cursor: "pointer" }}>
            M1 SPEICHERN
          </button>
          <button style={{ flex: 1, height: 40, background: "#fff", border: "1px solid #fff", color: "#000", letterSpacing: "1.5px", fontSize: 11, fontWeight: 700, cursor: "pointer" }}>
            M2 AKTIV →
          </button>
        </div>
      </div>
    </IVIChrome>
  );
}

Object.assign(window, { EntNav, EntNavSearch, EntVehicleInfo, EntMDrive, IVIChrome });
