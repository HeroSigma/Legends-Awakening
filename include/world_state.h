#ifndef GUARD_WORLD_STATE_H
#define GUARD_WORLD_STATE_H

#include "global.h"
#include "constants/world_state.h"

u16 GetWorldPhase(void);
bool32 SetWorldPhase(u16 phase);
bool32 IsWorldPhaseAtLeast(u16 phase);

u16 GetRegionWorldState(u8 region);
bool32 SetRegionWorldState(u8 region, u16 state);
bool32 IsRegionWorldStateAtLeast(u8 region, u16 state);

const u8 *GetWorldPhaseName(u16 phase);
const u8 *GetWorldRegionName(u8 region);

// Script arguments use VAR_0x8004 and VAR_0x8005. Results use VAR_RESULT.
u16 Script_GetWorldPhase(void);
u16 Script_SetWorldPhase(void);
u16 Script_GetRegionWorldState(void);
u16 Script_SetRegionWorldState(void);
u16 Script_IsWorldPhaseAtLeast(void);
u16 Script_IsRegionWorldStateAtLeast(void);
void Script_BufferWorldPhaseName(void);
void Script_BufferWorldRegionName(void);

#endif // GUARD_WORLD_STATE_H
