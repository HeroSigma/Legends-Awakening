#include "global.h"
#include "data.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"

#include "la_trainer.h"

// ---------------------------------------------------------------------------
// Single authoritative class -> category mapping (ROM-const).
// Unspecified classes default to LA_TRAINER_ORDINARY (zero-initialized entries).
// Major classes (Gym Leaders, Elite Four, Champions, Rivals, Aqua/Magma
// leaders/admins, plus FRLG variants) map to MAJOR. MAJOR does NOT include
// SCALE_EVOLUTION (handcrafted species are preserved).
// Frontier boss classes map to EXEMPT.
// ---------------------------------------------------------------------------

static const enum LATrainerCategory sLATrainerClassCategories[TRAINER_CLASS_COUNT] =
{
    // --- Major (handcrafted teams; species preserved, no SCALE_EVOLUTION) ---
    [TRAINER_CLASS_LEADER]           = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_ELITE_FOUR]       = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_CHAMPION]         = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_RIVAL]            = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_AQUA_LEADER]      = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_MAGMA_LEADER]     = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_AQUA_ADMIN]       = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_MAGMA_ADMIN]      = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_LEADER_FRLG]      = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_ELITE_FOUR_FRLG]  = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_CHAMPION_FRLG]    = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_RIVAL_EARLY_FRLG] = LA_TRAINER_MAJOR,
    [TRAINER_CLASS_RIVAL_LATE_FRLG]  = LA_TRAINER_MAJOR,

    // --- Frontier boss classes (EXEMPT: generic scaling must not touch) ---
    [TRAINER_CLASS_SALON_MAIDEN]   = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_DOME_ACE]       = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_PIKE_QUEEN]     = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_PYRAMID_KING]   = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_FACTORY_HEAD]   = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_PALACE_MAVEN]    = LA_TRAINER_EXEMPT,
    [TRAINER_CLASS_ARENA_TYCOON]    = LA_TRAINER_EXEMPT,
    // Note: Frontier Brain is a trainer-id concept (EXEMPT via sentinel path),
    // not a trainer-class id here.
};

/*
 * Pure class -> category lookup over the single ROM-const table above. No
 * trainer-table indexing happens here, so it is safe to unit-test directly and
 * is reused (without duplicating the trainer-class database) by
 * GetLATrainerPolicy.
 */
enum LATrainerCategory GetLATrainerCategoryForClass(u8 trainerClass)
{
    if (trainerClass >= TRAINER_CLASS_COUNT)
        return LA_TRAINER_ORDINARY;
    return sLATrainerClassCategories[trainerClass];
}


// ---------------------------------------------------------------------------
// Explicit LA trainer-ID overrides.
// These are legitimate classification decisions for trainers whose narrative
// role is major but whose trainer class is not a major class. They are NOT
// test-only rows.
// ---------------------------------------------------------------------------

// Wally: a major story trainer in a non-major class (TRAINER_CLASS_PKMN_TRAINER_1).
// MAJOR + HANDCRAFTED; no SCALE_EVOLUTION by default.
#define LA_OVERRIDE(TrainerIdConst) {TrainerIdConst, LA_POLICY_MAJOR}

struct LATrainerIdOverride
{
    u16 trainerId;
    u16 policyFlags;
};

static const struct LATrainerIdOverride sLATrainerIdOverrides[] =
{
    // Wally story-battle instances.
    LA_OVERRIDE(TRAINER_WALLY_VR_1),
    LA_OVERRIDE(TRAINER_WALLY_MAUVILLE),
    LA_OVERRIDE(TRAINER_WALLY_VR_2),
    LA_OVERRIDE(TRAINER_WALLY_VR_3),
    LA_OVERRIDE(TRAINER_WALLY_VR_4),
    LA_OVERRIDE(TRAINER_WALLY_VR_5),
    // Sentinel.
    {0xFFFF, 0},
};

static const struct LATrainerIdOverride *FindLATrainerIdOverride(u16 trainerId)
{
    u32 i;
    for (i = 0; sLATrainerIdOverrides[i].trainerId != 0xFFFF; i++)
    {
        if (sLATrainerIdOverrides[i].trainerId == trainerId)
            return &sLATrainerIdOverrides[i];
    }
    return NULL;
}

// ---------------------------------------------------------------------------
// Static policy resolution.
//
// Resolution order (see include/la_trainer.h):
//   1. partner-trainer ID space -> SPECIAL (no transformation flags)
//   2. sentinel trainer IDs (Secret Base / Link / Union Room) -> EXEMPT (IsSpecialTrainer)
//   3. trainerId >= TRAINERS_COUNT -> EXEMPT (invalid/out-of-range/sentinel 0xFFFF, reserved 855-863)
//   4. explicit LA trainer-ID override
//   5. canonical trainer-class via gTrainers[DIFFICULTY_NORMAL][trainerId].trainerClass
//   6. class -> category policy
//   7. valid, unclassified -> ORDINARY
//
// Note re: IsPartnerTrainerId bounds. IsPartnerTrainerId tests
//   (trainerId > TRAINER_PARTNER(PARTNER_NONE)) && (trainerId < TRAINER_PARTNER(PARTNER_COUNT))
// i.e. ids in (MAX_TRAINERS_COUNT+1 .. MAX_TRAINERS_COUNT+PARTNER_COUNT-1).
// The exact edge TRAINER_PARTNER(PARTNER_NONE) (= MAX_TRAINERS_COUNT) is NOT
// matched as a partner by the engine (strict inequality); it falls through to
// the TRAINERS_COUNT range check below and is treated EXEMPT. Partner ids are
// therefore handled before any gTrainers indexing, exactly as required.
struct LATrainerPolicy GetLATrainerPolicy(u16 trainerId)
{
    // Step 1: partner-trainer ID space -> SPECIAL.
    if (IsPartnerTrainerId(trainerId))
        return (struct LATrainerPolicy){ .category = LA_TRAINER_SPECIAL, .flags = LA_POLICY_SPECIAL };

