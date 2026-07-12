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

KBFL (KMods Blueprint Function Library) is the shared foundation library used by all KMods — providing asset scanning, CDO overwriting, custom swatch replication, resource node spawning, and a suite of Blueprint-exposed utilities that any other mod can build on.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Asset Data Subsystem</strong></td></tr></table>

`UKBFLAssetDataSubsystem` (GameInstanceSubsystem) scans the full asset registry at startup and categorises every discovered class into fast, queryable sets — partitioned per mod.

- Discover all **items**, **recipes**, **schematics**, **buildables**, **holograms**, **research trees**, **driveable pawns**, **mod modules**, and **session settings** across every installed mod
- Filter items by **resource form** (solid, liquid, gas, …) or by parent class hierarchy
- Filter recipes by **producer class**; look up building descriptors and vehicle descriptors
- Per-mod data buckets via `GetModRelatedData` — lets a mod introspect its own registered content at runtime
- Template helpers `FindAllDataAssetsOfClass` / `FindFirstDataAssetsOfClass` for `UPrimaryDataAsset` lookups without touching the registry directly

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">CDO Overwrite System</strong></td></tr></table>

A data-asset-driven pipeline that lets mods modify Class Default Objects without touching a single line of C++ or Blueprint logic in the target mod.

- **`UKBFLCDOOverwrite`** — point at any target class; configure property values in the editor; KBFL applies the diff at the right lifecycle phase
- Supports all property kinds: arrays, sets, maps, integers, floats, booleans, strings, texts, names, objects, classes, structs, enums
- Per-property **merge behaviors**: Replace, Merge, MergeUnique for collections; Add / Subtract / Multiply / Divide / Clamp for numerics; Append / Prepend for strings and names
- **Subclass broadcast**: optionally cascade changes to all Blueprint subclasses, with include/exclude lists and native-class filters
- **`UKBFLWorldCDOSubsystem`** handles world-phase CDO overwrites (applied after save load, before BeginPlay) with optional per-tick updates
- **`UKBFLCDOItemStackSize`** — data asset for batch-resizing item stack sizes; maps `EStackSize` to a list of item descriptor classes

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Swatch &amp; Customizer System</strong></td></tr></table>

Stable, save-safe custom paint swatch registration with full multiplayer replication.

- **`AKBFLSwatchReplicationSubsystem`** — server-authoritative swatch ID assignment; persists assignments across save/load via `IFGSaveInterface`; replicates the full color-slot table to every client via `OnRep_ColorSlots`
- Collision-free ID allocation: existing in-world swatches keep their IDs; new swatches fill gaps automatically
- **`UKBFLCustomizerSubsystem`** — registers custom swatch descriptors and swatch groups into the base-game customizer at world init
- **`UKBFLCustomizationProvider`** data asset — declarative registration of swatches, swatch groups, and material descriptor overrides without any Blueprint logic
- **`UKBFLColorRCO`** — RCO for client-side swatch name edits forwarded reliably to server

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Resource Node Spawning</strong></td></tr></table>

Data-asset-driven actor spawning tied to the world lifecycle — used to add or replace resource nodes and resource wells without modifying the level.

| Class | Purpose |
|---|---|
| `UKBFLResourceNodeSubsystem` | World subsystem; drives sub-level and descriptor spawning on `OnWorldBeginPlay` |
| `UKBFLActorSpawnDescriptorBase` | Abstract data asset; configurable spawn requirements, overlap checks, move/remove policies |
| `UKBFLResourceNodeDescriptor` | Concrete descriptor for solid resource nodes — set resource class, purity, and amount |
| `UKBFLResourceNodeDescriptor_ResourceNode` | Extended node descriptor with transform array support |
| `UKBFLResourceNodeDescriptor_ResourceWell` | Descriptor variant for resource wells |
| `UKBFLSubLevelSpawning` | Sub-level streaming helper for large-scale node layouts |
| `UKBFLSpawnRequirement` | Pluggable requirement object — override `IsRequirementMet` in Blueprint to gate spawning on any condition |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Blueprint Function Libraries</strong></td></tr></table>

A set of `UBlueprintFunctionLibrary` classes exposing commonly needed operations to both C++ and Blueprint authors.

| Library | Key Functions |
|---|---|
| `UKBFL_Asset` | `MarkDefaultDirty`, `HasProducer` |
| `UKBFL_Util` | `GetSubsystem` / `GetSubsystemFromChild`, `SortItemArray`, `DoPlayerViewLineTrace`, `DoPlayerViewLineTraceSphere` |
| `UKBFL_Player` | `GetBuildingGun`, `GetFGController`, `GetFGCharacter`, `GetFgPlayerState`, `GetBuildingGunHitResult`, `GetPlayerBuildState` |
| `UKBFL_Math` | `CalculateOrbitPoints`, `CalculateItemsPerMin`, `CalculateM3PerMin` (belt and pipe rate helpers) |
| `UKBFL_ConfigTools` | Type-safe get/set for Bool, Float, Int, String, Text, Name, Class from any `UModConfiguration` |
| `UKBFLCppInventoryHelper` | `CanStoreItem`, `StoreItemStack`, `AddStacksInInventory`, `PullBelt`, `PullPipe`, `PushPipe` — inventory I/O helpers used inside `Factory_Tick` |

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Lifecycle Module Base Classes</strong></td></tr></table>

`UKBFLGameInstanceModule` and `UKBFLWorldModule` extend SML's module system with structured construction / init / post-init lifecycle events, CDO registration hooks, and optional asset-registry auto-scan.

- Expose `ConstructionPhase`, `InitPhase`, `PostInitPhase` as BlueprintNativeEvents — no C++ required for most mods
- Optional `mUseAssetRegistry` flag triggers automatic scanning and CDO/schematic/recipe/research-tree registration

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Utility Buildables &amp; Holograms</strong></td></tr></table>

- **`AKBFLUtilItemCreaterBuildable`** — cheat/debug buildable that generates arbitrary items on belts and pipes; fully replicated via `UKBFLDefaultRCO`
- **`AKBFLUtilHologram`** — placement hologram with free transform (offset, rotation, scale) driven by `AKBFLLocationSubsystem`; shows a live widget overlay during placement
- **`AKBFLLocationSubsystem`** — saves and restores actor transforms to a file for iterative level design

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">C++ Data Structures</strong></td></tr></table>

- **`FSortedActorDistanceArray<T>`** — template proximity container; `Sort()` orders actors by distance to a reference actor, `PopClosestValid()` pops the nearest one; used for efficient proximity-based loading
- **`UKBFLCppInventoryHelper`** — all-static belt/pipe pull/push helpers that work directly with `UFGInventoryComponent` and connection components

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
