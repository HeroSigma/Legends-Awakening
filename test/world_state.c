#include "global.h"
#include "event_data.h"
#include "quests.h"
#include "test/test.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "string_util.h"

struct WorldStateSnapshot
{
    u16 phase;
    u16 regions[WORLD_REGION_COUNT];
};

static struct WorldStateSnapshot SnapshotWorldState(void)
{
    struct WorldStateSnapshot snapshot;
    u32 region;

    snapshot.phase = VarGet(VAR_WORLD_PHASE);
    for (region = 0; region < WORLD_REGION_COUNT; region++)
        snapshot.regions[region] = GetRegionWorldState(region);
    return snapshot;
}

static void RestoreWorldState(const struct WorldStateSnapshot *snapshot)
{
    u32 region;

    VarSet(VAR_WORLD_PHASE, snapshot->phase);
    for (region = 0; region < WORLD_REGION_COUNT; region++)
        VarSet(VAR_WORLD_STATE_HOENN + region, snapshot->regions[region]);
}

TEST("World State: default and valid phase behavior")
{
    struct WorldStateSnapshot snapshot = SnapshotWorldState();

    InitEventData();
    EXPECT_EQ(GetWorldPhase(), WORLD_PHASE_BEGINNING);
    EXPECT(SetWorldPhase(WORLD_PHASE_RISING));
    ClearTempFieldEventData();
    EXPECT_EQ(GetWorldPhase(), WORLD_PHASE_RISING);
    EXPECT(IsWorldPhaseAtLeast(WORLD_PHASE_ROOKIE));
    EXPECT(IsWorldPhaseAtLeast(WORLD_PHASE_RISING));
    EXPECT(!IsWorldPhaseAtLeast(WORLD_PHASE_ACE));
    EXPECT(SetWorldPhase(WORLD_PHASE_BEGINNING));
    EXPECT_EQ(GetWorldPhase(), WORLD_PHASE_BEGINNING);

    RestoreWorldState(&snapshot);
}

TEST("World State: invalid phases are safe and do not rewrite storage")
{
    struct WorldStateSnapshot snapshot = SnapshotWorldState();

    VarSet(VAR_WORLD_PHASE, 0xFFFF);
    EXPECT_EQ(GetWorldPhase(), WORLD_PHASE_BEGINNING);
    EXPECT_EQ(VarGet(VAR_WORLD_PHASE), 0xFFFF);
    EXPECT_EQ(SetWorldPhase(WORLD_PHASE_COUNT), FALSE);
    EXPECT_EQ(IsWorldPhaseAtLeast(WORLD_PHASE_COUNT), FALSE);
    EXPECT_EQ(GetWorldPhaseName(WORLD_PHASE_COUNT), GetWorldPhaseName(WORLD_PHASE_BEGINNING));

    RestoreWorldState(&snapshot);
}

TEST("World State: regions are independent, comparable, and reversible")
{
    struct WorldStateSnapshot snapshot = SnapshotWorldState();

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 2));
    EXPECT(SetRegionWorldState(WORLD_REGION_JOHTO, 5));
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_HOENN), 2);
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_JOHTO), 5);
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_KANTO), 0);
    EXPECT(IsRegionWorldStateAtLeast(WORLD_REGION_JOHTO, 4));
    EXPECT(!IsRegionWorldStateAtLeast(WORLD_REGION_HOENN, 3));
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_HOENN), 1);
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_JOHTO), 5);

    RestoreWorldState(&snapshot);
}

TEST("World State: invalid regions are safe and do not corrupt another region")
{
    struct WorldStateSnapshot snapshot = SnapshotWorldState();

    VarSet(VAR_WORLD_STATE_HOENN, 7);
    VarSet(VAR_WORLD_STATE_JOHTO, 9);
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_COUNT), 0);
    EXPECT_EQ(SetRegionWorldState(WORLD_REGION_COUNT, 12), FALSE);
    EXPECT_EQ(IsRegionWorldStateAtLeast(WORLD_REGION_COUNT, 1), FALSE);
    EXPECT_EQ(GetRegionWorldState(WORLD_REGION_JOHTO), 9);
    EXPECT_NE(GetWorldRegionName(WORLD_REGION_COUNT), GetWorldRegionName(WORLD_REGION_HOENN));

    RestoreWorldState(&snapshot);
}

