#include "global.h"
#include "event_data.h"
#include "pokemon.h"
#include "random.h"
#include "string_util.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "wild_scaling.h"
#include "dynamic_encounters.h"
#include "wild_encounter.h"
#include "constants/maps.h"

struct WildScalingMapModifier
{
    u16 mapGroup;
    u16 mapNum;
    s8 levelModifier;
};

struct WildScalingFamily
{
    u16 baseSpecies;
    u16 middleSpecies;
    u16 finalSpecies;
    u8 middleLevel;
    u8 finalLevel;
    u8 middleWeight;
    u8 finalWeight;
    u16 mapGroup;
    u16 mapNum;
    u16 profile;
    enum TimeOfDay timeOfDay;
};

static const struct WildScalingBand sWildScalingBands[WILD_SCALING_TIER_COUNT] =
{
    [WILD_SCALING_ROOKIE] = {1, 18},
    [WILD_SCALING_RISING] = {12, 30},
    [WILD_SCALING_ACE] = {24, 45},
    [WILD_SCALING_ELITE] = {38, 62},
    [WILD_SCALING_MASTER] = {52, 82},
    [WILD_SCALING_LEGEND] = {70, 100},
};

static const struct WildScalingMapModifier sWildScalingMapModifiers[] =
{
    {MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), -2},
};

static const struct WildScalingFamily sWildScalingFamilies[] =
{
    {SPECIES_TREECKO, SPECIES_GROVYLE, SPECIES_SCEPTILE, 20, 36, 25, 65, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1, TIME_DAY},
    {SPECIES_TREECKO, SPECIES_GROVYLE, SPECIES_SCEPTILE, 20, 36, 25, 65, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2, TIME_MORNING},
    {SPECIES_TREECKO, SPECIES_GROVYLE, SPECIES_SCEPTILE, 20, 36, 25, 65, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2, TIME_DAY},
    {SPECIES_MAGIKARP, SPECIES_NONE, SPECIES_GYARADOS, 0, 20, 0, 65, WILD_SCALING_ANY, WILD_SCALING_ANY, WILD_SCALING_ANY, TIMES_OF_DAY_COUNT},
    {SPECIES_WAILMER, SPECIES_NONE, SPECIES_WAILORD, 0, 40, 0, 65, WILD_SCALING_ANY, WILD_SCALING_ANY, WILD_SCALING_ANY, TIMES_OF_DAY_COUNT},
};

static u8 ClampTier(u16 tier)
{
    return tier < WILD_SCALING_TIER_COUNT ? tier : WILD_SCALING_LEGEND;
}

static u8 GetScalingTier(u8 trainerRank, u16 worldPhase)
{
    return ClampTier(max(trainerRank, worldPhase == WORLD_PHASE_BEGINNING ? WILD_SCALING_ROOKIE : worldPhase - 1));
}

static u8 ClampLevel(s32 level)
{
    return (u8)max(1, min(MAX_LEVEL, level));
}

static const struct WildScalingBand *GetCurrentBand(void)
{
    return &sWildScalingBands[GetScalingTier(GetTrainerRank(), GetWorldPhase())];
}

static u8 GetTopThreeAverage(const u8 *levels, u8 count)
{
    u8 top[3] = {0, 0, 0};
    u8 i;
    u8 j;
    u8 used = 0;

    for (i = 0; i < count; i++)
    {
        if (levels[i] == 0)
            continue;
        if (used < ARRAY_COUNT(top))
            top[used++] = levels[i];
        else if (levels[i] > top[2])
            top[2] = levels[i];
        else
            continue;
        for (j = used - 1; j > 0 && top[j] > top[j - 1]; j--)
        {
            u8 value = top[j];
            top[j] = top[j - 1];
            top[j - 1] = value;
        }
    }
    if (used == 0)
        return 0;
    return (top[0] + top[1] + top[2]) / used;
}

static u8 GetCurrentPartyAverage(void)
{
    u8 levels[PARTY_SIZE];
    u8 count = 0;
    u8 i;
    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (!GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG)
         && GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP) != 0)
            levels[count++] = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_LEVEL);
    }
    return GetTopThreeAverage(levels, count);
}

s16 GetWildScalingPartyAverage(void)
{
    return GetCurrentPartyAverage();
}

u16 CalculateWildScalingWorldLevel(u8 trainerRank, u16 worldPhase, const u8 *partyLevels, u8 partyCount)
{
    const struct WildScalingBand *band = &sWildScalingBands[GetScalingTier(trainerRank, worldPhase)];
    u8 partyAverage = GetTopThreeAverage(partyLevels, partyCount);
    u8 target = partyAverage == 0 ? band->minLevel : (band->minLevel + partyAverage * 2) / 3;
    return max(band->minLevel, min(band->maxLevel, target));
}

u16 GetCurrentWildScalingLevel(void)
{
    u8 levels[PARTY_SIZE];
    u8 count = 0;
    u8 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_IS_EGG))
            continue;
        if (GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_HP) == 0)
            continue;
        levels[count++] = GetMonData(&gParties[B_TRAINER_PLAYER][i], MON_DATA_LEVEL);
    }
    return CalculateWildScalingWorldLevel(GetTrainerRank(), GetWorldPhase(), levels, count);
}

s16 GetWildScalingMapModifier(u16 mapGroup, u16 mapNum)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sWildScalingMapModifiers); i++)
        if (sWildScalingMapModifiers[i].mapGroup == mapGroup && sWildScalingMapModifiers[i].mapNum == mapNum)
            return sWildScalingMapModifiers[i].levelModifier;
    return 0;
}

