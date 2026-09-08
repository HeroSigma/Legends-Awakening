#ifndef GUARD_DYNAMIC_ENCOUNTERS_H
#define GUARD_DYNAMIC_ENCOUNTERS_H

#include "global.h"
#include "wild_encounter.h"
#include "constants/dynamic_encounters.h"

struct DynamicEncounterRule
{
    u16 region;
    u16 minState;
    u16 maxState;
    u16 minPhase;
    u16 profile;
};

u16 GetDynamicEncounterProfileForMap(u16 mapGroup, u16 mapNum);
u16 GetCurrentDynamicEncounterProfile(void);
const struct WildEncounterTypes *GetEffectiveWildEncounterTypes(u32 headerId, u32 timeOfDay);
void Script_InspectDynamicEncounters(void);
#if TESTING
u16 TestResolveDynamicEncounterRules(const struct DynamicEncounterRule *rules, u32 count);
#endif

#endif
