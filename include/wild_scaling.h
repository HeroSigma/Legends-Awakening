#ifndef GUARD_WILD_SCALING_H
#define GUARD_WILD_SCALING_H

#include "global.h"
#include "constants/wild_scaling.h"
#include "constants/rtc.h"
#include "wild_encounter.h"

struct WildScalingBand
{
	u8 minLevel;
	u8 maxLevel;
};

u16 GetWildScalingLevelForMap(u16 mapGroup, u16 mapNum);
u16 GetCurrentWildScalingLevel(void);
s16 GetWildScalingPartyAverage(void);
s16 GetWildScalingMapModifier(u16 mapGroup, u16 mapNum);
u16 ApplyWildLevelScaling(u16 species, u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum);
u16 ApplyWildLevelScalingWithBaseLevel(u16 species, u16 originalLevel, u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum);
u16 ResolveScaledWildSpecies(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum);
u16 ResolveScaledWildSpeciesForContext(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum, enum TimeOfDay timeOfDay, u16 profile);
u16 ResolveScaledWildSpeciesForArea(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum, enum WildPokemonArea area);
u16 GetWildScalingDebugTarget(u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum);
void Script_InspectWildScaling(void);

#if TESTING
u16 CalculateWildScalingWorldLevel(u8 trainerRank, u16 worldPhase, const u8 *partyLevels, u8 partyCount);
u16 TestApplyWildLevelScaling(u16 originalMinLevel, u16 originalMaxLevel, u16 worldLevel, s16 mapModifier, u16 randomValue);
#endif

#endif // GUARD_WILD_SCALING_H