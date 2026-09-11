#ifndef GUARD_TRAINER_SCALING_H
#define GUARD_TRAINER_SCALING_H

#include "global.h"
#include "constants/trainer_rank.h"
#include "constants/world_state.h"
#include "la_trainer.h"

/*
 * LA v0.6.0 Phase 2 - Runtime Trainer Level Scaling.
 *
 * Deterministic, level-only trainer scaling for eligible trainers.
 * Player-strength influence appears ONLY in CalculateTrainerScalingWorldLevel;
 * the delta layer never re-reads the player party.
 *
 * NO downscaling: delta is always >= 0, so authored trainer levels form a
 * nonnegative progression floor.
 */

#define MAX_PARTY_BOOST 15

struct LAPartyStrength
{
    u8 avgLevel;     // strongest-3 usable level average (floor), 0 if none
    u8 usableCount;  // number of valid (non-egg, non-NONE, level > 0) party mons
};

// Scan gParties[B_TRAINER_PLAYER] and return the strongest-3 average level +
// usable count. Fainted normal Pokémon are INCLUDED (anti-cheese): only eggs,
// SPECIES_NONE slots, and level-0/invalid entries are excluded.
struct LAPartyStrength CalculateTrainerPartyStrength(void);

// Expected player-side world level at this rank/phase/player power, [6, MAX_LEVEL].
//   base = table[clamp(rank, 0, TRAINER_RANK_COUNT-1)][clamp(phase, 0, WORLD_PHASE_COUNT-1)]
//   if playerAvgLevel > base: boost = min((playerAvgLevel - base) / 3, MAX_PARTY_BOOST)
//   else: boost = 0
//   worldLevel = clamp(base + boost, 6, MAX_LEVEL)
// playerCount is used at minimum to explicitly represent the zero-valid-party case
// (avgLevel==0, count==0 -> no boost).
u8 CalculateTrainerScalingWorldLevel(u8 rank, u8 phase, u8 playerAvgLevel, u8 playerCount);

// Sparse trainer-ID level modifier. Defaults to 0.
// Production rows are only added where a real design decision exists.
s8 GetLATrainerIdLevelModifier(u16 trainerId);

// Category default modifier. Provision: ORDINARY=0, MAJOR=+3, SPECIAL=0, EXEMPT=0.
s8 GetLATrainerCategoryLevelModifier(struct LATrainerPolicy policy);

// Final modifier = category modifier + ID modifier (single source for deltas).
s8 GetLATrainerTotalLevelModifier(u16 trainerId, struct LATrainerPolicy policy);

// Per-trainer anchor delta (>= 0, no downscaling):
//   desiredAnchor = clamp(worldLevel + trainerModifier, MIN_LEVEL, MAX_LEVEL)
//   delta         = max(0, desiredAnchor - authoredAnchor)
u8 CalculateTrainerLevelDelta(u8 authoredAnchor, u8 worldLevel, s8 trainerModifier);

// Per-slot application: clamp(authoredSlotLevel + delta, MIN_LEVEL, MAX_LEVEL).
u8 ApplyTrainerLevelDelta(u8 authoredSlotLevel, u8 delta);

#endif // GUARD_TRAINER_SCALING_H