#include "global.h"
#include "data.h"
#include "pokemon.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "la_trainer.h"
#include "trainer_scaling.h"

// ---------------------------------------------------------------------------
// Balance data (PROVISIONAL v0.6.0). These are real gameplay values;
// tuning later requires editing ONLY this table + the two modifiers below.
//
// Row  = Trainer Rank (ROOKIE..LEGEND, TRAINER_RANK_COUNT == 6)
// Col  = World Phase (BEGINNING..LEGEND, WORLD_PHASE_COUNT == 7)
//
//        BEGIN  ROOKIE RISING ACE   ELITE MASTER LEGEND
// ROOKIE {  5,     6,     8,   10,   12,    14,    16  }
// RISING {  7,     9,    12,   15,   18,    21,    24  }
// ACE    { 10,    14,    19,   25,   31,    37,    43  }
// ELITE  { 15,    20,    27,   35,   43,    51,    59  }
// MASTER { 22,    28,    36,   45,   54,    63,    72  }
// LEGEND { 30,    38,    47,   57,   67,    75,    80  }
//
// Monotonic non-decreasing in both dimensions.
// ---------------------------------------------------------------------------

// Count-drift guards: forcing a deliberate balance review if the canonical
// enums change (TRAINER_RANK_COUNT must stay 6, WORLD_PHASE_COUNT must stay 7).
STATIC_ASSERT(TRAINER_RANK_COUNT == 6, TrainerScalingRankCountChangedReviewTable)
STATIC_ASSERT(WORLD_PHASE_COUNT == 7, TrainerScalingPhaseCountChangedReviewTable)

static const u8 sBaseLevelTable[TRAINER_RANK_COUNT][WORLD_PHASE_COUNT] =
{
    [TRAINER_RANK_ROOKIE] = { [WORLD_PHASE_BEGINNING] = 5,  [WORLD_PHASE_ROOKIE]  = 6,  [WORLD_PHASE_RISING] = 8,  [WORLD_PHASE_ACE]   = 10, [WORLD_PHASE_ELITE] = 12, [WORLD_PHASE_MASTER] = 14, [WORLD_PHASE_LEGEND] = 16 },
    [TRAINER_RANK_RISING] = { [WORLD_PHASE_BEGINNING] = 7,  [WORLD_PHASE_ROOKIE]  = 9,  [WORLD_PHASE_RISING] = 12, [WORLD_PHASE_ACE]   = 15, [WORLD_PHASE_ELITE] = 18, [WORLD_PHASE_MASTER] = 21, [WORLD_PHASE_LEGEND] = 24 },
    [TRAINER_RANK_ACE]    = { [WORLD_PHASE_BEGINNING] = 10, [WORLD_PHASE_ROOKIE]  = 14, [WORLD_PHASE_RISING] = 19, [WORLD_PHASE_ACE]   = 25, [WORLD_PHASE_ELITE] = 31, [WORLD_PHASE_MASTER] = 37, [WORLD_PHASE_LEGEND] = 43 },
    [TRAINER_RANK_ELITE]  = { [WORLD_PHASE_BEGINNING] = 15, [WORLD_PHASE_ROOKIE]  = 20, [WORLD_PHASE_RISING] = 27, [WORLD_PHASE_ACE]   = 35, [WORLD_PHASE_ELITE] = 43, [WORLD_PHASE_MASTER] = 51, [WORLD_PHASE_LEGEND] = 59 },
    [TRAINER_RANK_MASTER] = { [WORLD_PHASE_BEGINNING] = 22, [WORLD_PHASE_ROOKIE]  = 28, [WORLD_PHASE_RISING] = 36, [WORLD_PHASE_ACE]   = 45, [WORLD_PHASE_ELITE] = 54, [WORLD_PHASE_MASTER] = 63, [WORLD_PHASE_LEGEND] = 72 },
    [TRAINER_RANK_LEGEND] = { [WORLD_PHASE_BEGINNING] = 30, [WORLD_PHASE_ROOKIE]  = 38, [WORLD_PHASE_RISING] = 47, [WORLD_PHASE_ACE]   = 57, [WORLD_PHASE_ELITE] = 67, [WORLD_PHASE_MASTER] = 75, [WORLD_PHASE_LEGEND] = 80 },
};

// Provisional v0.6.0 category default level modifiers.
static const s8 sLACategoryLevelModifiers[LA_TRAINER_EXEMPT + 1] =
{
    [LA_TRAINER_ORDINARY] = 0,
    [LA_TRAINER_MAJOR]    = 3,
    [LA_TRAINER_SPECIAL]  = 0,
    [LA_TRAINER_EXEMPT]   = 0,
};

