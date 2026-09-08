#include "global.h"
#include "quests.h"
#include "event_data.h"
#include "item.h"
#include "trainer_rank.h"
#include "main.h"
#include "text.h"
#include "constants/items.h"
#include "test/test.h"

TEST("Field Log: new records start locked and invalid quest IDs are safe")
{
    struct QuestSaveData before;
    QuestDebugReset();
    before = gSaveBlock3Ptr->quests;
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_LOCKED);
    EXPECT_EQ(QuestGetState(QUEST_COUNT), QUEST_STATE_LOCKED);
    EXPECT_EQ(QuestStart(QUEST_COUNT), FALSE);
    EXPECT_EQ(QuestStart(0xFFFF), FALSE);
    EXPECT_EQ(QuestCompleteSubquest(QUEST_COUNT, 0), FALSE);
    EXPECT_EQ(QuestSetFavorite(QUEST_COUNT, TRUE), FALSE);
    EXPECT_EQ(QuestGetDefinition(QUEST_COUNT), NULL);
    EXPECT_EQ(memcmp(&before, &gSaveBlock3Ptr->quests, sizeof(before)), 0);
}

TEST("Field Log: active, subquest, reward and complete representations round trip")
{
    struct QuestSaveData saved;
    u32 state;

    QuestDebugReset();
    ClearBag();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE));
    for (state = QUEST_STATE_ACTIVE; state <= QUEST_STATE_COMPLETE; state++)
    {
        if (state == QUEST_STATE_REWARD)
        {
            EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0));
            EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 1));
            EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 2));
            EXPECT(QuestMarkRewardAvailable(QUEST_FRAMEWORK_TEST));
        }
        else if (state == QUEST_STATE_COMPLETE)
        {
            EXPECT(QuestClaimReward(QUEST_FRAMEWORK_TEST));
        }
        // Exercise the exact bytes carried by SaveBlock3, not a RAM-only cache.
        saved = gSaveBlock3Ptr->quests;
        QuestDebugReset();
        memcpy(&gSaveBlock3Ptr->quests, &saved, sizeof(saved));
        EXPECT(QuestSaveIsValid());
        EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), state);
        EXPECT(QuestIsFavorite(QUEST_FRAMEWORK_TEST));
        EXPECT_EQ(QuestAreAllSubquestsComplete(QUEST_FRAMEWORK_TEST), state >= QUEST_STATE_REWARD);
    }
}

TEST("Field Log: invalid state changes and child IDs do not mutate progress")
{
    struct QuestSaveData before;
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    before = gSaveBlock3Ptr->quests;
    EXPECT_EQ(QuestSetState(QUEST_FRAMEWORK_TEST, QUEST_STATE_COUNT), FALSE);
    EXPECT_EQ(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 3), FALSE);
    EXPECT_EQ(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0xFFFF), FALSE);
    EXPECT_EQ(QuestIsSubquestComplete(QUEST_FRAMEWORK_TEST, 3), FALSE);
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_ACTIVE);
    EXPECT_EQ(memcmp(&before, &gSaveBlock3Ptr->quests, sizeof(before)), 0);
}

TEST("Field Log: rewards require objectives and cannot be claimed twice")
{
    u32 child;
    u16 quantity;
    QuestDebugReset();
    ClearBag();
    EXPECT_EQ(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0), FALSE);
    EXPECT_EQ(QuestClaimReward(QUEST_FRAMEWORK_TEST), FALSE);
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT_EQ(QuestMarkRewardAvailable(QUEST_FRAMEWORK_TEST), FALSE);
    for (child = 0; child < QuestGetDefinition(QUEST_FRAMEWORK_TEST)->numSubquests; child++)
        EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, child));
    EXPECT(QuestMarkRewardAvailable(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestClaimReward(QUEST_FRAMEWORK_TEST));
    quantity = CountTotalItemQuantityInBag(QuestGetDefinition(QUEST_FRAMEWORK_TEST)->rewardItem);
    EXPECT_EQ(quantity, 1);
    EXPECT_EQ(QuestClaimReward(QUEST_FRAMEWORK_TEST), FALSE);
    EXPECT_EQ(QuestStart(QUEST_FRAMEWORK_TEST), FALSE);
    EXPECT_EQ(CountTotalItemQuantityInBag(QuestGetDefinition(QUEST_FRAMEWORK_TEST)->rewardItem), quantity);
}

TEST("Field Log: damaged and future-version records are preserved")
{
    bool32 future;
    struct QuestSaveData before;
    PARAMETRIZE { future = FALSE; }
    PARAMETRIZE { future = TRUE; }
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    if (future)
        gSaveBlock3Ptr->quests.version++;
    else
        gSaveBlock3Ptr->quests.subquests[0] ^= 1;
    before = gSaveBlock3Ptr->quests;
    EXPECT_EQ(QuestSaveIsValid(), FALSE);
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_LOCKED);
    EXPECT_EQ(QuestStart(QUEST_FRAMEWORK_TEST), FALSE);
    EXPECT_EQ(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE), FALSE);
    EXPECT_EQ(memcmp(&before, &gSaveBlock3Ptr->quests, sizeof(before)), 0);
}

