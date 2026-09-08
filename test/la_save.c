#include "global.h"
#include "fake_rtc.h"
#include "la_save.h"
#include "load_save.h"
#include "quests.h"
#include "save.h"
#include "rtc.h"
#include "malloc.h"
#include "test/test.h"

TEST("SaveBlock: LA layout fits sectors and keeps the original Field Log offset")
{
    EXPECT_EQ(sizeof(struct SaveBlock3), 124);
    EXPECT_EQ(SAVE_BLOCK_3_CHUNK_SIZE * NUM_SECTORS_PER_SLOT, 1624);
    EXPECT_EQ(SAVE_BLOCK_3_CHUNK_SIZE * NUM_SECTORS_PER_SLOT - sizeof(struct SaveBlock3), 1500);
    EXPECT_EQ(offsetof(struct SaveBlock3, dexNavChain), 0);
    EXPECT_EQ(offsetof(struct SaveBlock3, quests), 4);
    EXPECT_EQ(sizeof(struct QuestSaveData), 104);
    EXPECT_EQ(offsetof(struct SaveBlock3, fakeRTC), 108);
    EXPECT_EQ(sizeof(struct SiiRtcInfo), 12);
    EXPECT_EQ(offsetof(struct SaveBlock3, laLayoutTag), 120);
}

TEST("Field Log: pre-RTC saves retain quest progress favorites and chain on upgrade")
{
    struct QuestSaveData record;
    struct SaveBlock3 upgraded;
    ClearSav3();
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0));
    EXPECT(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE));
    record = gSaveBlock3Ptr->quests;
    // Exact v0.2-v0.4 bytes; all trailing sector padding was zero-filled.
    memset(gSaveBlock3Ptr, 0, sizeof(*gSaveBlock3Ptr));
    gSaveBlock3Ptr->dexNavChain = 37;
    gSaveBlock2Ptr->localTimeOffset.days = 8000;
    gSaveBlock2Ptr->localTimeOffset.hours = 18;
    memcpy((u8 *)gSaveBlock3Ptr + 4, &record, sizeof(record));
    LaSaveBlock3OnLoad();
    EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
    EXPECT(QuestSaveIsValid());
    EXPECT(QuestIsSubquestComplete(QUEST_FRAMEWORK_TEST, 0));
    EXPECT(QuestIsFavorite(QUEST_FRAMEWORK_TEST));
    EXPECT_EQ(gSaveBlock3Ptr->dexNavChain, 37);
    RtcCalcLocalTime();
    EXPECT_EQ(gLocalTime.days, 0);
    EXPECT_EQ(gLocalTime.hours, 0);
    EXPECT_EQ(gSaveBlock3Ptr->fakeRTC.month, 1);
    EXPECT_EQ(gSaveBlock3Ptr->fakeRTC.day, 1);
    EXPECT_EQ(gSaveBlock3Ptr->laLayoutTag, LA_SAVE_LAYOUT_V1);
    upgraded = *gSaveBlock3Ptr;
    LaSaveBlock3OnLoad();
    EXPECT_EQ(memcmp(&upgraded, gSaveBlock3Ptr, sizeof(upgraded)), 0);
}

TEST("Field Log: experimental prefixed RTC saves migrate overlapping records intact")
{
    struct QuestSaveData record;
    struct SiiRtcInfo rtc;
    u32 variant;
    ClearSav3();
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 1));
    EXPECT(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE));
    FakeRtc_AdvanceTimeBy(2, 19, 23, 45);
    rtc = gSaveBlock3Ptr->fakeRTC;
    record = gSaveBlock3Ptr->quests;
    for (variant = 0; variant < 3; variant++)
    {
        // Recognized damaged and future-version records must not be repaired
        // or silently reset during relocation.
        if (variant == 1)
            record.checksum ^= 1;
        if (variant == 2)
            record.version++;
        memset(gSaveBlock3Ptr, 0, sizeof(*gSaveBlock3Ptr));
        memcpy(gSaveBlock3Ptr, &rtc, sizeof(rtc));
        *((u8 *)gSaveBlock3Ptr + 12) = 53;
        memcpy((u8 *)gSaveBlock3Ptr + 16, &record, sizeof(record));
        LaSaveBlock3OnLoad();
        EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
        EXPECT_EQ(memcmp(&rtc, &gSaveBlock3Ptr->fakeRTC, sizeof(rtc)), 0);
        EXPECT_EQ(gSaveBlock3Ptr->dexNavChain, 53);
        if (variant == 0)
        {
            EXPECT(QuestSaveIsValid());
            EXPECT(QuestIsSubquestComplete(QUEST_FRAMEWORK_TEST, 1));
        }
        else
        {
            EXPECT(!QuestSaveIsValid());
            EXPECT(!QuestComplete(QUEST_FRAMEWORK_TEST));
            EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
        }
    }
}

