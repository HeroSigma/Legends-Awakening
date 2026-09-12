#ifndef GUARD_TRAINER_SETS_H
#define GUARD_TRAINER_SETS_H

#include "global.h"
#include "la_trainer.h"
#include "difficulty.h"

#define LA_SET_DIFFICULTY_ANY 0xFF
#define LA_SET_PERFECT_IVS TRAINER_PARTY_IVS(31, 31, 31, 31, 31, 31)

struct LASetTraining
{
    u8 evs[6]; // HP, Atk, Def, SpA, SpD, Spe: TrainerMon storage order.
    u8 nature;
};

struct LACompetitiveSet
{
    enum Species finalSpecies;
    const struct LASetTraining *training;
    enum Move moves[MAX_MON_MOVES];
    enum Item heldItem;
    u8 abilitySlot;
};

struct LASetBundle
{
    const struct LACompetitiveSet *const *variants;
    u8 count;
};

struct LASetAssignment
{
    u16 trainerId;
    u8 difficulty;
    u32 sourceKey;
    enum Species authoredSpecies;
    const struct LASetBundle *bundle;
};

bool32 CanApplyLACompetitiveSets(u16 trainerId, struct LATrainerPolicy policy,
    bool32 runtimeEligible, u32 battleTypeFlags, bool32 aiVsAi);
const struct LASetBundle *FindLASetBundle(const struct LASetAssignment *assignments,
    u32 count, u16 trainerId, enum DifficultyLevel difficulty, u32 sourceKey, enum Species authoredSpecies);
const struct LASetBundle *GetLASetBundle(u16 trainerId, enum DifficultyLevel difficulty,
    u32 sourceKey, enum Species authoredSpecies);
bool32 LASetSpeciesCanLearnMove(enum Species species, enum Move move);
bool32 ApplyLACompetitiveSet(struct TrainerMon *workingMon, const struct LASetBundle *bundle);

#endif
