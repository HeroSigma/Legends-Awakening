#ifndef GUARD_TRAINER_RANK_H
#define GUARD_TRAINER_RANK_H

#include "global.h"
#include "constants/trainer_rank.h"

u8 GetTrainerRank(void);
bool32 SetTrainerRank(u8 rank);
bool32 IsTrainerRankAtLeast(u8 rank);
const u8 *GetTrainerRankName(u8 rank);

// Script adapters: rank arguments use VAR_0x8004; names use STR_VAR_1.
u16 Script_GetTrainerRank(void);
u16 Script_SetTrainerRank(void);
void Script_BufferTrainerRankName(void);

#endif // GUARD_TRAINER_RANK_H
