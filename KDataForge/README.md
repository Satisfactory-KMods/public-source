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
  <a href="https://docs.k-mods.com/kdataforge/"><img src="https://img.shields.io/badge/KDataForge-Documentation-1f8acb?style=flat-square" alt="KDataForge Documentation" /></a>
</td>
</tr>
</table>

{{IMPORTANT_NOTE}}

{{INFORMATION}}

---

KDataForge lets pack authors rebalance existing Satisfactory content or create recipes, items, buildings, gameplay tags, and more in YAML. You do not need to write code or cook Unreal assets.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">What KDataForge does</strong></td></tr></table>

KDataForge is a reflection-based runtime content editor. Put `.yml` files in `<GameFolder>/FactoryGame/DataForge/`; KDataForge applies them at startup. It can edit reflected properties on class default objects and data assets, register gameplay tags, generate items, buildings, recipes, and schematics, or patch spawned actors while the game runs.

See the [documentation](https://docs.k-mods.com/kdataforge/) for the full YAML grammar, examples for every document type, and task-based tutorials.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">YAML editing</strong></td></tr></table>

- Edit **any reflected property** on any class default object: numbers, texts, arrays, maps, structs, object references
- 22 property operations: `set`, `multiply`, `clamp`, `append`, `remove`, `sort`, `copy`, and more
- Target class defaults, single data assets, or **all assets of a class** at once
- Reference classes by their short name (e.g. `Desc_Water`) instead of the full asset path
- Conditional documents: apply only for certain game versions, installed mods, or existing classes
- Trace every applied change with debug logging: per file (`debug: true`), per pack, or globally (`KDF.Debug 1` / `-KDFDebug`), each op logged with its before/after values

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Content creation</strong></td></tr></table>

- Create **new items, resources, buildings, recipes, schematics, research trees and unlocks** from YAML: no editor, no cooking
- Generated content is **savegame-stable**: deterministic id-based classes plus a tombstone manifest mean your saves load cleanly even after packs change or disappear
- Rename ids safely with pack `redirects:` so old saves keep working
- Register existing classes (e.g. hidden schematics from other mods) without generating anything
- Go beyond the preset types with `type: class`: generate a class from **any** parent (equipment descriptors, categories, other mods' classes, …), C++ or Blueprint
- Ship your own **2D images** (PNG/JPG/TGA/BMP) in a pack and use them as item or building icons: loaded at runtime, no cooking

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Gameplay tags and runtime actor patches</strong></td></tr></table>

- Register new gameplay tags at runtime from YAML; add/remove/clear tags on any tag container property
- `applyToSpawnedActors: true` re-applies your values to every actor instance the moment it spawns: even values that saves or archetypes would normally override

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Content packs and session loading</strong></td></tr></table>

- Group files into packs with a `pack.yml` manifest (name, version, priority, dependencies) for deterministic load order across packs and files
- DataForge YAML is loaded once during session initialization; restart the session after changing files
- `/kdf report` prints a full load report with warnings and errors
- Broken files never block loading: you get warnings instead

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Mod integration</strong></td></tr></table>

- **Create** real PrimaryDataAsset instances from YAML: e.g. new KAPI cleaner items or RefinedR&D boiler/heater/turbine data assets
- **Edit** any mod's existing data assets with plain `cdo` documents (single asset, all assets of a class, or by gameplay tag)
- **Create or edit curves** with `type: curve`: `UCurveFloat` and `UCurveVector`, including interpolation, tangents, defaults, and extrapolation
- **Auto-rescan**: KAPI and RefinedR&D (RefinedPower / FicsitFarming) are notified after the initial load, without KDataForge requiring those mods

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Chat and console commands</strong></td></tr></table>

| Command                          | Where                               | Effect                                                                                                  |
| -------------------------------- | ----------------------------------- | ------------------------------------------------------------------------------------------------------- |
| `/kdf report`                    | chat                                | Prints the load report: packs, applied documents, ops, warnings, errors                                 |
| `/kdf editor`                    | chat (host)                         | Toggles the in-game editor: browse content, edit properties live, undo/redo, and export changes to YAML |
| `/kdf debug on\|off`             | chat                                | Flips global debug logging (same switch as `KDF.Debug`); bare `/kdf debug` toggles the current state    |
| `KDF.Editor`                     | console                             | Same as `/kdf editor`                                                                                   |
| `KDF.Debug 1`                    | console (or `-KDFDebug`)            | Global debug logging: one `[KDF DEBUG]` line per applied op, including editor edits                     |
| `KDF.ApplyToAssets [PathFilter]` | console (**UE editor builds only**) | Applies loaded YAML directly onto editor assets and marks packages dirty                                |
| `/kdf-reflection [options]`      | chat (host/server)                  | Starts time-sliced export; no options enables assets, functions, flattened classes, ZIP, and 1024px PNGs |
| `/kdf-export [options]`          | chat alias                          | Alias of `/kdf-reflection`                                                                               |
| `KDF.DumpReflection [options]`   | console (host/server)               | Starts export with core defaults below; output defaults to `FactoryGame/KMods/ReflectionExport/`          |
| `KDF.ValidateReflection [--out <dir>]` | console                        | Checks runtime reflection contracts and basic completed-export checks                                   |

### Reflection export for Pack Manager

Run after world begins play:

```text
KDF.DumpReflection --assets --functions --flattened
```

Bare `KDF.DumpReflection` exports core reflection data, textures and the ZIP; functions,
data-asset/curve indexes and flattened classes require their explicit flags. Bare `/kdf-reflection`
enables the full profile shown above. Supplying any chat option uses only the options supplied.

Options: `--out <dir>`, `--mounts /Game,/Plugin`, `--assets`, `--asset-cap N`, `--functions`,
`--flattened`, `--widgets` (widgets skipped by default), `--budget-ms N` (default `8`),
`--batch-size N`, `--families name:/Script/Module.Class,...`, and client-only override `--force`.
`--image-max-dimension N` caps the largest exported texture side in pixels; default `1024`,
accepted range `1` to `4096`.
`--pack` remains accepted for compatibility; ZIP packaging is always enabled.
Automated runtime QA can pass `-KDFReflectionAutoExport=<dir>`; it waits for `BeginPlay` and runs the full
`--assets --functions --flattened` profile.

Output contains manifest; class, struct, and enum NDJSON; subclass, asset, and native/Blueprint
indexes; `indexes/textures.json`; PNG files below `images/`; linter-compatible
`catalog/instanced_classes.json`; JSON Schema; optional flattened classes;
and one `<ISO-date>.kdfdata` ZIP. In desktop editor open **Datapacks**, import generated archive,
then bind imported game-data pack to project; Pack Manager exposes it as datapack `reflection` section.
Loose files serve development/debugging. Texture selection scans reflected non-widget class defaults recursively.
This retains descriptor, item, recipe, and schematic icons while excluding material-only maps, widget-only
decoration, unreferenced helpers, and dynamic render/media textures. Selected textures are indexed with Asset
Registry tags, image path/export status, and without applying `--asset-cap`; `--assets` enables indexes for other
asset classes. All exported images are deterministic PNG; cooked BC/BC7 data is decoded before writing.
Images exceeding the configured `--image-max-dimension` cap use a proportionally scaled image or
cooked mip. The cap does not upscale images. Missing or unreadable higher mips can produce smaller
output even when the source fits within the cap. Records retain source and exported dimensions plus
`resized`. Cooked-decode and GPU-readback fallbacks warn when using a lower mip than requested;
the raw `platformMipPng` path does not. Stripped/virtual/unsupported payloads keep an explicit
per-asset error. Each encoded PNG is limited to 4 MiB (4,194,304 bytes). Larger previews are not
written; their texture metadata remains indexed with `status: error`, `errorCode: encodedImageTooLarge`,
and an increased image error count. Reduce `--image-max-dimension` and rerun the export to retry.
Exporter
attempts to load every previously unloaded registry asset under selected mounts in bounded batches using
quiet/no-warn load flags, including `UWorld` map assets; failures remain in `manifest.json.failedLoads`.
Garbage collection is requested through engine safe points rather than called inside exporter ticker.
Properties whose constructors produce changing GUIDs, timestamps, seeds, transient object names, runtime command
lines, or uninitialized bytes are zero-normalized and marked `defaultVolatile: true`; stable defaults stay intact.
Blueprint-authored member names and serialized `FName` values use deterministic casing. Native type casing is
recovered from reflected C++ property types when Unreal's global `FName` registration order changed its display form.
Transient runtime textures are excluded because they have no portable asset identity and their pixels can change
between processes.

Runtime export prepares output in a sibling `.<output-name>.kdf-export-<id>.stage` directory,
then publishes generated files individually with the new archive last. Do not import or modify the
destination until completion. Before publication it saves affected previous files beneath
`.<output-name>.kdf-export-<id>.stage.backup/previous/` and records `destination`, `archive`,
`previousFiles`, `newFiles`, and `newDirectories` in that backup's `recovery.json`.
A publication error attempts rollback and retains recovery files. Process crashes can require
**manual rollback**; there is no automatic restart recovery.

The sibling `.stage.backup.committed` marker records completed publication and is removed after
backup cleanup. It does not prove artifact validity or a complete remaining backup. A missing marker
can also mean its write failed after successful publication. Stop all writers, preserve copies of the
destination and all matching transaction artifacts, inspect journal/logs, and validate before recovery.
To roll back, first verify every `previousFiles` backup exists; remove only destination `newFiles`,
remove only empty `newDirectories` deepest first, then copy `previous/` files back to their matching
relative paths, restoring the previous archive last. Keep backup sources intact, compare restored
bytes, and validate the previous complete export before deleting recovery artifacts. If no previous
complete export existed, verify prior state and export afresh into a new empty directory.
See [runtime publication and recovery](https://docs.k-mods.com/kdataforge/reference/reflection-export/#runtime-export-publication-and-recovery)
for path checks and incomplete-backup handling. Runtime transaction names differ from metadata-merge
`.<output-name>.kdf-merge-<id>.stage` and `.backup` directories; their recovery procedures differ.

Shipping builds often strip reflection metadata. Generate supplement from modding editor:

```powershell
UnrealEditor-Cmd.exe FactoryGame.uproject -run=KDFReflectionMetaExport -out=C:\Temp\KDFMeta
UnrealEditor-Cmd.exe FactoryGame.uproject -run=KDFReflectionMetaExport `
  -runtime=C:\Path\ReflectionExport `
  -supplement=C:\Temp\KDFMeta\metadata.supplement.json `
  -out=C:\Path\ReflectionMerged
```

Merge runs natively inside Unreal; no Python files enter mod or `.kdfdata` packages. Add `-force` only to replace
an existing merge output directory. Known limits: unloadable assets appear in `manifest.json.failedLoads`; registry indexing caps non-texture
classes, while asset loading itself covers every selected registry entry; full output can be large. Merge tool
always rebuilds ISO-named `.kdfdata` ZIP.

Acceptance commands:

```powershell
KDF.ValidateReflection --out C:\Path\ReflectionExport
python -m pip install jsonschema
python Mods/GameFeatures/KDataForge/Tools/validate_reflection_export.py `
  C:\Path\ReflectionExport-1 --compare C:\Path\ReflectionExport-2
```

`KDF.ValidateReflection` checks live FactoryGame reflection contracts, authored struct member names,
and deterministic default `ExportText`/`ImportText` samples (up to three per property kind). For the
completed export it checks required-file presence, native/Blueprint mapping, basic consistency of
optional settings/icon indexes and the cold-property catalog, and the presence of exactly one
`.kdfdata` archive.

Run the Python artifact/package validator for emitted-schema
validation of exported JSON documents and NDJSON records, PNG integrity and texture-index consistency,
and available settings/icon/cold-property catalog checks. It verifies the archive's ISO filename,
sorted entries, ZIP CRCs, inclusion of every generated file, and byte-for-byte agreement between each
packed entry and its loose file. ZIP entries and indexed image paths must use safe relative paths
contained within the export directory; traversal paths are rejected. The validator rejects symlinks
and Windows reparse points anywhere in the loose export tree, including unrelated custom entries.
Unrelated regular custom files outside `images/` may remain outside the archive.
It also checks numeric integer `schemaVersion: 1` and mirrors the editor's archive
inspection gates: source archive at most 2 GiB, at most 20,000 entries, at most 4 GiB expanded,
`manifest.json` at most 1 MiB, and each PNG within the 4 MiB preview limit.
Runtime acceptance alone does not validate ZIP contents, images, or every record against the schema.
The editor can compact source archives larger than 512 MiB and rejects installed output that still
exceeds its 512 MiB cache cap. Python PASS does not predict compacted-cache success or prove every
desktop editor import gate; confirm acceptance through the desktop editor's **Import game data**.
With `--compare`, Python validates both exports and compares
SHA-256 hashes of loose files, excluding `.kdfdata` archives and normalizing `manifest.json` with only
`generatedAt` removed.

{{GALLERY}}

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Technical</strong></td></tr></table>

- Reflection-based property engine: supports every reflectable UObject property type, including Blueprint structs and PrimaryDataAssets
- Fixed stage pipeline (tags → assets → localization → CDO changes → content → validation)
- Vanilla value snapshots power editor diffs, undo support, and changes-only export
- Extensible C++/Blueprint handler API for other mods (`IKDFDataEditorHandler`)
- Dedicated server compatible

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Requirements</strong></td></tr></table>

| Dependency | Link                                     |
| ---------- | ---------------------------------------- |
| SML        | [ficsit.app](https://ficsit.app/mod/SML) |

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
    <td colspan="2" style="padding:3px 4px;border:none"><a href="https://docs.k-mods.com/kdataforge/"><img src="https://img.shields.io/badge/KDataForge%20Documentation-1f8acb?style=for-the-badge" height="28" alt="KDataForge Documentation" /></a></td>
  </tr>
  </table>
</td>
</tr>
</table>
