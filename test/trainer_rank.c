#include "global.h"
#include "event_data.h"
#include "string_util.h"
#include "trainer_rank.h"
#include "test/test.h"

TEST("Trainer Rank: cleared event data starts at Rookie and survives map temp clearing")
{
    InitEventData();
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_ROOKIE);
    EXPECT(SetTrainerRank(TRAINER_RANK_RISING));
    ClearTempFieldEventData();
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_RISING);
    EXPECT_EQ(VarGet(VAR_TRAINER_RANK), TRAINER_RANK_RISING);
}

TEST("Trainer Rank: all ranks round trip, compare, and allow demotion")
{
    u32 rank, threshold;

    for (rank = TRAINER_RANK_ROOKIE; rank < TRAINER_RANK_COUNT; rank++)
    {
        EXPECT(SetTrainerRank(rank));
        EXPECT_EQ(GetTrainerRank(), rank);
        for (threshold = TRAINER_RANK_ROOKIE; threshold < TRAINER_RANK_COUNT; threshold++)
            EXPECT_EQ(IsTrainerRankAtLeast(threshold), rank >= threshold);
    }
    EXPECT(SetTrainerRank(TRAINER_RANK_ROOKIE));
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_ROOKIE);
}

TEST("Trainer Rank: invalid requests do not change the saved rank")
{
    u32 rank;

    SetTrainerRank(TRAINER_RANK_ACE);
    for (rank = TRAINER_RANK_COUNT; rank <= 255; rank++)
    {
        EXPECT_EQ(SetTrainerRank(rank), FALSE);
        EXPECT_EQ(IsTrainerRankAtLeast(rank), FALSE);
        EXPECT_EQ(GetTrainerRankName(rank), GetTrainerRankName(TRAINER_RANK_ROOKIE));
    }
    EXPECT_EQ(VarGet(VAR_TRAINER_RANK), TRAINER_RANK_ACE);
}

TEST("Trainer Rank: invalid saved values fall back without writing or truncating")
{
    u16 value;
    PARAMETRIZE { value = TRAINER_RANK_COUNT; }
    PARAMETRIZE { value = 255; }
    PARAMETRIZE { value = 256; }
    PARAMETRIZE { value = 257; }
    PARAMETRIZE { value = 65535; }

    VarSet(VAR_TRAINER_RANK, value);
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_ROOKIE);
    EXPECT_EQ(IsTrainerRankAtLeast(TRAINER_RANK_RISING), FALSE);
    EXPECT_EQ(VarGet(VAR_TRAINER_RANK), value);
}

TEST("Trainer Rank: script adapters validate full-width input and buffer names")
{
    u32 rank;

    for (rank = TRAINER_RANK_ROOKIE; rank < TRAINER_RANK_COUNT; rank++)
    {
        gSpecialVar_0x8004 = rank;
        EXPECT_EQ(Script_SetTrainerRank(), TRUE);
        EXPECT_EQ(Script_GetTrainerRank(), rank);
        EXPECT_EQ(StringCompare(gStringVar1, GetTrainerRankName(rank)), 0);
        Script_BufferTrainerRankName();
        EXPECT_EQ(StringCompare(gStringVar1, GetTrainerRankName(rank)), 0);
    }
    gSpecialVar_0x8004 = 257;
    EXPECT_EQ(Script_SetTrainerRank(), FALSE);
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_LEGEND);
    Script_BufferTrainerRankName();
    EXPECT_EQ(StringCompare(gStringVar1, GetTrainerRankName(TRAINER_RANK_ROOKIE)), 0);
}