TEST("World State: script adapters validate arguments and preserve other systems")
{
    struct WorldStateSnapshot snapshot = SnapshotWorldState();
    u8 trainerRank = GetTrainerRank();
    u8 questState = QuestGetState(QUEST_FRAMEWORK_TEST);

    gSpecialVar_0x8004 = WORLD_PHASE_ACE;
    EXPECT_EQ(Script_SetWorldPhase(), TRUE);
    EXPECT_EQ(Script_GetWorldPhase(), WORLD_PHASE_ACE);
    gSpecialVar_0x8004 = WORLD_REGION_HOENN;
    gSpecialVar_0x8005 = 3;
    EXPECT_EQ(Script_SetRegionWorldState(), TRUE);
    EXPECT_EQ(Script_GetRegionWorldState(), 3);
    EXPECT_EQ(Script_IsWorldPhaseAtLeast(), TRUE);
    EXPECT_EQ(Script_IsRegionWorldStateAtLeast(), TRUE);
    gSpecialVar_0x8004 = WORLD_REGION_COUNT;
    EXPECT_EQ(Script_SetRegionWorldState(), FALSE);
    EXPECT_EQ(Script_GetRegionWorldState(), 0);
    EXPECT_EQ(GetTrainerRank(), trainerRank);
    EXPECT_EQ(QuestGetState(QUEST_FRAMEWORK_TEST), questState);

    RestoreWorldState(&snapshot);
}

TEST("World State: all phases and unsigned region boundaries round trip")
{
    u32 phase, region, other;

    for (phase = 0; phase < WORLD_PHASE_COUNT; phase++)
    {
        EXPECT(SetWorldPhase(phase));
        EXPECT_EQ(GetWorldPhase(), phase);
        EXPECT(IsWorldPhaseAtLeast(phase));
    }
    for (region = 0; region < WORLD_REGION_COUNT; region++)
        EXPECT_EQ(GetRegionWorldState(region), 0);
    for (region = 0; region < WORLD_REGION_COUNT; region++)
    {
        EXPECT(SetRegionWorldState(region, 0xFFFF));
        EXPECT_EQ(GetRegionWorldState(region), 0xFFFF);
        EXPECT(IsRegionWorldStateAtLeast(region, 0xFFFF));
        for (other = 0; other < WORLD_REGION_COUNT; other++)
            if (other != region)
                EXPECT_EQ(GetRegionWorldState(other), 0);
        EXPECT(SetRegionWorldState(region, 0));
        EXPECT(!IsRegionWorldStateAtLeast(region, 1));
    }
}

TEST("World State: full-width script arguments cannot alias valid IDs")
{
    u32 i;
    static const u16 invalid[] = {WORLD_REGION_COUNT, 0x100, 0xFFFF};

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 42));
    for (i = 0; i < ARRAY_COUNT(invalid); i++)
    {
        gSpecialVar_0x8004 = invalid[i];
        gSpecialVar_0x8005 = 0;
        EXPECT_EQ(Script_GetRegionWorldState(), 0);
        EXPECT_EQ(Script_SetRegionWorldState(), FALSE);
        EXPECT_EQ(Script_IsRegionWorldStateAtLeast(), FALSE);
        Script_BufferWorldRegionName();
        EXPECT_EQ(StringCompare(gStringVar1, GetWorldRegionName(WORLD_REGION_COUNT)), 0);
        EXPECT_EQ(GetRegionWorldState(WORLD_REGION_HOENN), 42);
    }
    gSpecialVar_0x8004 = 0x100;
    EXPECT_EQ(Script_SetWorldPhase(), FALSE);
    EXPECT_EQ(Script_IsWorldPhaseAtLeast(), FALSE);
    EXPECT(SetWorldPhase(WORLD_PHASE_RISING));
    Script_BufferWorldPhaseName();
    EXPECT_EQ(StringCompare(gStringVar1, GetWorldPhaseName(WORLD_PHASE_RISING)), 0);
}

TEST("World State: mutations preserve populated Trainer Rank and quest storage")
{
    struct QuestSaveData questBefore;
    u16 rankBefore;
    u32 region;

    EXPECT(SetTrainerRank(3));
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0));
    EXPECT(QuestSetFavorite(QUEST_FRAMEWORK_TEST, TRUE));
    questBefore = gSaveBlock3Ptr->quests;
    rankBefore = VarGet(VAR_TRAINER_RANK);
    EXPECT(SetWorldPhase(WORLD_PHASE_LEGEND));
    for (region = 0; region < WORLD_REGION_COUNT; region++)
        EXPECT(SetRegionWorldState(region, 10 + region));
    EXPECT_EQ(SetRegionWorldState(0xFF, 0xFFFF), FALSE);
    EXPECT_EQ(SetWorldPhase(0xFFFF), FALSE);
    EXPECT_EQ(VarGet(VAR_TRAINER_RANK), rankBefore);
    EXPECT_EQ(memcmp(&questBefore, &gSaveBlock3Ptr->quests, sizeof(questBefore)), 0);
}