u16 GetWildScalingLevelForMap(u16 mapGroup, u16 mapNum)
{
    const struct WildScalingBand *band = GetCurrentBand();
    s32 target = GetCurrentWildScalingLevel() + GetWildScalingMapModifier(mapGroup, mapNum);
    return ClampLevel(min(band->maxLevel, max(band->minLevel, target)));
}

u16 TestApplyWildLevelScaling(u16 originalMinLevel, u16 originalMaxLevel, u16 worldLevel, s16 mapModifier, u16 randomValue)
{
    s32 original = (originalMinLevel + originalMaxLevel) / 2;
    s32 target = worldLevel + mapModifier + (original - worldLevel) / 4;
    target += (randomValue % 5) - 2;
    return ClampLevel(target);
}

u16 GetWildScalingDebugTarget(u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum)
{
    const struct WildScalingBand *band = GetCurrentBand();
    u16 target = TestApplyWildLevelScaling(originalMinLevel, originalMaxLevel,
        GetCurrentWildScalingLevel(), GetWildScalingMapModifier(mapGroup, mapNum), 2);
    return max(band->minLevel, min(band->maxLevel, target));
}

u16 ApplyWildLevelScaling(u16 species, u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum)
{
    return ApplyWildLevelScalingWithBaseLevel(species, (originalMinLevel + originalMaxLevel) / 2,
        originalMinLevel, originalMaxLevel, mapGroup, mapNum);
}

u16 ApplyWildLevelScalingWithBaseLevel(u16 species, u16 originalLevel, u16 originalMinLevel, u16 originalMaxLevel, u16 mapGroup, u16 mapNum)
{
    const struct WildScalingBand *band = GetCurrentBand();
    s32 target;
    (void)species;
    (void)originalMinLevel;
    (void)originalMaxLevel;
    target = GetCurrentWildScalingLevel() + GetWildScalingMapModifier(mapGroup, mapNum)
        + ((s32)originalLevel - GetCurrentWildScalingLevel()) / 4;
    target += (Random() % 5) - 2;
    return max(band->minLevel, min(band->maxLevel, ClampLevel(target)));
}

u16 ResolveScaledWildSpecies(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum)
{
    return ResolveScaledWildSpeciesForArea(species, effectiveLevel, mapGroup, mapNum, WILD_AREA_LAND);
}

u16 ResolveScaledWildSpeciesForArea(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum, enum WildPokemonArea area)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    enum TimeOfDay timeOfDay = GetTimeOfDayForEncounters(headerId, area);
    return ResolveScaledWildSpeciesForContext(species, effectiveLevel, mapGroup, mapNum,
        timeOfDay, GetDynamicEncounterProfileForMap(mapGroup, mapNum));
}

u16 ResolveScaledWildSpeciesForContext(u16 species, u16 effectiveLevel, u16 mapGroup, u16 mapNum, enum TimeOfDay timeOfDay, u16 profile)
{
    u32 i;
    if (species == SPECIES_NONE)
        return species;
    for (i = 0; i < ARRAY_COUNT(sWildScalingFamilies); i++)
    {
        const struct WildScalingFamily *family = &sWildScalingFamilies[i];
        if (species != family->baseSpecies && species != family->middleSpecies && species != family->finalSpecies)
            continue;
        if (family->mapGroup != WILD_SCALING_ANY && family->mapGroup != mapGroup)
            continue;
        if (family->mapNum != WILD_SCALING_ANY && family->mapNum != mapNum)
            continue;
        if (family->profile != WILD_SCALING_ANY && family->profile != profile)
            continue;
        if (family->timeOfDay != TIMES_OF_DAY_COUNT && family->timeOfDay != timeOfDay)
            continue;
        if (family->middleSpecies == SPECIES_NONE)
        {
            if (effectiveLevel < family->finalLevel)
                return family->baseSpecies;
            return Random() % 100 < family->finalWeight ? family->finalSpecies : family->baseSpecies;
        }
        if (effectiveLevel < family->middleLevel)
            return family->baseSpecies;
        if (family->finalSpecies == SPECIES_NONE || effectiveLevel < family->finalLevel)
            return Random() % 100 < family->middleWeight ? family->middleSpecies : family->baseSpecies;
        if (Random() % 100 < family->finalWeight)
            return family->finalSpecies;
        return Random() % 100 < family->middleWeight ? family->middleSpecies : family->baseSpecies;
    }
    return species;
}

void Script_InspectWildScaling(void)
{
    u16 headerId = GetCurrentMapWildMonHeaderId();
    enum TimeOfDay timeOfDay = GetTimeOfDayForEncounters(headerId, WILD_AREA_LAND);
    const struct WildPokemonInfo *info = headerId == HEADER_NONE ? NULL : GetEffectiveWildEncounterTypes(headerId, timeOfDay)->landMonsInfo;
    u16 minLevel = info == NULL ? 1 : info->wildPokemon[0].minLevel;
    u16 maxLevel = info == NULL ? 1 : info->wildPokemon[0].maxLevel;
    ConvertIntToDecimalStringN(gStringVar1, GetTrainerRank(), STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, GetWorldPhase(), STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, GetWildScalingPartyAverage(), STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar4, GetCurrentWildScalingLevel(), STR_CONV_MODE_LEFT_ALIGN, 3);
    gSpecialVar_0x8006 = GetWildScalingMapModifier(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
    gSpecialVar_Result = GetWildScalingDebugTarget(minLevel, maxLevel, gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
}