    // Step 2: sentinel trainer IDs -> EXEMPT.
    if (IsSpecialTrainer(trainerId))
        return (struct LATrainerPolicy){ .category = LA_TRAINER_EXEMPT, .flags = LA_POLICY_EXEMPT };

    // Step 3: out-of-range / invalid / 0xFFFF / reserved band -> EXEMPT.
    // This happens BEFORE any gTrainers indexing; SanitizeTrainerId is NOT used.
    if (trainerId >= TRAINERS_COUNT)
        return (struct LATrainerPolicy){ .category = LA_TRAINER_EXEMPT, .flags = LA_POLICY_EXEMPT };

    // Step 4: explicit LA trainer-ID override.
    const struct LATrainerIdOverride *override = FindLATrainerIdOverride(trainerId);
    if (override != NULL)
        return (struct LATrainerPolicy){ .category = LA_TRAINER_MAJOR, .flags = override->policyFlags };

    // Step 5 + 6: canonical class -> category -> policy.
    u8 trainerClass = gTrainers[DIFFICULTY_NORMAL][trainerId].trainerClass;

    enum LATrainerCategory category = GetLATrainerCategoryForClass(trainerClass);
    u16 policyFlags;
    switch (category)
    {
    case LA_TRAINER_MAJOR:   policyFlags = LA_POLICY_MAJOR;    break;
    case LA_TRAINER_EXEMPT:  policyFlags = LA_POLICY_EXEMPT;   break;
    case LA_TRAINER_SPECIAL: policyFlags = LA_POLICY_SPECIAL;  break;
    case LA_TRAINER_ORDINARY:
    default:                 policyFlags = LA_POLICY_ORDINARY; break;
    }

    return (struct LATrainerPolicy){ .category = category, .flags = policyFlags };
}

// ---------------------------------------------------------------------------
// Value-based helpers (operate on an already-resolved policy).
// ---------------------------------------------------------------------------

bool32 LATrainerPolicyHas(struct LATrainerPolicy policy, u16 flag)
{
    return (policy.flags & flag) != 0;
}

bool32 LATrainerPolicyHasAll(struct LATrainerPolicy policy, u16 flags)
{
    return (policy.flags & flags) == flags;
}

bool32 LATrainerUsesScaling(u16 trainerId)
{
    return LATrainerPolicyHas(GetLATrainerPolicy(trainerId), LA_TRAINER_POLICY_SCALE_LEVEL);
}

bool32 LATrainerUsesCompetitiveAI(u16 trainerId)
{
    return LATrainerPolicyHas(GetLATrainerPolicy(trainerId), LA_TRAINER_POLICY_SMART_AI);
}

bool32 LATrainerUsesSixMonPolicy(u16 trainerId)
{
    return LATrainerPolicyHas(GetLATrainerPolicy(trainerId), LA_TRAINER_POLICY_FULL_PARTY);
}

bool32 LATrainerUsesBattleItems(u16 trainerId)
{
    return LATrainerPolicyHas(GetLATrainerPolicy(trainerId), LA_TRAINER_POLICY_COMPETITIVE_ITEMS);
}

bool32 LATrainerIsMajor(u16 trainerId)
{
    return LATrainerPolicyHas(GetLATrainerPolicy(trainerId), LA_TRAINER_POLICY_MAJOR);
}

// ---------------------------------------------------------------------------
// Runtime eligibility. Pure with respect to global battle state — only its
// explicit arguments are read. Not wired into battle code in Phase 1.
// ---------------------------------------------------------------------------

bool32 LATrainerRuntimeEligibility(struct LATrainerPolicy policy, u16 trainerId, u32 battleTypeFlags, bool32 isDebugBattle)
{
    if (LATrainerPolicyHas(policy, LA_TRAINER_POLICY_EXEMPT))
        return FALSE;

    // SPECIAL trainers participate only when an override grants them a flag;
    // the default SPECIAL policy carries none.
    if (policy.category == LA_TRAINER_SPECIAL)
        return FALSE;

    if (isDebugBattle)
        return FALSE;

    // Partner ids are handled statically (-> SPECIAL, already rejected).
    // Double/two-opponent mechanics are preserved (FULL_PARTY caps are the
    // battle-setup's job, not ours), so we do not gate on party arrangement.
    (void)battleTypeFlags;
    (void)trainerId;

    return TRUE;
}

// ---------------------------------------------------------------------------
// Protected/special AI detection for future phases.
// ---------------------------------------------------------------------------

bool32 LATrainerAIIsProtected(u64 aiFlags)
{
    return (aiFlags & (AI_FLAG_DYNAMIC_FUNC | AI_FLAG_ROAMING | AI_FLAG_SAFARI | AI_FLAG_FIRST_BATTLE)) != 0;
}

u64 GetLATrainerAIFlags(u64 authoredFlags, struct LATrainerPolicy policy,
    bool32 runtimeEligible, bool32 controllerAllowsAugmentation)
{
    if (!runtimeEligible || !controllerAllowsAugmentation
     || !LATrainerPolicyHas(policy, LA_TRAINER_POLICY_SMART_AI)
     || LATrainerAIIsProtected(authoredFlags))
        return authoredFlags;
    return authoredFlags | LA_SMART_AI_MASK;
}
