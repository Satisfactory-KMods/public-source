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
  <a href="https://k-mods.com/production-modeler"><img src="https://img.shields.io/badge/SF%2B-Production%20Modeler-e8a202?style=flat-square" alt="SF+ Production Modeler" /></a>
  &nbsp;
  <a href="https://k-mods.com/wiki"><img src="https://img.shields.io/badge/SF%2B-Wiki-e8a202?style=flat-square" alt="SF+ Wiki" /></a>
</td>
</tr>
</table>

{{IMPORTANT_NOTE}}

{{INFORMATION}}

---

KAPI is a shared dependency for KMods. Players do not need to configure it; Satisfactory Mod Manager installs it with mods that require it. Mod authors can use its data assets to configure KMods machines without changing C++ code.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">What KAPI does</strong></td></tr></table>

KAPI provides a central `UKAPIDataAssetSubsystem` (a `GameInstanceSubsystem`). At startup it scans the Unreal Engine Asset Registry, discovers registered data assets, resolves priorities, and shares the results with other KMods. Data assets hold configuration for miners, cleaners, air collectors, slug breeders, manufacturer changes, delivery tasks, FaxIT capacity rules, and UI injectors.

Third-party mods can extend KMods behavior by creating data assets derived from `UKAPIDataAssetBase`. This does not require changes to KMods source code.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Data asset base: `UKAPIDataAssetBase`</strong></td></tr></table>

Every KAPI config asset inherits from this base class, which provides:

