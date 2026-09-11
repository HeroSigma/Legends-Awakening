#include "global.h"
#include "trainer_items.h"
#include "constants/trainer_rank.h"
#include "constants/world_state.h"
#include "data/trainer_items.h"

STATIC_ASSERT(MAX_TRAINER_ITEMS == 4, LATrainerItemsCapacityReview)
STATIC_ASSERT(TRAINER_RANK_COUNT == 6, LATrainerItemsRankReview)
STATIC_ASSERT(WORLD_PHASE_COUNT == 7, LATrainerItemsPhaseReview)

struct LATrainerItemSet BuildLATrainerBattleItems(struct LATrainerPolicy policy,
    bool32 runtimeEligible, u8 trainerRank, u8 worldPhase,
    const enum Item authoredItems[MAX_TRAINER_ITEMS])
{
    struct LATrainerItemSet result;
    u32 count = 0;
    memcpy(result.items, authoredItems, sizeof(result.items));
    for (u32 i = 0; i < MAX_TRAINER_ITEMS; i++)
        if (authoredItems[i] != ITEM_NONE)
            count++;

    if (!runtimeEligible || !LATrainerPolicyHas(policy, LA_TRAINER_POLICY_COMPETITIVE_ITEMS)
     || (count != 0 && LATrainerPolicyHas(policy, LA_TRAINER_POLICY_HANDCRAFTED)))
        return result;

    trainerRank = min(trainerRank, TRAINER_RANK_COUNT - 1);
    worldPhase = min(worldPhase, WORLD_PHASE_COUNT - 1);
    u32 progress = trainerRank + worldPhase;
    u32 tier = progress <= 1 ? 0 : progress <= 3 ? 1 : progress <= 6 ? 2 : progress <= 9 ? 3 : 4;
    if (LATrainerPolicyHas(policy, LA_TRAINER_POLICY_MAJOR))
        tier = min(tier + 1, ARRAY_COUNT(sLATrainerItemTiers) - 1);
    const enum Item *desired = sLATrainerItemTiers[tier].items;
    u32 targetCount = 0;
    for (u32 i = 0; i < MAX_TRAINER_ITEMS; i++)
        if (desired[i] != ITEM_NONE)
            targetCount++;
    if (count >= targetCount)
        return result;

    // Preserve authored order and priority. Supplement only after those entries.
    u32 writeIndex = 0;
    bool8 satisfied[MAX_TRAINER_ITEMS] = {FALSE};
    memset(result.items, 0, sizeof(result.items));
    for (u32 i = 0; i < MAX_TRAINER_ITEMS; i++)
    {
        if (authoredItems[i] == ITEM_NONE)
            continue;
        result.items[writeIndex++] = authoredItems[i];
        for (u32 j = 0; j < targetCount; j++)
        {
            if (!satisfied[j] && authoredItems[i] == desired[j])
            {
                satisfied[j] = TRUE;
                break;
            }
        }
    }
    for (u32 i = 0; i < targetCount && writeIndex < targetCount; i++)
        if (!satisfied[i])
            result.items[writeIndex++] = desired[i];
    return result;
}