TEST("Field Log: script arguments are full width and Trainer Rank is independent")
{
    QuestDebugReset();
    EXPECT(SetTrainerRank(TRAINER_RANK_RISING));
    gSpecialVar_0x8004 = 256;
    gSpecialVar_0x8005 = QUEST_CMD_START;
    EXPECT_EQ(Script_QuestCommand(), FALSE);
    gSpecialVar_0x8004 = QUEST_FRAMEWORK_TEST;
    EXPECT(Script_QuestCommand());
    gSpecialVar_0x8005 = QUEST_CMD_COMPLETE_SUB;
    gSpecialVar_0x8006 = 256;
    EXPECT_EQ(Script_QuestCommand(), FALSE);
    EXPECT_EQ(QuestIsSubquestComplete(QUEST_FRAMEWORK_TEST, 0), FALSE);
    QuestDebugReset();
    EXPECT_EQ(GetTrainerRank(), TRAINER_RANK_RISING);
}

TEST("Field Log: serialized capacity and original SaveBlock3 prefix stay fixed")
{
    EXPECT_EQ(sizeof(struct QuestSaveData), 104);
    EXPECT_EQ(offsetof(struct SaveBlock3, quests), 4);
    EXPECT_EQ(sizeof(struct SaveBlock3), 108);
    EXPECT_EQ(sizeof(struct SaveBlock1), 15568);
    EXPECT_EQ(sizeof(struct SaveBlock2), 3884);
}

TEST("Field Log: definitions use distinct in-range subquest IDs")
{
    u32 quest, child;
    bool8 used[SUBQUEST_SAVE_CAPACITY] = {0};
    for (quest = 0; quest < QUEST_COUNT; quest++)
    {
        const struct SideQuest *def = QuestGetDefinition(quest);
        EXPECT(def != NULL);
        for (child = 0; child < def->numSubquests; child++)
        {
            u16 id = def->subquests[child].id;
            EXPECT(id < SUBQUEST_SAVE_CAPACITY);
            EXPECT_EQ(used[id], FALSE);
            used[id] = TRUE;
        }
    }
}

TEST("Field Log: full bag preserves reward availability for retry")
{
    struct BagPocket *pocket;
    struct QuestSaveData before;
    u32 i;
    QuestDebugReset();
    ClearBag();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    for (i = 0; i < 3; i++)
        EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, i));
    EXPECT(QuestMarkRewardAvailable(QUEST_FRAMEWORK_TEST));
    pocket = &gBagPockets[GetItemPocket(ITEM_POTION)];
    for (i = 0; i < pocket->capacity; i++)
        BagPocket_SetSlotItemIdAndCount(pocket, i, ITEM_POTION, MAX_BAG_ITEM_CAPACITY);
    before = gSaveBlock3Ptr->quests;
    EXPECT_EQ(QuestClaimReward(QUEST_FRAMEWORK_TEST), FALSE);
    EXPECT_EQ(memcmp(&before, &gSaveBlock3Ptr->quests, sizeof(before)), 0);
    EXPECT(RemoveBagItem(ITEM_POTION, 1));
    EXPECT(QuestClaimReward(QUEST_FRAMEWORK_TEST));
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_COMPLETE);
}

static bool8 sFieldLogTestReturned;

static void FieldLogTestReturn(void)
{
    sFieldLogTestReturned = TRUE;
}

static void FieldLogTestFrames(u16 key, u32 frames)
{
    u32 i;
    gMain.newKeys = key;
    gMain.newAndRepeatedKeys = key;
    for (i = 0; i < frames && !sFieldLogTestReturned; i++)
    {
        if (gMain.vblankCallback != NULL)
            gMain.vblankCallback();
        gMain.callback2();
        gMain.newKeys = 0;
        gMain.newAndRepeatedKeys = 0;
    }
}

TEST("Field Log: menu opens, filters, navigates icons, and releases resources")
{
    MainCallback callback = gMain.callback2;
    IntrCallback vblank = gMain.vblankCallback;
    u32 i, pass;
    QuestDebugReset();
    for (pass = 0; pass < 3; pass++)
    {
        if (pass == 1)
            EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
        if (pass == 2)
            EXPECT(QuestStart(QUEST_ICON_TEST));
        sFieldLogTestReturned = FALSE;
        QuestMenu_Init(pass == 2 ? QUEST_ICON_TEST : QUEST_FRAMEWORK_TEST, FieldLogTestReturn);
        FieldLogTestFrames(0, 40);
        if (pass == 1)
        {
            FieldLogTestFrames(SELECT_BUTTON, 1);
            EXPECT(QuestIsFavorite(QUEST_FRAMEWORK_TEST));
            FieldLogTestFrames(A_BUTTON, 1);
            FieldLogTestFrames(DPAD_DOWN, 1); // Pokemon icon
            FieldLogTestFrames(DPAD_DOWN, 1); // Item icon
            FieldLogTestFrames(B_BUTTON, 1); // Parent NPC icon
        }
        FieldLogTestFrames(START_BUTTON, 1);
        for (i = 0; i < 5; i++)
            FieldLogTestFrames(R_BUTTON, 1); // Includes empty filters
        FieldLogTestFrames(B_BUTTON, 40);
        EXPECT(sFieldLogTestReturned);
    }
    SetVBlankCallback(vblank);
    SetMainCallback2(callback);
}
