#include "global.h"
#include "la_save.h"
#include "fake_rtc.h"
#include "rtc.h"
#include "save.h"

// Only the pre-release v0.5 fake-RTC build used this untagged prefix.
struct LegacyFakeRtcSaveBlock3
{
    struct SiiRtcInfo fakeRTC;
    u8 dexNavChain;
    struct QuestSaveData quests;
};

// Changing a storage-affecting feature needs a new layout/migration, not a
// silent change to these offsets. All existing enabled SB3 features are kept.
STATIC_ASSERT(OW_USE_FAKE_RTC == TRUE, LaRequiresFakeRtc);
STATIC_ASSERT(sizeof(struct SiiRtcInfo) == 12, LaRtcSize);
STATIC_ASSERT(offsetof(struct SaveBlock3, dexNavChain) == 0, LaChainOffset);
STATIC_ASSERT(offsetof(struct SaveBlock3, quests) == 4, LaQuestOffset);
STATIC_ASSERT(sizeof(struct QuestSaveData) == 104, LaQuestSize);
STATIC_ASSERT(offsetof(struct SaveBlock3, fakeRTC) == 108, LaRtcOffset);
STATIC_ASSERT(offsetof(struct SaveBlock3, laLayoutTag) == 120, LaLayoutTagOffset);
STATIC_ASSERT(sizeof(struct SaveBlock3) == 124, LaSaveSize);
STATIC_ASSERT(sizeof(struct SaveBlock3) <= SAVE_BLOCK_3_CHUNK_SIZE * NUM_SECTORS_PER_SLOT, LaSaveCapacity);
STATIC_ASSERT(offsetof(struct LegacyFakeRtcSaveBlock3, quests) == 16, LaLegacyQuestOffset);
STATIC_ASSERT(sizeof(struct LegacyFakeRtcSaveBlock3) == 120, LaLegacySize);

static bool32 IsClockValid(const struct SiiRtcInfo *rtc)
{
    static const u8 days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    u32 maxDay;
    if (rtc->year > 99 || rtc->month < 1 || rtc->month > 12
     || rtc->dayOfWeek > 6 || rtc->hour > 23 || rtc->minute > 59 || rtc->second > 59)
        return FALSE;
    maxDay = days[rtc->month - 1];
    if (rtc->month == 2 && IsLeapYear(2000 + rtc->year))
        maxDay++;
    return rtc->day >= 1 && rtc->day <= maxDay;
}

static void ResetMigratedClock(void)
{
    struct Time origin = {0};
    FakeRtc_Reset();
    // A real-RTC save can have a large calendar offset. Rebase it along with
    // the new clock so local day counts cannot wrap negative after upgrading.
    RtcCalcTimeDifference(&gSaveBlock3Ptr->fakeRTC,
        &gSaveBlock2Ptr->localTimeOffset, &origin);
}

void LaSaveBlock3OnLoad(void)
{
    struct SaveBlock3 *save = gSaveBlock3Ptr;
    struct LegacyFakeRtcSaveBlock3 legacy;

    if (save->laLayoutTag == LA_SAVE_LAYOUT_V1)
    {
        // SB3 footer bytes have no sector checksum. Do not let a damaged clock
        // index calendar tables out of bounds; never repair/rechecksum quests.
        if (!IsClockValid(&save->fakeRTC))
            ResetMigratedClock();
        return;
    }
    // Recognized future layout tags must not be interpreted as old saves.
    if ((save->laLayoutTag & 0x00FFFFFF) == (LA_SAVE_LAYOUT_V1 & 0x00FFFFFF))
        return;

    memcpy(&legacy, save, sizeof(legacy));
    if (save->quests.magic != QUEST_SAVE_MAGIC
     && (legacy.quests.magic == QUEST_SAVE_MAGIC || IsClockValid(&legacy.fakeRTC)))
    {
        // Copy through a snapshot: these source and destination ranges overlap.
        // Preserve the exact record, including unknown versions/bad checksums;
        // the Field Log's existing validation continues to reject those writes.
        save->dexNavChain = legacy.dexNavChain;
        save->quests = legacy.quests;
        save->fakeRTC = legacy.fakeRTC;
        if (!IsClockValid(&save->fakeRTC))
            ResetMigratedClock();
    }
    else
    {
        // v0.1-v0.4: chain and quest offsets already match. Bytes beyond the
        // old 108-byte record were zero-filled sector padding, not a clock.
        ResetMigratedClock();
    }
    save->laLayoutTag = LA_SAVE_LAYOUT_V1;
}
