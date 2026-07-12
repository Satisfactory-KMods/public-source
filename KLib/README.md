<table width="100%" cellpadding="0" cellspacing="0" style="border:none;background:#1a1a2e;border-radius:12px;overflow:hidden;margin-bottom:16px">
<tr>
<td width="140" style="padding:20px 16px 20px 24px;vertical-align:middle;border:none">
  <a href="{{DISCORD_URL}}">
    <img src="https://raw.githubusercontent.com/Kyri123/KMods-Docs/main/docs/Images/KMods-Logo.png" height="100" width="100" alt="KMods Logo" />
  </a>
</td>
<td style="padding:20px 24px 20px 0;vertical-align:middle;text-align:right;border:none">
  <div>
    <a href="{{DISCORD_URL}}"><img src="https://img.shields.io/badge/Discord-Join%20Us-5865F2?style=for-the-badge&logo=discord&logoColor=white" height="28" alt="Discord" /></a>
    &nbsp;
    <a href="{{PATREON_URL}}"><img src="https://img.shields.io/badge/Patreon-Support%20Us-F96854?style=for-the-badge&logo=patreon&logoColor=white" height="28" alt="Patreon" /></a>
  </div>
  <div style="margin-top:8px">
    <a href="{{FICSIT_PROFILE_URL}}"><img src="https://img.shields.io/badge/ficsit.app-KMods-009688?style=for-the-badge" height="28" alt="ficsit.app" /></a>
    &nbsp;
    {{MULTIPLAYER_BADGE}}
  </div>
</td>
</tr>
</table>

<br />

<table width="100%" cellpadding="0" cellspacing="0" style="border:none">
<tr>
<td style="padding:4px 0;border:none">
  <a href="https://k-mods.com"><img src="https://img.shields.io/badge/k--mods.com-Website-e8a202?style=flat-square" alt="k-mods.com" /></a>
  &nbsp;
  <a href="https://k-mods.com/production-planner"><img src="https://img.shields.io/badge/SF%2B-Production%20Planner-e8a202?style=flat-square" alt="SF+ Production Planner" /></a>
  &nbsp;
  <a href="https://k-mods.com/wiki"><img src="https://img.shields.io/badge/SF%2B-Wiki-e8a202?style=flat-square" alt="SF+ Wiki" /></a>
</td>
</tr>
</table>

{{IMPORTANT_NOTE}}

{{INFORMATION}}

---