TEST("SaveBlock: blank old saves initialize the clock without inventing quest progress")
{
    u32 fill;
    struct QuestSaveData before;
    for (fill = 0; fill < 2; fill++)
    {
        memset(gSaveBlock3Ptr, fill ? 0xFF : 0, sizeof(*gSaveBlock3Ptr));
        before = gSaveBlock3Ptr->quests;
        LaSaveBlock3OnLoad();
        EXPECT_EQ(memcmp(&before, &gSaveBlock3Ptr->quests, sizeof(before)), 0);
        EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_LOCKED);
        EXPECT_EQ(gSaveBlock3Ptr->fakeRTC.month, 1);
        EXPECT_EQ(gSaveBlock3Ptr->fakeRTC.day, 1);
        EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
        EXPECT(QuestSaveIsValid());
    }
}

TEST("SaveBlock: experimental saves without quests retain their clock and chain")
{
    struct SiiRtcInfo rtc;
    ClearSav3();
    FakeRtc_AdvanceTimeBy(0, 21, 12, 9);
    rtc = gSaveBlock3Ptr->fakeRTC;
    memset(gSaveBlock3Ptr, 0, sizeof(*gSaveBlock3Ptr));
    memcpy(gSaveBlock3Ptr, &rtc, sizeof(rtc));
    *((u8 *)gSaveBlock3Ptr + 12) = 8;
    LaSaveBlock3OnLoad();
    EXPECT_EQ(memcmp(&rtc, &gSaveBlock3Ptr->fakeRTC, sizeof(rtc)), 0);
    EXPECT_EQ(gSaveBlock3Ptr->dexNavChain, 8);
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), QUEST_STATE_LOCKED);
}

TEST("Field Log: clock ticks resets and quest writes cannot overlap")
{
    struct QuestSaveData record;
    struct SiiRtcInfo rtc;
    ClearSav3();
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    record = gSaveBlock3Ptr->quests;
    FakeRtc_AdvanceTimeBy(30, 23, 59, 59);
    EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
    rtc = gSaveBlock3Ptr->fakeRTC;
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0));
    EXPECT(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE));
    EXPECT_EQ(memcmp(&rtc, &gSaveBlock3Ptr->fakeRTC, sizeof(rtc)), 0);
    record = gSaveBlock3Ptr->quests;
    FakeRtc_Reset();
    EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
    EXPECT(QuestSaveIsValid());
}

TEST("SaveBlock: native footer chunks round trip the appended clock and layout tag")
{
    struct SaveSector *sector = AllocZeroed(sizeof(*sector));
    u8 (*chunks)[SAVE_BLOCK_3_CHUNK_SIZE] = AllocZeroed(NUM_SECTORS_PER_SLOT * sizeof(*chunks));
    struct SaveBlock3 expected;
    u32 i, id;
    EXPECT_NE(sector, NULL);
    EXPECT_NE(chunks, NULL);
    ClearSav3();
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 2));
    FakeRtc_AdvanceTimeBy(1, 18, 24, 32);
    gSaveBlock3Ptr->dexNavChain = 19;
    expected = *gSaveBlock3Ptr;
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        memset(sector, 0, sizeof(*sector));
        TestCopyFromSaveBlock3(i, sector);
        memcpy(chunks[i], sector->saveBlock3Chunk, SAVE_BLOCK_3_CHUNK_SIZE);
    }
    memset(gSaveBlock3Ptr, 0, sizeof(*gSaveBlock3Ptr));
    // A permutation models the rotating physical sector order.
    for (i = 0; i < NUM_SECTORS_PER_SLOT; i++)
    {
        id = (i * 5 + 1) % NUM_SECTORS_PER_SLOT;
        memcpy(sector->saveBlock3Chunk, chunks[id], SAVE_BLOCK_3_CHUNK_SIZE);
        TestCopyToSaveBlock3(id, sector);
    }
    LaSaveBlock3OnLoad();
    EXPECT_EQ(memcmp(&expected, gSaveBlock3Ptr, sizeof(expected)), 0);
    EXPECT(QuestSaveIsValid());
    Free(chunks);
    Free(sector);
}

TEST("SaveBlock: corrupt clock initialization preserves valid quest data")
{
    struct QuestSaveData record;
    ClearSav3();
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    record = gSaveBlock3Ptr->quests;
    gSaveBlock3Ptr->fakeRTC.month = 255;
    LaSaveBlock3OnLoad();
    EXPECT_EQ(gSaveBlock3Ptr->fakeRTC.month, 1);
    EXPECT_EQ(memcmp(&record, &gSaveBlock3Ptr->quests, sizeof(record)), 0);
    EXPECT(QuestSaveIsValid());
}
