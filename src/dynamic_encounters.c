#include "global.h"
#include "dynamic_encounters.h"
#include "world_state.h"
#include "string_util.h"
#include "event_data.h"

struct DynamicEncounterMap
{
    u16 mapGroup;
    u16 mapNum;
    const struct DynamicEncounterRule *rules;
    u16 count;
};

#include "data/dynamic_encounters.h"
extern const u32 gWildMonHeaderCount;
static const struct WildEncounterTypes sEmptyTypes;

static u16 ResolveRules(const struct DynamicEncounterRule *rules, u32 count)
{
    u32 i;
    for (i = 0; i < count; i++)
    {
        const struct DynamicEncounterRule *rule = &rules[i];
        // Malformed ROM configuration fails closed to the original profile.
        if (rule->region >= WORLD_REGION_COUNT || rule->minState > rule->maxState
         || rule->minPhase >= WORLD_PHASE_COUNT || rule->profile >= ARRAY_COUNT(sProfiles)
         || (rule->profile != DYNAMIC_ENCOUNTER_PROFILE_DEFAULT && sProfiles[rule->profile] == NULL))
            return DYNAMIC_ENCOUNTER_PROFILE_DEFAULT;
        if (GetWorldPhase() >= rule->minPhase
         && GetRegionWorldState(rule->region) >= rule->minState
         && GetRegionWorldState(rule->region) <= rule->maxState)
            return rule->profile;
    }
    return DYNAMIC_ENCOUNTER_PROFILE_DEFAULT;
}

u16 GetDynamicEncounterProfileForMap(u16 mapGroup, u16 mapNum)
{
    u32 lo = 0, hi = ARRAY_COUNT(sDynamicMaps);
    u32 key;
    if (mapGroup >= 0xFF || mapNum >= 0xFF)
        return DYNAMIC_ENCOUNTER_PROFILE_DEFAULT;
    key = (mapGroup << 8) | mapNum;
    while (lo < hi)
    {
        u32 mid = (lo + hi) / 2;
        const struct DynamicEncounterMap *map = &sDynamicMaps[mid];
        u32 candidate = (map->mapGroup << 8) | map->mapNum;
        if (key == candidate)
            return ResolveRules(map->rules, map->count);
        if (key < candidate)
            hi = mid;
        else
            lo = mid + 1;
    }
    return DYNAMIC_ENCOUNTER_PROFILE_DEFAULT;
}

u16 GetCurrentDynamicEncounterProfile(void)
{
    return GetDynamicEncounterProfileForMap(gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
}

const struct WildEncounterTypes *GetEffectiveWildEncounterTypes(u32 headerId, u32 timeOfDay)
{
    const struct WildPokemonHeader *header;
    u16 profile;
    if (headerId >= gWildMonHeaderCount || timeOfDay >= TIMES_OF_DAY_COUNT)
        return &sEmptyTypes;
    header = &gWildMonHeaders[headerId];
    profile = GetDynamicEncounterProfileForMap(header->mapGroup, header->mapNum);
    if (profile != DYNAMIC_ENCOUNTER_PROFILE_DEFAULT)
        return &sProfiles[profile][timeOfDay];
    return &header->encounterTypes[timeOfDay];
}

// DEVELOPMENT ONLY. Map group/number in STR_VAR_1/2; profile in STR_VAR_3;
// controlling Hoenn state in VAR_RESULT. No persistent writes.
void Script_InspectDynamicEncounters(void)
{
    ConvertIntToDecimalStringN(gStringVar1, gSaveBlock1Ptr->location.mapGroup, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar2, gSaveBlock1Ptr->location.mapNum, STR_CONV_MODE_LEFT_ALIGN, 3);
    ConvertIntToDecimalStringN(gStringVar3, GetCurrentDynamicEncounterProfile(), STR_CONV_MODE_LEFT_ALIGN, 3);
    gSpecialVar_Result = GetRegionWorldState(WORLD_REGION_HOENN);
}

#if TESTING
u16 TestResolveDynamicEncounterRules(const struct DynamicEncounterRule *rules, u32 count)
{
    return ResolveRules(rules, count);
}
#endif
