// app.jsx — main entry: DesignCanvas with all sections + Tweaks panel.

const TWEAK_DEFAULTS = /*EDITMODE-BEGIN*/{
  "theme": "night",
  "mode": "sport"
}/*EDITMODE-END*/;

function App() {
  const [t, setTweak] = useTweaks(TWEAK_DEFAULTS);
  const theme = t.theme;
  const mode = t.mode;

  return (
    <>
      <DesignCanvas>
        {/* Section 1 — Cockpit context */}
        <DCSection id="cockpit" title="M Cockpit · Dual Display Context"
          subtitle="Both 800×480 screens placed in their dashboard context — cluster (driver) + entertainment (center stack)">
          <DCArtboard id="cockpit-full" label="Full cockpit · driving" width={1760} height={720}>
            <CockpitMockup theme={theme} mode={mode} />
          </DCArtboard>
        </DCSection>

        {/* Section 2 — Cluster · Layout A · Classic Dual Dial */}
        <DCSection id="cluster-classic"
          title="Cluster A · Classic Dual Dial"
          subtitle="Twin analog-style dials. Familiar BMW heritage layout — RPM left, speed right, gear in the centre.">
          <DCArtboard id="ca-idle"  label="Idle · Park" width={800} height={480}>
            <ClusterClassic state="idle" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="ca-drive" label="Driving · 87 km/h" width={800} height={480}>
            <ClusterClassic state="drive" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="ca-track" label="Track · Nürburgring" width={800} height={480}>
            <ClusterClassic state="track" theme={theme} mode="track" />
          </DCArtboard>
        </DCSection>

        {/* Section 3 — Cluster · Layout B · Center Speed */}
        <DCSection id="cluster-center"
          title="Cluster B · Center Speed"
          subtitle="Single dominant speed numeral. RPM rendered as a discrete-cell bar across the top — sporty signature.">
          <DCArtboard id="cb-idle"  label="Idle · Park" width={800} height={480}>
            <ClusterCenter state="idle" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="cb-drive" label="Driving · 87 km/h" width={800} height={480}>
            <ClusterCenter state="drive" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="cb-track" label="Track · 218 km/h" width={800} height={480}>
            <ClusterCenter state="track" theme={theme} mode="track" />
          </DCArtboard>
        </DCSection>

        {/* Section 4 — Cluster · Layout C · Minimal Digital */}
        <DCSection id="cluster-minimal"
          title="Cluster C · Minimal Digital"
          subtitle="No dials. Heavy-display numerals + dense column metrics. Engineering-forward, dashboard-as-readout.">
          <DCArtboard id="cc-idle"  label="Idle · Park" width={800} height={480}>
            <ClusterMinimal state="idle" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="cc-drive" label="Driving · 87 km/h" width={800} height={480}>
            <ClusterMinimal state="drive" theme={theme} mode={mode} />
          </DCArtboard>
          <DCArtboard id="cc-track" label="Track · 218 km/h" width={800} height={480}>
            <ClusterMinimal state="track" theme={theme} mode="track" />
          </DCArtboard>
        </DCSection>

        {/* Section 5 — Cluster · Layout D · M Track */}
        <DCSection id="cluster-mtrack"
          title="Cluster D · M Track"
          subtitle="Race telemetry surface. Full-width RPM bar, lap timer, sectors, G-force compass. Surfaces only when in TRACK mode.">
          <DCArtboard id="cd-idle"  label="Pit lane · Stationary" width={800} height={480}>
            <ClusterMTrack state="idle" theme={theme} mode="track" />
          </DCArtboard>
          <DCArtboard id="cd-drive" label="Out-lap · 87 km/h" width={800} height={480}>
            <ClusterMTrack state="drive" theme={theme} mode="track" />
          </DCArtboard>
          <DCArtboard id="cd-track" label="Hot lap · Lap 12 / S2" width={800} height={480}>
            <ClusterMTrack state="track" theme={theme} mode="track" />
          </DCArtboard>
        </DCSection>

        {/* Section 6 — Entertainment · Navigation (focus) */}
        <DCSection id="ent-nav"
          title="Entertainment · Navigation"
          subtitle="Primary surface of the IVI — full-screen map with maneuver banner, ETA, lane assist, and live speed limit.">
          <DCArtboard id="en-active" label="Active route · Berlin Mitte" width={800} height={480}>
            <EntNav theme={theme} mode={mode} state="drive" />
          </DCArtboard>
          <DCArtboard id="en-search" label="Route selection · 3 options" width={800} height={480}>
            <EntNavSearch theme={theme} mode={mode} />
          </DCArtboard>
        </DCSection>

        {/* Section 7 — Entertainment · Vehicle */}
        <DCSection id="ent-vehicle"
          title="Entertainment · Vehicle Info"
          subtitle="Tire pressure quadrant, fluid status, range and service. Always one tap from the main rail.">
          <DCArtboard id="ev-status" label="Vehicle status" width={800} height={480}>
            <EntVehicleInfo theme={theme} mode={mode} state="drive" />
          </DCArtboard>
          <DCArtboard id="ev-mdrive" label="M Drive · setup" width={800} height={480}>
            <EntMDrive theme={theme} mode={mode === "track" ? "track" : mode === "sport" ? "sport" : "comfort"} />
          </DCArtboard>
        </DCSection>
      </DesignCanvas>

      {/* Tweaks panel */}
      <TweaksPanel title="Cockpit · Tweaks">
        <TweakSection label="Theme">
          <TweakRadio label="Day / Night" value={theme}
            options={[{ value: "night", label: "NIGHT" }, { value: "day", label: "DAY" }]}
            onChange={(v) => setTweak("theme", v)} />
        </TweakSection>
        <TweakSection label="Drive Mode">
          <TweakRadio label="Mode" value={mode}
            options={[
              { value: "comfort", label: "COMF" },
              { value: "sport",   label: "SPORT" },
              { value: "track",   label: "TRACK" },
            ]}
            onChange={(v) => setTweak("mode", v)} />
        </TweakSection>
      </TweaksPanel>
    </>
  );
}

ReactDOM.createRoot(document.getElementById("root")).render(<App />);
