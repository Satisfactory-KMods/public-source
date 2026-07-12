// ILikeBanas

#pragma once

#include "CoreMinimal.h"

#include "NativeGameplayTags.h"

/**
 * Native KMods gameplay tags.
 *
 * All shared "KMods.*" tags live here in KAPI so every KMod (and third-party mods integrating with
 * KMods) can reference them without additional dependencies. Assign them in the editor on item
 * descriptors, building descriptors, recipes or schematics via their gameplay tag containers.
 *
 * All tags are exported by the SatisfactoryPlus wiki extractor in each entity's "tags" array;
 * the tags below additionally drive dedicated behavior where noted.
 */

// --- Gameplay ---------------------------------------------------------------------------------

/** Resource descriptors carrying this tag are excluded from resource node randomization. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_ExcludeFromNodeRandomization);

/**
 * KAPI data assets carrying this tag are treated as disabled — equivalent to setting mIsDisabled.
 * Checked by UKAPIDataAssetBase::IsEnabled_Internal.
 */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_Disabled);

// --- Wiki export ------------------------------------------------------------------------------

/**
 * Recipes carrying this tag are alternate recipes. This is the single source of truth for the wiki
 * export — the unlocking schematic type is irrelevant.
 */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_AlternateRecipe);

/** Items, buildings and recipes carrying this tag are excluded from the wiki production planner. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_PlannerExclude);

/** Recipe is excluded from overclocking in the production planner; its clock remains capped at 100%. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_PlannerExcludeOverclocking);

/** Marks a Modular Miner extraction recipe as using the Crusher module. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_MinerModuleCrusher);

/** Marks a Modular Miner extraction recipe as using the Smelter module. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_MinerModuleSmelter);

/** Marks a Modular Miner extraction recipe as using the Fluid module. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_MinerModuleFluid);

/** Content carrying this tag is skipped entirely by the wiki export. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_WikiExclude);

// --- Content classification (informational — exported in the "tags" array) ---------------------

/** Legacy content kept only for save compatibility; hidden or badged as deprecated on the wiki. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_Deprecated);

/** Unfinished content that may change; badged as work-in-progress on the wiki. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_WIP);

/** Cheat / debug / developer content that is not part of normal progression. */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_Cheat);

/** Content only obtainable during time-limited events (e.g. FICSMAS). */
KAPI_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_KMods_EventOnly);
