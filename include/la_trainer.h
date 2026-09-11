#ifndef GUARD_LA_TRAINER_H
#define GUARD_LA_TRAINER_H

#include "constants/trainers.h"
#include "constants/battle_ai.h"

// Types needed by this API. Pull in the core definitions without dragging
// in the entire global header (avoids pulling test-incompatible paths).
#include "data.h"  // IsPartnerTrainerId, IsSpecialTrainer, gTrainers, TRAINERS_COUNT, struct Trainer

enum LATrainerCategory
{
    LA_TRAINER_ORDINARY,
    LA_TRAINER_MAJOR,
    LA_TRAINER_SPECIAL,
    LA_TRAINER_EXEMPT,
};

#define LA_TRAINER_POLICY_NONE            0

// Preserve authored gimmick flags, but never grant Smart Tera or prediction.
#define LA_SMART_AI_MASK (AI_FLAG_SMART_TRAINER & ~AI_FLAG_SMART_TERA)

#define LA_TRAINER_POLICY_SCALE_LEVEL          (1 << 0)
#define LA_TRAINER_POLICY_SCALE_EVOLUTION      (1 << 1)
#define LA_TRAINER_POLICY_SMART_AI             (1 << 2)
#define LA_TRAINER_POLICY_FULL_PARTY           (1 << 3)
#define LA_TRAINER_POLICY_COMPETITIVE_ITEMS    (1 << 4)
#define LA_TRAINER_POLICY_MAJOR                (1 << 5)
#define LA_TRAINER_POLICY_HANDCRAFTED          (1 << 6)
#define LA_TRAINER_POLICY_EXEMPT               (1 << 7)

#define LA_POLICY_ORDINARY   (LA_TRAINER_POLICY_SCALE_LEVEL   \
                            | LA_TRAINER_POLICY_SCALE_EVOLUTION \
                            | LA_TRAINER_POLICY_SMART_AI      \
                            | LA_TRAINER_POLICY_FULL_PARTY      \
                            | LA_TRAINER_POLICY_COMPETITIVE_ITEMS)

// MAJOR: handcrafted species are preserved; SCALE_EVOLUTION is NOT included
#define LA_POLICY_MAJOR    (LA_TRAINER_POLICY_SCALE_LEVEL      \
                          | LA_TRAINER_POLICY_SMART_AI         \
                          | LA_TRAINER_POLICY_FULL_PARTY       \
                          | LA_TRAINER_POLICY_COMPETITIVE_ITEMS  \
                          | LA_TRAINER_POLICY_MAJOR              \
                          | LA_TRAINER_POLICY_HANDCRAFTED)

#define LA_POLICY_SPECIAL  LA_TRAINER_POLICY_NONE
#define LA_POLICY_EXEMPT   LA_TRAINER_POLICY_EXEMPT

struct LATrainerPolicy
{
    u8 category;   // enum LATrainerCategory
    u16 flags;
};

/*
 * Resolve the static (ROM/data-time) classification of a trainer.
 * Pure with respect to battle/timing context: no gIsDebugBattle, no
 * gBattleTypeFlags, no runtime Difficulty. Reads only the trainerId value
 * and ROM tables.
 *
 * Resolution order:
 *   1. partner-trainer ID space -> SPECIAL (no transformation flags)
 *   2. sentinel trainer IDs (Secret Base / Link / Union Room) -> EXEMPT (IsSpecialTrainer)
 *   3. trainerId >= TRAINERS_COUNT -> EXEMPT (invalid/out-of-range/sentinel 0xFFFF, reserved 855-863)
 *   4. explicit LA trainer-ID override
 *   5. canonical trainer-class via gTrainers[DIFFICULTY_NORMAL][trainerId].trainerClass
 *   6. class -> category policy
 *   7. valid, unclassified -> ORDINARY
 *
 * Invalid IDs are rejected BEFORE any gTrainers indexing.
 */
struct LATrainerPolicy GetLATrainerPolicy(u16 trainerId);

/*
 * Pure helper over the class->category mapping. Lets the same ROM-const
 * table be unit-tested directly (e.g. frontier classes) without going
 * through an arbitrary trainerId.
 */
enum LATrainerCategory GetLATrainerCategoryForClass(u8 trainerClass);

/*
 * Value-based helpers operating on an already-resolved policy so callers
 * resolve once and test many flags.
 */
bool32 LATrainerPolicyHas(struct LATrainerPolicy policy, u16 flag);
bool32 LATrainerPolicyHasAll(struct LATrainerPolicy policy, u16 flags);

/* Convenience wrappers (each resolves exactly once internally). */
bool32 LATrainerUsesScaling(u16 trainerId);
bool32 LATrainerUsesCompetitiveAI(u16 trainerId);
bool32 LATrainerUsesSixMonPolicy(u16 trainerId);
bool32 LATrainerUsesBattleItems(u16 trainerId);
bool32 LATrainerIsMajor(u16 trainerId);

/*
 * Runtime eligibility: static policy + explicit battle-context arguments.
 * Pure with respect to global battle state — only its parameters are read.
 * Phase 1 implements the helper only; it is NOT wired into battle code yet.
 */
bool32 LATrainerRuntimeEligibility(struct LATrainerPolicy policy,
                                   u16 trainerId,
                                   u32 battleTypeFlags,
                                   bool32 isDebugBattle);

/*
 * Returns TRUE if aiFlags contains a protected/special AI configuration
 * that future phases must not blanket-augment (e.g. scripted custom AI,
 * roaming, safari, first battle).
 */
bool32 LATrainerAIIsProtected(u64 aiFlags);

u64 GetLATrainerAIFlags(u64 authoredFlags, struct LATrainerPolicy policy,
    bool32 runtimeEligible, bool32 controllerAllowsAugmentation);

#endif // GUARD_LA_TRAINER_H
