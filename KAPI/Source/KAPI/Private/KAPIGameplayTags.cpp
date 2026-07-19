// ILikeBanas

#include "KAPIGameplayTags.h"

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_ExcludeFromNodeRandomization, "KMods.ExcludeFromNodeRandomization",
							   "Resource descriptors with this tag are excluded from resource node randomization.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_Disabled, "KMods.Disabled",
							   "Treats a KAPI data asset as disabled, equivalent to setting mIsDisabled.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_AlternateRecipe, "KMods.AlternateRecipe",
							   "Marks a recipe as an alternate recipe (wiki), independent of the unlocking schematic.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_PlannerExclude, "KMods.PlannerExclude",
							   "Excludes an item, building or recipe from the wiki production planner.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(
	TAG_KMods_PlannerExcludeOverclocking, "KMods.PlannerExcludeOverclocking",
	"Excludes a recipe from overclocking in the production planner; its clock remains capped at 100%.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_PlannerResource, "KMods.PlannerResource",
							   "Marks an item as a raw input in the production planner, treated like a raw resource.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_MinerModuleCrusher, "KMods.MinerModule.Crusher",
							   "Marks a Modular Miner extraction recipe as using the Crusher module.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_MinerModuleSmelter, "KMods.MinerModule.Smelter",
							   "Marks a Modular Miner extraction recipe as using the Smelter module.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_MinerModuleFluid, "KMods.MinerModule.Fluid",
							   "Marks a Modular Miner extraction recipe as using the Fluid module.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_WikiExclude, "KMods.WikiExclude",
							   "Skips this content entirely in the wiki export.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_Deprecated, "KMods.Deprecated",
							   "Legacy content kept only for save compatibility; hidden/badged on the wiki.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_WIP, "KMods.WIP",
							   "Unfinished content that may change; badged as work-in-progress on the wiki.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_Cheat, "KMods.Cheat",
							   "Cheat / debug / developer content outside normal progression.");

UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_KMods_EventOnly, "KMods.EventOnly",
							   "Content only obtainable during time-limited events (e.g. FICSMAS).");
