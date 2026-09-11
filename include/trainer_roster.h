#ifndef GUARD_TRAINER_ROSTER_H
#define GUARD_TRAINER_ROSTER_H

#include "global.h"
#include "la_trainer.h"
#include "difficulty.h"

#define LA_ROSTER_DIFFICULTY_ANY 0xFF
#define LA_ROSTER_SUPPLEMENT_KEY(namespaceId, candidateId) \
    (0x80000000u | ((u32)(namespaceId) << 16) | (u32)(candidateId))

struct LARosterAuthoredRef
{
    u32 sourceIndex;
    enum Species expectedSpecies;
};

struct LARosterSupplement
{
    u16 candidateId;
    struct TrainerMon mon;
};

struct LARosterProfile
{
    const struct LARosterAuthoredRef *retained;
    const struct LARosterSupplement *supplements;
    u16 namespaceId;
    u16 supplementCount;
    u8 retainedCount;
};

struct LARosterAssignment
{
    u16 trainerId;
    u8 difficulty;
    const struct LARosterProfile *profile;
};

enum LARosterSelectionStatus
{
    LA_ROSTER_SELECTION_INVALID,
    LA_ROSTER_SELECTION_INCOMPLETE,
    LA_ROSTER_SELECTION_COMPLETE,
};

struct LARosterSelectedMon
{
    const struct TrainerMon *source;
    u32 sourceKey;
};

struct LARosterSelection
{
    struct LARosterSelectedMon members[PARTY_SIZE];
    u8 count;
    enum LARosterSelectionStatus status;
};

// Exact matches are authoritative, including invalid/ambiguous matches.
const struct LARosterProfile *FindLARosterProfile(const struct LARosterAssignment *assignments,
    u32 count, u16 trainerId, enum DifficultyLevel difficulty);
const struct LARosterProfile *GetLARosterProfile(u16 trainerId, enum DifficultyLevel difficulty);
bool32 CanApplyLARoster(u16 trainerId, struct LATrainerPolicy policy,
    bool32 runtimeEligible, u32 battleTypeFlags, bool32 aiVsAi);
struct LARosterSelection SelectLARoster(const struct Trainer *trainer,
    const struct LARosterProfile *profile, u32 targetCount);

#endif
