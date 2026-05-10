// dashboard.jsx — Cockpit context mockup: shows both 800x480 screens
// inside a stylized BMW M dashboard surround.

function CockpitMockup({ theme = "night", mode = "sport" }) {
  return (
    <div style={{
      width: 1760, height: 720, background: "#000", position: "relative",
      fontFamily: "Inter, sans-serif", overflow: "hidden",
    }}>
      {/* Dashboard top edge — soft carbon-gray gradient suggestion */}
      <div style={{
        position: "absolute", top: 0, left: 0, right: 0, height: 80,
        background: "linear-gradient(to bottom, #0d0d0d 0%, #000 100%)",
        borderBottom: "1px solid #1a1a1a",
      }} />

      {/* M tricolor stripe at very top */}
      <div className="cp-mstripe" style={{ height: 3, position: "absolute", top: 0, left: 0, right: 0 }} />

      {/* Dashboard label */}
      <div style={{ position: "absolute", top: 24, left: 40, display: "flex", alignItems: "center", gap: 16 }}>
        <div style={{ display: "flex", gap: 0 }}>
          <div style={{ width: 20, height: 20, background: "#0066b1" }} />
          <div style={{ width: 20, height: 20, background: "#1c69d4" }} />
          <div style={{ width: 20, height: 20, background: "#e22718" }} />
        </div>
        <span className="cp-cap" style={{ fontSize: 14, color: "#fff", letterSpacing: "1.5px" }}>
          M COCKPIT — 800 × 480 DUAL DISPLAY
        </span>
      </div>
      <div style={{ position: "absolute", top: 24, right: 40 }}>
        <span className="cp-cap" style={{ fontSize: 11, color: "#7e7e7e" }}>
          {mode.toUpperCase()} · {theme.toUpperCase()}
        </span>
      </div>

      {/* Two screens — bezels and labels */}
      <div style={{
        position: "absolute", top: 100, left: 60, display: "flex", gap: 40,
      }}>
        {/* Cluster screen frame */}
        <ScreenFrame label="CLUSTER" sub="INSTRUMENT PANEL · DRIVER">
          <ClusterCenter state="drive" theme={theme} mode={mode} />
        </ScreenFrame>

        {/* Entertainment screen frame */}
        <ScreenFrame label="ENTERTAINMENT" sub="CENTER STACK · IVI">
          <EntNav theme={theme} mode={mode} state="drive" />
        </ScreenFrame>
      </div>

      {/* Steering wheel hint — bottom left */}
      <div style={{
        position: "absolute", bottom: -120, left: 220, width: 320, height: 320,
        borderRadius: "50%", border: "8px solid #1a1a1a",
        boxShadow: "inset 0 0 0 1px #262626",
      }}>
        {/* hub */}
        <div style={{
          position: "absolute", top: "50%", left: "50%", width: 80, height: 60,
          transform: "translate(-50%, -50%)", background: "#0d0d0d",
          border: "1px solid #262626",
          display: "flex", alignItems: "center", justifyContent: "center",
        }}>
          <div style={{ display: "flex", gap: 0 }}>
            <div style={{ width: 8, height: 16, background: "#0066b1" }} />
            <div style={{ width: 8, height: 16, background: "#1c69d4" }} />
            <div style={{ width: 8, height: 16, background: "#e22718" }} />
          </div>
        </div>
        {/* spokes */}
        <div style={{ position: "absolute", top: "50%", left: 0, right: 0, height: 8, background: "#1a1a1a", transform: "translateY(-50%)" }} />
        <div style={{ position: "absolute", left: "50%", bottom: 0, top: "50%", width: 8, background: "#1a1a1a", transform: "translateX(-50%)" }} />
      </div>

      {/* Center console hint — between/below the screens */}
      <div style={{
        position: "absolute", bottom: 0, left: 700, right: 60, height: 80,
        background: "linear-gradient(to bottom, #000 0%, #0d0d0d 100%)",
        borderTop: "1px solid #1a1a1a",
        display: "flex", alignItems: "center", justifyContent: "space-around",
        padding: "0 60px",
      }}>
        {/* iDrive controller */}
        <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 6 }}>
          <div style={{ width: 44, height: 44, borderRadius: "50%", border: "1px solid #3c3c3c", background: "#0d0d0d" }} />
          <span className="cp-cap" style={{ fontSize: 8, color: "#7e7e7e" }}>iDRIVE</span>
        </div>
        {/* hard buttons */}
        {["MAP", "MEDIA", "TEL", "MENU", "OPTIONS"].map((b) => (
          <div key={b} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
            <div style={{ width: 28, height: 18, background: "#0d0d0d", border: "1px solid #262626" }} />
            <span className="cp-cap" style={{ fontSize: 7, color: "#7e7e7e" }}>{b}</span>
          </div>
        ))}
        {/* M1/M2 buttons */}
        <div style={{ display: "flex", gap: 8 }}>
          {["M1", "M2"].map((b) => (
            <div key={b} style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 4 }}>
              <div style={{ width: 32, height: 22, background: "#0d0d0d", border: "1px solid #e22718", display: "flex", alignItems: "center", justifyContent: "center" }}>
                <span className="cp-cap" style={{ fontSize: 9, color: "#e22718" }}>{b}</span>
              </div>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

function ScreenFrame({ label, sub, children }) {
  return (
    <div style={{ display: "flex", flexDirection: "column", gap: 12 }}>
      {/* Label above */}
      <div style={{ display: "flex", justifyContent: "space-between", alignItems: "baseline", paddingLeft: 4 }}>
        <span className="cp-cap" style={{ fontSize: 11, color: "#fff", letterSpacing: "1.5px" }}>{label}</span>
        <span className="cp-cap" style={{ fontSize: 9, color: "#7e7e7e" }}>{sub}</span>
      </div>
      {/* Bezel */}
      <div style={{
        padding: 8, background: "#0a0a0a",
        border: "1px solid #1a1a1a",
        boxShadow: "inset 0 0 0 1px #000",
      }}>
        {children}
      </div>
      {/* Resolution caption */}
      <span className="cp-cap" style={{ fontSize: 9, color: "#3c3c3c", paddingLeft: 4, letterSpacing: "1.5px" }}>
        800 × 480 · 16:10
      </span>
    </div>
  );
}

Object.assign(window, { CockpitMockup });
