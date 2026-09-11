#ifndef GUARD_TRAINER_EVOLUTION_H
#define GUARD_TRAINER_EVOLUTION_H

#include "global.h"
#include "data.h"
#include "difficulty.h"

enum TrainerEvolutionThresholdSource
{
    TRAINER_EVO_CANONICAL_LEVEL,
    TRAINER_EVO_EXPLICIT_LEVEL,
};

struct TrainerEvolutionStep
{
    enum Species from;
    enum Species to;
    enum TrainerEvolutionThresholdSource thresholdSource;
    u8 level;
};

struct TrainerEvolutionProfile
{
    const struct TrainerEvolutionStep *steps;
    u8 stepCount;
    bool8 preserveSpecies;
};

// Keys refer to the requesting trainer and the selected source entry, including
// when that trainer borrows an overrideTrainer party. No generated slot keys.
struct TrainerEvolutionAssignment
{
    u16 trainerId;
    enum DifficultyLevel difficulty;
    u32 sourceIndex;
    enum Species authoredSpecies;
    const struct TrainerEvolutionProfile *profile;
};

const struct TrainerEvolutionProfile *GetTrainerEvolutionProfile(u16 trainerId,
    enum DifficultyLevel difficulty, u32 sourceIndex, enum Species authoredSpecies);

// ROM-only, forward traversal. NULL selects conservative automatic evolution.
// An unsupported edge retains the last accepted stage, including at branches.
enum Species ResolveTrainerScaledSpecies(enum Species authoredSpecies, u8 finalLevel,
    const struct TrainerEvolutionProfile *profile);

// Operates only on a caller-owned copy. Changes species and explicit ability ID
// incrementally; never changes level, moves, equipment or other authored fields.
bool32 ApplyTrainerEvolution(struct TrainerMon *workingMon,
    const struct TrainerEvolutionProfile *profile);

#endif
