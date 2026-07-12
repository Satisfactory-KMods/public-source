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

KUI is the shared UI framework and asset library that powers the visual layer of all KMods — providing localization macros, a rich icon/button/background library, 9-slice textures, dynamic materials, bundled fonts, widget element primitives, and reusable UMG window templates.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Localization Macros</strong></td></tr></table>

KUI exposes a lightweight C++ localization API via `KUIModule.h`. All KMods use these macros instead of raw `LOCTEXT`/`NSLOCTEXT` calls so every string automatically participates in the shared string-table pipeline.

| Macro | Returns | Description |
|---|---|---|
| `FI18N_TEXT(Key)` | `FText` | Look up a localized `FText` from the `KUI/ST_KUI_CPP` string table |
| `FI18N_FORMAT(Key, ...)` | `FString` | Look up + format with positional `FStringFormatArg` arguments |
| `FI18N_NS(Key, Default)` | `FText` | Inline `NSLOCTEXT`-style text (no string table required) |
| `FI18N_NS_FORMAT(Key, Default, ...)` | `FString` | Inline text with positional format arguments |

String table assets are split by domain for easy translation management:

| String Table | Domain |
|---|---|
| `KUI_GeneralWords` | Common UI labels |
| `KUI_Production` | Factory / production status |
| `KUI_Resource` | Resource names and units |
| `KUI_Time` | Duration and time formatting |
| `KUI_ItemFormat` | Item count and stack display |
| `KUI_WidgetWords` | Widget control labels |
| `KUI_Formats` | Generic format patterns |
| `KUI_Popups` | Popup and confirmation text |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Icon Library</strong></td></tr></table>

Over 160 icon textures organized into sub-libraries, ready to reference from any Blueprint or C++ widget:

- **Action icons** — arrows, navigation, CRUD (add, delete, copy, paste, rename, refresh, search, save, settings, edit, link, sort, menu, warning, lock/unlock, info, plus/minus, drag handle)
- **Equipment icons** — build gun, codex, customizer, dismantle, flashlight, holster, inventory, map, resource scanner
- **Map compass icons** — cluster border, directional actors, ping actors, static actors, hub marker, loot crate
- **Resource category icons** — foundations, logistics, organisation, power, production, transport, walls
- **Status / indicator icons** — aggro, directional arrows, LED indicators (green/red), standby set, small variant set
- **White icon variants** — 52 white/outline versions for dark-background contexts

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Button &amp; Toggle Assets</strong></td></tr></table>

Pre-built button and toggle textures that cover the standard Satisfactory UI aesthetic:

- Default confirm / cancel buttons (`default_yes`, `default_no`)
- Toggle pair (`toggle_on`, `toggle_off`)
- Switch states — on, off, up, down, with green / red / unlit light variants
- Scroll grabber, slider grab handle, transparent and white slider text backgrounds
- Page navigation buttons (`page_back`, `page_next`)
- Manual icon button, custom header button asset

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">9-Slice Textures</strong></td></tr></table>

Thirteen named 9-slice assets for pixel-perfect scalable borders and panels:

| Asset | Description |
|---|---|
| `9S_0000` – `9S_1111` | Full bitmask set (corner flags: top-left, top-right, bottom-right, bottom-left) |
| `25_9S_0000` / `50_9S_0000` / `75_9S_0000` | Opacity variants at 25 %, 50 %, 75 % |
| `MM_TransperentSlice` | Transparent master material for runtime tinting |
| `selection_border` | Highlight selection ring |
| `guide` | Layout guide overlay |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Materials</strong></td></tr></table>

A focused set of dynamic UI materials ready for instancing:

| Material | Use |
|---|---|
| `MI_DiagonalLinesProgressbar` / `MI_DiagonalLinesProgressbar1` | Animated diagonal-stripe progress bar fills |
| `MI_TileCleaner` / `MI_TileCleaner1` | Tiling texture for cleaner machine UI backgrounds |
| `MI_TileCleaner_Recipe` | Recipe slot tiled background variant |
| `MI_TileDiagonalLines` | Standalone diagonal-line tile for general panel fills |
| `MM_TileImages` | Master material for all tiled image instances |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Bundled Fonts</strong></td></tr></table>

Eight ready-to-use font families shipped with KUI — no extra installation needed:

| Font | Style |
|---|---|
| **Roboto** | Clean sans-serif; default body text |
| **OpenSans** | Legible sans-serif; labels and tables |
| **Rajdhani** | Condensed semi-technical |
| **Orbitron** | Geometric sci-fi display font |
| **Antonio** | Bold condensed display |
| **Amatic** | Handwritten / casual |
| **CourierPrime** | Monospaced; data readouts |
| **Allan** | Stylized display |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Widget Element Primitives</strong></td></tr></table>

Reusable texture elements for assembling custom UMG widgets:

- **Backgrounds** — `BG_converter`, `BG_downlink`, `BG_tower`, `BG_uplink`, `converter_mk2` plus `window_shine` overlay
- **UI Components** — corner border, flat cable, fuse label/body/plug, module box, shadow-top strip, blank panels (normal + small)
- **Widget controls** — blank, buff indicator, green/red LED, green switch light, on/off switch, warning badge, text block, dot, scroll grabber, white slider backgrounds
- **Logos** — KMods, KMods Basic, LIBS, Pneumatic, RSS, SF+, UP

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Reusable UMG Window Templates</strong></td></tr></table>

Pre-built Blueprint Widget classes that other KMods extend:

| Widget | Purpose |
|---|---|
| `KUI_MainWindow` | Full-size buildable interaction window with header and tabs |
| `KUI_SubWindow` | Nested sub-panel inside a main window |
| `KUI_CustomHeaderButton` | Styled tab/header button |
| `KUI_CustomOverclockingUI` | Overclock slider panel |
| `KUI_CustomOverclockingUI_Production` | Production-aware overclock variant |
| `KUI_ProductionInfoWidget` | Machine production status block |
| `KUI_ProductionInfoWidget_Small` | Compact production status block |
| `KUI_Widget_Tooltip` | Styled tooltip container |
| `BPW_KUI_FuelList` | Fuel-type list popup with entity rows |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Sounds &amp; Miscellaneous</strong></td></tr></table>

- `Sound_LevelUP` — shared level-up / unlock audio cue
- `BFL_Widget` — Blueprint Function Library with widget utilities
- `EIndikator` / `EClampType` — shared enumerations for indicator states and UI clamping

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
