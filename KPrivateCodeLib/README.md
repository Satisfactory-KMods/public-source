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

KPrivateCodeLib is the shared C++ foundation that powers every KMods building — base classes, the FaxIT item network, modular assembly, fluid valve, subsystems, and replication utilities, all bundled in one dependency mod.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Base Buildable Classes</strong></td></tr></table>

All KMods machines derive from one of three abstract base classes, each of which extends the corresponding Satisfactory factory type and adds a consistent set of shared features.

| Class | Extends | Use case |
|---|---|---|
| `AKPCLProducerBase` | `AFGBuildableFactory` | Producers, cleaners, air collectors, slug breeders |
| `AKPCLExtractorBase` | `AFGBuildableResourceExtractor` | Miners, pumps, all resource extractors |
| `AKPCLBuildableManufacturerBase` | `AFGBuildableManufacturer` | Manufacturers and crafting machines |

### Shared features across all base classes

- **`FPowerOptions`** — configurable power curve with optional variable-power ramp, exponent scaling, and a runtime multiplier. Supports both consumer and dynamic-producer modes.
- **`FFullProductionHandle`** — production timer with built-in productivity tracking (rolling 5-minute window), pending-potential support, and a fire-and-forget callback on cycle completion.
- **`UKPCLBetterIndicator`** — custom status indicator component that drives per-state LED colours and pulse animations for up to 11 production states (`Idle`, `Producing`, `Paused`, `Error`, etc.).
- **Inventory helpers** — typed accessors for input, output, and booster inventories with overridable filter callbacks; connection map for belt and pipe I/O.
- **`AIO_*` instance data API** — runtime mesh-slot overrides (swap `UStaticMesh`, set custom float channels as colour, hide/show, transform) that replicate to clients.
- **Clipboard support** — copy/paste production settings (pause state, target potential, power shards) between buildings of the same type.
- **Audio config helper** — `FKPCLModConfigHelper_Float`-driven volume control for all attached audio components, updated live from mod configuration changes.
- **`Factory_TickAuthOnly` / `Factory_TickClientOnly`** — authority- and client-only tick splits, called automatically from the shared `Factory_Tick` override.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Modular Building System</strong></td></tr></table>

KPrivateCodeLib provides a full snap-point module system so buildings can accept physically-attached upgrade modules.

- **`AKPCLModularBuildingBase`** — producer variant; extends `AKPCLProducerBase` and `IKPCLModularBuildingInterface`.
- **`AKPCLModularExtractorBase`** — extractor variant; extends `AKPCLExtractorBase` and `IKPCLModularBuildingInterface`.
- **`UKPCLModularBuildingHandler`** — manages up to 10 attachment-type slots, each with an array of world-space snap transforms. Tracks which modules are snapped at which positions; fully saved and replicated.
- **`UKPCLModularBuildingHandlerStacker`** — stacking-variant handler for vertically stacked modules.
- **`KPCLModularSnapPoint`** — component that marks valid snap locations on a module.
- **`KPCLModularHologram`** — placement hologram that previews snap-point alignment before the module is built.

### How it works

1. The master building holds a `UKPCLModularBuildingHandler` component with typed `FAttachmentInfos` entries defining which `UKAPIModularAttachmentDescriptor` class each slot accepts and the allowed world-space snap positions.
2. When a module is placed, its hologram finds the nearest free snap transform via `GetSnapPointInRange` and calls `AddNewActorToAttachment` on the handler.
3. `OnModulesUpdated` fires on master and module, triggering mesh updates and power recalculation.
4. Dismantling the master optionally recurses to all child modules (`mDismantleAllChilds`).

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">FaxIT Item Network</strong></td></tr></table>

FaxIT is a wireless item-transfer network. Buildings connect to a named network core over cable, then upload and download items without physical conveyors.

### Network buildings

| Class | Role |
|---|---|
| `AKPCLNetworkCore` | Hub: stores items, drives limits, exposes stats |
| `AKPCLNetworkBuildingBase` | Base for all network-connected buildings |
| `AKPCLNetworkConnectionBuilding` | Depot/terminal with item filter and speed override |
| `AKPCLNetworkTerminal` | Player-interactive access terminal |
| `AKPCLNetworkPole` / `AKPCLNetworkTower` | Range extenders and radar-tower integration |
| `UKPCLNetwork` | Custom `UFGPowerCircuit` subclass that forms the cable subnet |

### Core capabilities

- **Named networks** — each `AKPCLNetworkCore` gets a GUID-based `mNetworkId`; buildings join by cable proximity.
- **Drive-based capacity** — unique item slots and stack limits scale with the number of installed drives; buildings-per-network limit also scales.
- **Distance power cost** — `FKPCLNetworkDistanceModifier` adds extra power consumption proportional to cable length beyond a safe radius.
- **Per-building statistics** — `FKPCLFaxitNetworkStatData` records upload/download counts per item; the core archives 60-second bundles (`FKPCLFaxitNetworkStatDataBundle`).
- **Thread-safe storage** — `FKPCLMappedItemAmount` wraps an item→count `TMap` behind an `FRWLock`, with a separate replicated `TArray` for net serialisation.
- **`AKPCLFaxitSubsystem`** — world-level manager; handles network creation/merge/destruction, tier unlock state, solid/fluid speed levels, and per-minute throughput queries.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Subsystems</strong></td></tr></table>

