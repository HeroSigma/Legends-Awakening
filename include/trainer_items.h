#ifndef GUARD_TRAINER_ITEMS_H
#define GUARD_TRAINER_ITEMS_H

#include "global.h"
#include "la_trainer.h"

struct LATrainerItemSet
{
    enum Item items[MAX_TRAINER_ITEMS];
};

// Pure ROM-data policy. Eligibility is resolved with the original requesting ID.
// Nonparticipating and nonempty HANDCRAFTED arrays are preserved exactly.
struct LATrainerItemSet BuildLATrainerBattleItems(struct LATrainerPolicy policy,
    bool32 runtimeEligible, u8 trainerRank, u8 worldPhase,
    const enum Item authoredItems[MAX_TRAINER_ITEMS]);

#endif