| Field | Type | Purpose |
|---|---|---|
| `mName` | `FText` | Display name shown in UI |
| `mDescription` | `FText` | Multi-line description shown in UI |
| `mIcon` | `UTexture2D` | Icon used in UI |
| `mIsDisabled` | `bool` | Disable this asset at runtime (e.g. to suppress another mod's content) |
| `mPriority` | `int` | Sort order: higher values win when multiple assets compete |
| `mTags` | `FGameplayTagContainer` | Tags used for filtering, feature flags, and integration rules |

Override `IsEnabled_Internal` for custom enable/disable logic beyond the simple flag.

KAPI also exposes Blueprint helpers for reading and changing gameplay-tag containers. They work with KAPI data assets, item descriptors, recipes, schematics, research trees, classes, and objects that implement Unreal's gameplay-tag interface.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Modular Miner: `UKAPIModularMinerDescription`</strong></td></tr></table>

This data asset describes how the KMods Modular Miner handles a resource node.

- Maps each **waste producer module type** (`UKAPIWasteProducerType`) to its own production item and trash item via `FKAPIModuleItems`
- Specifies which **attachment modules** (`UKAPIModularAttachmentDescriptor`) are required or forbidden
- Configures **fluid modules** with per-item flow rates and production time multipliers
- Carries UI metadata: rarity text, found text, and a node screenshot texture
- `mDrillTier` controls which tier of drill head is needed to mine this resource
- `mIsFallbackDescriptor` marks the asset as the default when no specific asset matches

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Cleaner: `UKAPICleanerItemDescription`</strong></td></tr></table>

Defines one cleaning recipe for the KMods fluid-cleaner machine.

- Input and output fluid amounts (`mInFluid`, `mOutFluid`) drive pipeline flow
- Optional cleaner-item consumption (`mCleanerItemInfo`): the machine can require a consumable item alongside the fluid
- Bypass products (`mBypassProducts`) let content bypass normal cleaning and produce alternative outputs
- `bSkipUnlockCheck` makes the recipe available without a Milestone unlock

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Air Collector: `UKAPIAirCollectorData`</strong></td></tr></table>

Configures one collection recipe for the KMods Air Collector, which harvests items from the surrounding environment.

- **World scan**: specify actor classes, scan radius (`mRangeToScan`), trace channels, and whether resource nodes count as hits
- **Height-based production**: yield scales with altitude (`bUseHightBasesdProduction`), with min, max, and base values per scan cycle
- **Hit-based production**: fixed output per detected actor hit (`mProductionPerHit`, `mMaxHit`)
- Recipe priority (`mRecipePrio`) resolves conflicts when multiple assets match the same biome

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Slug hatching: `UKAPISugHatchingData`</strong></td></tr></table>

Full breeding and incubation profile for one slug type in the KLib slug system.

- **Breeding**: food item, consumption rate, breeding duration, die timer, and egg count per cycle
- **Incubation**: hatch duration, power draw, incubator tier, optional fluid requirement (fluid class, tank tier, volume, consume interval)
- **Probability rolling**: each `FKAPISlugIncubation` entry carries a probability, a fixed-chance accumulator (`mFixedChanceMultiplier`), and a production count. Slugs that fail to roll build up guaranteed-success chance over time
- **Environment comfort**: temperature range, humidity range, and time-of-day preference (`EKAPISlugTime`) gate breeding efficiency
- Tier system (`mSlugTier`) determines priority when two slug types compete for environment resources

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Manufacturer modifications: `UKAPIManufacturerModifications`</strong></td></tr></table>

This asset changes inventory slots on an existing vanilla or modded manufacturer at runtime. It does not require a Blueprint subclass.

- Targets any `AFGBuildableManufacturer` class (optionally including subclasses via `bApplyOnSubclasses`)
- Per-slot icon overrides for solid and fluid inputs/outputs (`mFluidSlotIcons`, `mSolidSlotIcons`)
- Inventory size multipliers for solids, fluids, and gases independently
- `bAllowMoreInputSlots` unlocks additional input slots when a recipe needs them

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Extractor allow list: `UKAPIExtractorAllowList`</strong></td></tr></table>

Fine-grained whitelist/blacklist control over which resource extractors are allowed on which resource nodes.

- `mAllowedExtractors` / `mDisallowExtractors`: extractor class filters
- `mAllowedResources` / `mDisallowResources`: resource descriptor filters

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Tooltip widget injector: `UKAPTooltipWidgetInjector`</strong></td></tr></table>

Injects a custom UMG widget into item tooltips for a filtered set of item descriptors.

- `mWidgetClass`: the widget to inject
- `mClassFilter`: set of item descriptor classes that trigger the injection
- `mInjectAtIndex`: insertion position within the tooltip panel (`-1` appends at the end)

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Delivery tasks: `UKAPIDeliveryTask`</strong></td></tr></table>

Defines nodes for the KMods delivery tree. A task can use fixed costs and rewards or generate repeatable requests from an item pool.

- Tree coordinates and parent-direction flags control placement and prerequisites
- One-time tasks can grant items, schematics, unlock objects, ADA messages, and completion toasts
- Endless tasks support item pools, mandatory entries, task-count requirements, amount steps, reward tiers, and scaling curves
- Availability dependencies can gate tasks with the same dependency system used by Satisfactory schematics

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">FaxIT byte usage: `UKAPIFaxitByteUsageDefinition`</strong></td></tr></table>

Defines one tagged FaxIT capacity category. Each unit can consume bytes, add power demand, change a displayed value, apply per-drive limits, and set reclaim priority when capacity must be reduced.

---

<table width="100%" cellpadding="0" cellspacing="0"><tr><td style="background:#e8a202;padding:6px 14px;border-radius:6px 6px 0 0;border:none"><strong style="color:#1a1a2e;font-size:18px">Descriptors</strong></td></tr></table>

| Class | Role |
|---|---|
| `UKAPIModularAttachmentDescriptor` | Abstract tag class identifying a module attachment type for the modular miner |
| `UKAPIWasteProducerType` | Abstract tag class classifying waste output of a miner module |

Both are `Abstract` and `Blueprintable`: create Blueprint subclasses to define new module and waste categories without C++.

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
    <td style="padding:3px 4px;border:none"><a href="https://k-mods.com/production-modeler"><img src="https://img.shields.io/badge/SF%2B%20Modeler-e8a202?style=for-the-badge" height="28" alt="SF+ Production Modeler" /></a></td>
  </tr>
  <tr>
    <td colspan="2" style="padding:3px 4px;text-align:right;border:none"><a href="https://k-mods.com/wiki"><img src="https://img.shields.io/badge/SF%2B%20Wiki-e8a202?style=for-the-badge" height="28" alt="SF+ Wiki" /></a></td>
  </tr>
  </table>
</td>
</tr>
</table>
