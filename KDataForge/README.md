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
  <a href="https://k-mods.com/documentation/kdataforge"><img src="https://img.shields.io/badge/KDataForge-Documentation-1f8acb?style=flat-square" alt="KDataForge Documentation" /></a>
</td>
</tr>
</table>

{{IMPORTANT_NOTE}}

{{INFORMATION}}

---

KDataForge lets you rebalance and create Satisfactory game content — recipes, items, buildings, gameplay tags and more — with plain YAML files, no code or editor required.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">What Is KDataForge?</strong></td></tr></table>

KDataForge is a reflection-based runtime content editor. Drop `.yml` files into `<GameFolder>/FactoryGame/DataForge/` and it applies them at startup — no C++, no Blueprint, no cooking. It can edit any reflected property on any class default object or data asset, register gameplay tags, generate entirely new items/buildings/recipes/schematics, and patch spawned actors live.

Full grammar, examples for every document type, and task-oriented tutorials live in the [documentation](https://k-mods.com/documentation/kdataforge).

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">YAML-Driven Editing</strong></td></tr></table>

- Edit **any reflected property** on any class default object: numbers, texts, arrays, maps, structs, object references
- 22 property operations: `set`, `multiply`, `clamp`, `append`, `remove`, `sort`, `copy`, and more
- Target class defaults, single data assets, or **all assets of a class** at once
- Reference classes by their short name (e.g. `Desc_Water`) instead of the full asset path
- Conditional documents: apply only for certain game versions, installed mods, or existing classes
- Trace every applied change with debug logging — per file (`debug: true`), per pack, or globally (`KDF.Debug 1` / `-KDFDebug`), each op logged with its before/after values

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Content Creation</strong></td></tr></table>

- Create **new items, resources, buildings, recipes, schematics, research trees and unlocks** from YAML — no editor, no cooking
- Generated content is **savegame-stable**: deterministic id-based classes plus a tombstone manifest mean your saves load cleanly even after packs change or disappear
- Rename ids safely with pack `redirects:` — old saves keep working
- Register existing classes (e.g. hidden schematics from other mods) without generating anything
- Go beyond the preset types with `type: class` — generate a class from **any** parent (equipment descriptors, categories, other mods' classes, …), C++ or Blueprint
- Ship your own **2D images** (PNG/JPG/TGA/BMP) in a pack and use them as item or building icons — loaded at runtime, no cooking

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Gameplay Tags &amp; Runtime Actor Patches</strong></td></tr></table>

- Register new gameplay tags at runtime from YAML; add/remove/clear tags on any tag container property
- `applyToSpawnedActors: true` re-applies your values to every actor instance the moment it spawns — even values that saves or archetypes would normally override

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Content Packs &amp; Live Reload</strong></td></tr></table>

- Group files into packs with a `pack.yml` manifest (name, version, priority, dependencies) — deterministic load order across packs and files
- `/kdf reload` in chat re-reads your YAML and applies CDO/tag changes without restarting
- `/kdf report` prints a full load report with warnings and errors
- Broken files never block loading — you get warnings instead

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Mod Integration</strong></td></tr></table>

- **Create** real PrimaryDataAsset instances from YAML — e.g. new KAPI cleaner items or RefinedR&D boiler/heater/turbine data assets
- **Edit** any mod's existing data assets with plain `cdo` documents (single asset, all assets of a class, or by gameplay tag)
- **Auto-rescan**: KAPI and RefinedR&D (RefinedPower / FicsitFarming) are notified after every load and reload, so your data assets are picked up without a restart — and without KDataForge requiring those mods

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Chat &amp; Console Commands</strong></td></tr></table>

| Command | Where | Effect |
|---|---|---|
| `/kdf report` | chat | Prints the load report: packs, applied documents, ops, warnings, errors |
| `/kdf reload` | chat (host) | Reverts + re-applies live-safe stages (tags, data assets, localization, CDO changes) without restarting |
| `/kdf editor` | chat (host) | Toggles the in-game editor — browse content, edit properties live, undo/redo, and export changes to YAML |
| `/kdf debug on\|off` | chat | Flips global debug logging (same switch as `KDF.Debug`); bare `/kdf debug` toggles the current state |
| `KDF.Editor` | console | Same as `/kdf editor` |
| `KDF.Debug 1` | console (or `-KDFDebug`) | Global debug logging — one `[KDF DEBUG]` line per applied op, including editor edits |
| `KDF.ApplyToAssets [PathFilter]` | console (**UE editor builds only**) | Applies loaded YAML directly onto editor assets and marks packages dirty |

{{GALLERY}}

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Technical</strong></td></tr></table>

- Reflection-based property engine — supports every reflectable UObject property type, including Blueprint structs and PrimaryDataAssets
- Fixed stage pipeline (tags → assets → localization → CDO changes → content → validation)
- Vanilla value snapshots power revert, live reload, and safe instance propagation
- Multiplayer-safe: on join, the server compares patchset checksums — clients with different packs are rejected with a clear message listing both sides' packs, so mismatched sessions can never silently desync
- Extensible C++/Blueprint handler API for other mods (`IKDFDataEditorHandler`)
- Dedicated server compatible

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Requirements</strong></td></tr></table>

| Dependency | Link |
|---|---|
| SML | [ficsit.app](https://ficsit.app/mod/SML) |
| KAPI | [ficsit.app](https://ficsit.app/mod/KAPI) |

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
    <td colspan="2" style="padding:3px 4px;border:none"><a href="https://k-mods.com/documentation/kdataforge"><img src="https://img.shields.io/badge/KDataForge%20Documentation-1f8acb?style=for-the-badge" height="28" alt="KDataForge Documentation" /></a></td>
  </tr>
  </table>
</td>
</tr>
</table>