| Subsystem | Type | Purpose |
|---|---|---|
| `AKPCLFaxitSubsystem` | `AKPCLModSubsystem` | FaxIT network registry and unlock state |
| `AKPCLSwatchSystem` | `AKPCLModSubsystem` | Saved custom swatch presets (name + primary/secondary colour), replicated to all clients |
| `AKPCLUnlockSubsystem` | `AKPCLModSubsystem` | Schematic-unlock listener, loot chest registry |
| `AKPCLOutlineSubsystem` | `AKPCLModSubsystem` | Post-process outline rendering with 5 colour slots and fill/outline/combined modes; RCO-driven multicast |
| `UKPCLPatreonSubsystem` | `UGameInstanceSubsystem` | Optional Patreon integration (configurable; controls list, button, and news-feed visibility) |

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Pressure Regulator Valve</strong></td></tr></table>

`AKPCLPressureRegulatorValve` is a fill-level-controlled bidirectional pipe valve that gates fluid flow without a pump motor.

- Extends `AFGBuildablePipelinePump` with headlift disabled (`SetMaxHeadLift(0, 0)`) so fluid moves purely on network pressure.
- **Hysteresis logic** — two independently configurable normalised thresholds:
  - Fill ≥ `mOnThreshold` → open (`SetUserFlowLimit(mMaxFlowRate)`)
  - Fill ≤ `mOffThreshold` → close (`SetUserFlowLimit(0)`)
  - Between thresholds → hold current state
- `bInvertThresholdBehavior` swaps the open/close semantics for drain-on-high use cases.
- All three threshold properties are `SaveGame`, `FGReplicated`, and editable in Blueprint defaults.
- `mValveOpenAlpha` (0–1) drives mesh animation; `OnValveStateChanged` and `OnUiRequireUpdate` are Blueprint implementable events.
- RCO methods on `UKPCLDefaultRCO` (`Server_Valve_Set*`) allow client-side UI to change thresholds safely; the setters dispatch internally.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Balance Splitter</strong></td></tr></table>

`AKPCLBuildableBalanceSplitter` extends `AFGBuildableConveyorAttachment` with per-output item filtering and rate capping.

- Three configurable modes: **Normal** (round-robin), **Smart** (overflow-aware), and **Programmable** (explicit items-per-minute per output).
- `FKPCLSplitterTimer` per output tracks target rate, current timer, item filter list, and wildcard/overrule flags.
- Clipboard support copies and pastes all output rules at once.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Outline System</strong></td></tr></table>

`AKPCLOutlineSubsystem` renders post-process outlines around arbitrary actors using a material parameter collection.

- Five independent colour slots (`EOutlineColorSlot`) with configurable default colours.
- Three outline modes: `Fill`, `Outline`, and `OutlineAndFill`.
- All operations available locally or via `NetMulticast` reliable RPCs for server-driven highlights.
- Paired `UKPCLDefaultRCO` methods (`Server_CreateOutlineForActor`, `Server_ClearOutlines`, `Server_SetOutlineColor`) allow clients to request server-authoritative outline changes.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Replication &amp; RCO</strong></td></tr></table>

`UKPCLDefaultRCO` is the central Remote Call Object for all KMods buildings.

- Retrieve with `UKPCLDefaultRCO::Get(WorldContext)` from any `UObject` with a valid world.
- Covers: fluid flush, swatch mutations, loot chest interaction, item moves between inventories, all FaxIT operations (filter item, speed override, grab/store, rename network), pressure valve threshold changes, and balance splitter rule updates.
- All `_Validate` stubs return `true` by default; override in subclass for input sanitisation.
- Buildings dispatch to the RCO internally in their setters — callers never need to check `HasAuthority()`.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Blueprint Function Library</strong></td></tr></table>

`UKPCLBlueprintFunctionLib` provides utility functions callable from C++ and Blueprint.

- `InjectWidgetAt` — insert a widget into a panel at a specific index with padding and optional `TFITCheck`.
- `FindChildWidgetsOfClassDeep` — recursive deep search for all child widgets of a given class.
- `GetClostestActor<T>` — template nearest-actor search with optional range cap and predicate filter.
- `DestroyAndRemoveFromReplicationGraph` — clean actor teardown that correctly unregisters from the replication graph.
- `UpdateIconLib` — add or remove items from a `UFGIconLibrary` at runtime.
- `ResolveHitResult` / `ResolveOverlapResult` — unwrap hit and overlap results to their owning actors.

---

{{GALLERY}}

---

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