// Sparse trainer-ID additional level modifier. Production rows are added only
// where a real design decision exists (e.g. an intentionally easier story
// fight or an extra-hard ace). Currently NO legitimate LA trainer needs one,
// so this resolves all IDs to the default 0.
static const struct LATrainerIdLevelModifier
{
    u16 trainerId;
    s8 modifier;
} sLATrainerIdLevelModifiers[] =
{
    // No production rows yet.
};

// ---------------------------------------------------------------------------
// Implementations
// ---------------------------------------------------------------------------

struct LAPartyStrength CalculateTrainerPartyStrength(void)
{
    u32 top[3] = {0, 0, 0};
    u32 topCount = 0;
    u32 usableCount = 0;
    u32 i;

    for (i = 0; i < PARTY_SIZE; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_PLAYER][i];
        enum Species species = GetMonData(mon, MON_DATA_SPECIES_OR_EGG);
        u8 level = GetMonData(mon, MON_DATA_LEVEL);

        // Exclude eggs, empty slots, and invalid/noncombat entries.
        // Fainted normal Pokémon are deliberately INCLUDED (anti-cheese):
        // fainting high-level mons before a battle must not lower scaling.
        if (species == SPECIES_NONE || species == SPECIES_EGG)
            continue;
        if (GetMonData(mon, MON_DATA_IS_EGG))
            continue;
        if (level == 0)
            continue;

        usableCount++;
        if (topCount < 3)
            top[topCount++] = level;
        else if (level > top[2])
            top[2] = level;

        // Keep top[3] sorted descending.
        u32 j = topCount - 1;
        while (j > 0 && top[j] > top[j - 1])
        {
            u32 t = top[j];
            top[j] = top[j - 1];
            top[j - 1] = t;
            j--;
        }
    }

    struct LAPartyStrength result;
    result.usableCount = (u8)usableCount;

    if (topCount == 0)
    {
        result.avgLevel = 0;
        return result;
    }

    u32 sum = top[0] + top[1] + top[2];   // <= 300, fits u32 (wide accumulator)
    result.avgLevel = (u8)(sum / topCount);
    return result;
}

u8 CalculateTrainerScalingWorldLevel(u8 rank, u8 phase, u8 playerAvgLevel, u8 playerCount)
{
    if (rank >= TRAINER_RANK_COUNT)
        rank = TRAINER_RANK_COUNT - 1;
    if (phase >= WORLD_PHASE_COUNT)
        phase = WORLD_PHASE_COUNT - 1;

    u8 base = sBaseLevelTable[rank][phase];
    u8 boost = 0;

    if (playerCount > 0 && playerAvgLevel > base)
    {
        u32 rawBoost = (u32)(playerAvgLevel - base) / 3;
        boost = (u8)min(rawBoost, MAX_PARTY_BOOST);
    }

    s32 worldLevel = (s32)base + (s32)boost;
    if (worldLevel < 6)
        worldLevel = 6;
    if (worldLevel > MAX_LEVEL)
        worldLevel = MAX_LEVEL;

    return (u8)worldLevel;
}

s8 GetLATrainerIdLevelModifier(u16 trainerId)
{
    u32 i;
    for (i = 0; i < ARRAY_COUNT(sLATrainerIdLevelModifiers); i++)
    {
        if (sLATrainerIdLevelModifiers[i].trainerId == trainerId)
            return sLATrainerIdLevelModifiers[i].modifier;
    }
    return 0;
}

s8 GetLATrainerCategoryLevelModifier(struct LATrainerPolicy policy)
{
    if (policy.category > LA_TRAINER_EXEMPT)
        return 0;
    return sLACategoryLevelModifiers[policy.category];
}

s8 GetLATrainerTotalLevelModifier(u16 trainerId, struct LATrainerPolicy policy)
{
    return GetLATrainerCategoryLevelModifier(policy) + GetLATrainerIdLevelModifier(trainerId);
}

u8 CalculateTrainerLevelDelta(u8 authoredAnchor, u8 worldLevel, s8 trainerModifier)
{
    s32 desiredAnchor = (s32)worldLevel + (s32)trainerModifier;
    if (desiredAnchor < MIN_LEVEL)
        desiredAnchor = MIN_LEVEL;
    if (desiredAnchor > MAX_LEVEL)
        desiredAnchor = MAX_LEVEL;

    s32 delta = desiredAnchor - (s32)authoredAnchor;
    if (delta < 0)
        delta = 0;

    return (u8)delta;
}

u8 ApplyTrainerLevelDelta(u8 authoredSlotLevel, u8 delta)
{
    s32 scaled = (s32)authoredSlotLevel + (s32)delta;
    if (scaled < MIN_LEVEL)
        scaled = MIN_LEVEL;
    if (scaled > MAX_LEVEL)
        scaled = MAX_LEVEL;
    return (u8)scaled;
}