**KLib** is the core gameplay mod of the KMods suite — it adds a collection of advanced production buildings powered by the data-driven [KAPI](https://ficsit.app/mod/KAPI) framework, all with full multiplayer support, overclocking, and Factory Clipboard integration.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Modular Miner</strong></td></tr></table>

A fully customizable resource extractor built around a snap-on module system. Place the main miner on any resource node, then attach modules to shape its output.

- **Dual belt outputs** plus a pipe connection for fluid nodes
- **Swappable modules** — Drill, Fluid, Power Shard, and Waste Producer attachments
- Each attached module multiplies throughput (3× per module by default)
- Production time scales dynamically with node purity and installed modules
- Full overclocking support with custom power curves and pending potential
- Copy/paste settings via the **Factory Clipboard**

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Slug Breeding &amp; Incubation</strong></td></tr></table>

A two-stage creature farming system for producing Power Slugs and their variants.

### Slug Breeder
Place two groups of parent slugs, keep them fed, and dial in the right temperature and humidity. When conditions are met the breeder produces eggs.

- Two parent slots (up to 5 slugs each) + one food slot
- Configurable temperature and humidity targets
- Visual status indicators for slug comfort and feeding state
- Overclocking speeds up the breeding cycle
- Settings are copy/paste-able via the Factory Clipboard

### Slug Incubator
Hatch eggs by supplying the right fluid and environmental conditions.

- One egg input + fluid input, multiple slug outputs
- **Snap-on modules** change hatching behaviour:

  | Module | Effect |
  |--------|--------|
  | Temperature | Raises or lowers incubation temperature |
  | Humidity | Controls ambient humidity (0–1) |
  | Tank | Increases fluid buffer capacity |
  | Incubator | Active temperature regulation |
  | Time | Restricts hatching to day-only or night-only cycles |

- Egg species + active modules together determine which slug variant hatches
- Smooth stat transitions driven by configurable float curves

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Air Collector</strong></td></tr></table>

Extract atmospheric gases and particles without needing a resource node.

- Production scales with **altitude** — the higher you build, the more you get (4 000 – 45 000 UU range)
- **Clustering bonus** — collectors within 2 500 UU of each other boost each other's output
- **Proximity debuff** — other nearby buildings reduce output; the fewer structures in range, the higher the yield
- Output type is determined by KAPI scan data assets
- Gas output via pipe connection
- Build-mode sphere visualisation shows the cluster range before you place

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Cleaner / Filtrator</strong></td></tr></table>

A configurable item-washing station that processes items through fluids, driven by KAPI recipe data assets.

- Three connections: fluid in (pipe), item in (belt), fluid out (pipe)
- Recipe auto-detected from the input item, or selected manually
- Up to 10 parallel bypass processing slots
- Recipes unlock progressively through M.A.M. research
- Overclocking reduces processing time

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Sink Storage</strong></td></tr></table>

Power-consuming storage buildings integrated with the AWESOME Sink subsystem.

| Building | Base | Notes |
|----------|------|-------|
| **Sink Storage** | Standard Container | Storage with a power requirement |
| **Depot Sink Storage** | Central Storage | Large unified depot variant |

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Toxic Resource Node</strong></td></tr></table>

Hazardous resource deposits that spawn ambient gas pillars and clouds. Cannot be mined by standard extractors — requires specialist equipment.

---

---

{{GALLERY}}

---

<table width="100%" cellpadding="0" cellspacing="0" style="border:none;background:#1a1a2e;border-radius:12px;overflow:hidden;margin-top:16px">
<tr>
<td colspan="2" style="background:#e8a202;padding:0;border:none;height:4px"></td>
</tr>
<tr>
<td style="padding:20px 24px;border:none;vertical-align:middle">
  <strong style="color:#e8a202;font-size:14px">Questions, bugs, or suggestions?</strong><br />
  <span style="color:#94a3b8;font-size:12px">Join the Discord or support us on Patreon</span>
</td>
<td style="padding:20px 24px 20px 0;text-align:right;border:none;vertical-align:middle">
  <table cellpadding="0" cellspacing="0" style="display:inline-table;border:none">
  <tr>
    <td style="padding:3px 4px;border:none"><a href="{{DISCORD_URL}}"><img src="https://img.shields.io/badge/Discord-5865F2?style=for-the-badge&logo=discord&logoColor=white" height="28" alt="Discord" /></a></td>
    <td style="padding:3px 4px;border:none"><a href="{{PATREON_URL}}"><img src="https://img.shields.io/badge/Patreon-F96854?style=for-the-badge&logo=patreon&logoColor=white" height="28" alt="Patreon" /></a></td>
  </tr>
  <tr>
    <td style="padding:3px 4px;border:none"><a href="{{FICSIT_PROFILE_URL}}"><img src="https://img.shields.io/badge/ficsit.app-009688?style=for-the-badge" height="28" alt="ficsit.app" /></a></td>
    <td style="padding:3px 4px;border:none"><a href="https://k-mods.com"><img src="https://img.shields.io/badge/k--mods.com-e8a202?style=for-the-badge" height="28" alt="k-mods.com" /></a></td>
  </tr>
  <tr>
    <td style="padding:3px 4px;border:none"><a href="https://k-mods.com/production-planner"><img src="https://img.shields.io/badge/SF%2B%20Planner-e8a202?style=for-the-badge" height="28" alt="SF+ Production Planner" /></a></td>
    <td style="padding:3px 4px;border:none"><a href="https://k-mods.com/wiki"><img src="https://img.shields.io/badge/SF%2B%20Wiki-e8a202?style=for-the-badge" height="28" alt="SF+ Wiki" /></a></td>
  </tr>
  </table>
</td>
</tr>
</table>
