#include "global.h"
#include "event_data.h"
#include "dynamic_encounters.h"
#include "trainer_rank.h"
#include "quests.h"
#include "mass_outbreak.h"
#include "world_state.h"
#include "test/test.h"

struct DynamicEncounterSnapshot
{
    u16 phase;
    u16 hoenn;
    u16 mapGroup;
    u16 mapNum;
};

static struct DynamicEncounterSnapshot SnapshotDynamicEncounterState(void)
{
    struct DynamicEncounterSnapshot snapshot;

    snapshot.phase = VarGet(VAR_WORLD_PHASE);
    snapshot.hoenn = GetRegionWorldState(WORLD_REGION_HOENN);
    snapshot.mapGroup = gSaveBlock1Ptr->location.mapGroup;
    snapshot.mapNum = gSaveBlock1Ptr->location.mapNum;
    return snapshot;
}

static void RestoreDynamicEncounterState(const struct DynamicEncounterSnapshot *snapshot)
{
    VarSet(VAR_WORLD_PHASE, snapshot->phase);
    VarSet(VAR_WORLD_STATE_HOENN, snapshot->hoenn);
    gSaveBlock1Ptr->location.mapGroup = snapshot->mapGroup;
    gSaveBlock1Ptr->location.mapNum = snapshot->mapNum;
}

TEST("Dynamic Encounter: unconfigured map returns default profile")
{
    struct DynamicEncounterSnapshot snapshot = SnapshotDynamicEncounterState();

    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE110), MAP_NUM(MAP_ROUTE110)), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);

    RestoreDynamicEncounterState(&snapshot);
}

TEST("Dynamic Encounter: Route101 resolves Hoenn World State profile correctly")
{
    struct DynamicEncounterSnapshot snapshot = SnapshotDynamicEncounterState();

    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 0));
    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), 1);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 1);

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 2));
    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), 2);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 2);

    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 4));
    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), 2);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 2);

    RestoreDynamicEncounterState(&snapshot);
}

TEST("Dynamic Encounter: other region state does not affect Hoenn test map")
{
    struct DynamicEncounterSnapshot snapshot = SnapshotDynamicEncounterState();

    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);
    EXPECT(SetRegionWorldState(WORLD_REGION_JOHTO, 7));
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 0));
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 1);

    RestoreDynamicEncounterState(&snapshot);
}

TEST("Dynamic Encounter: invalid map and invalid profile fall back safely")
{
    struct DynamicEncounterSnapshot snapshot = SnapshotDynamicEncounterState();

    EXPECT_EQ(GetDynamicEncounterProfileForMap(0xFF, 0xFF), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);
    EXPECT_EQ(GetDynamicEncounterProfileForMap(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), DYNAMIC_ENCOUNTER_PROFILE_DEFAULT);

    RestoreDynamicEncounterState(&snapshot);
}

TEST("Dynamic Encounter: Trainer Rank and quest state do not affect resolution")
{
    struct DynamicEncounterSnapshot snapshot = SnapshotDynamicEncounterState();

    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 1);
    QuestDebugReset();
    EXPECT(QuestStart(QUEST_FRAMEWORK_TEST));
    EXPECT(QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, 0));
    EXPECT(SetTrainerRank(5));
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 1);

    RestoreDynamicEncounterState(&snapshot);
}


TEST("Dynamic Encounter: effective land tables change immediately at every time")
{
    u32 time, slot;
    u16 header;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);
    header = GetCurrentMapWildMonHeaderId();
    EXPECT_NE(header, HEADER_NONE);
    for (time = 0; time < TIMES_OF_DAY_COUNT; time++)
    {
        EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 0));
        EXPECT_EQ(GetEffectiveWildEncounterTypes(header, time), &gWildMonHeaders[header].encounterTypes[time]);
        EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
        for (slot = 0; slot < NUM_LAND_MONS_ENCOUNTER_SLOTS; slot++)
            EXPECT_EQ(GetEffectiveWildEncounterTypes(header, time)->landMonsInfo->wildPokemon[slot].species,
                time == TIME_MORNING ? SPECIES_SABLEYE : time == TIME_DAY ? SPECIES_TREECKO : time == TIME_EVENING ? SPECIES_MAGIKARP : SPECIES_WAILMER);
        EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 65535));
        EXPECT_EQ(GetEffectiveWildEncounterTypes(header, time)->landMonsInfo->wildPokemon[0].species,
            time == TIME_MORNING ? SPECIES_TREECKO : time == TIME_DAY ? SPECIES_GROVYLE : time == TIME_EVENING ? SPECIES_MAGIKARP : SPECIES_WAILMER);
    }
    EXPECT_EQ(GetEffectiveWildEncounterTypes(HEADER_NONE, 0)->landMonsInfo, NULL);
    EXPECT_EQ(GetEffectiveWildEncounterTypes(header, TIMES_OF_DAY_COUNT)->landMonsInfo, NULL);
}

TEST("Dynamic Encounter: malformed rules fall back and priority is explicit")
{
    struct DynamicEncounterRule rules[2] =
    {
        {WORLD_REGION_HOENN, 1, 5, WORLD_PHASE_BEGINNING, 2},
        {WORLD_REGION_HOENN, 1, 5, WORLD_PHASE_BEGINNING, 1},
    };
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    EXPECT_EQ(TestResolveDynamicEncounterRules(rules, 2), 2);
    rules[0].profile = DYNAMIC_ENCOUNTER_PROFILE_COUNT;
    EXPECT_EQ(TestResolveDynamicEncounterRules(rules, 2), 0);
    rules[0].profile = 1;
    rules[0].region = 0x100;
    EXPECT_EQ(TestResolveDynamicEncounterRules(rules, 2), 0);
    rules[0].region = WORLD_REGION_HOENN;
    rules[0].minState = 6;
    EXPECT_EQ(TestResolveDynamicEncounterRules(rules, 2), 0);
    rules[0].minState = 1;
    rules[0].minPhase = WORLD_PHASE_COUNT;
    EXPECT_EQ(TestResolveDynamicEncounterRules(rules, 2), 0);
    EXPECT_EQ(GetDynamicEncounterProfileForMap(0x100, MAP_NUM(MAP_ROUTE101)), 0);
}

TEST("Dynamic Encounter: unconfigured land water fishing and all times retain exact tables")
{
    u16 header;
    u32 time;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE102);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE102);
    header = GetCurrentMapWildMonHeaderId();
    EXPECT_NE(header, HEADER_NONE);
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 2));
    for (time = 0; time < TIMES_OF_DAY_COUNT; time++)
        EXPECT_EQ(GetEffectiveWildEncounterTypes(header, time), &gWildMonHeaders[header].encounterTypes[time]);
    EXPECT_NE(GetEffectiveWildEncounterTypes(header, TIME_OF_DAY_DEFAULT)->landMonsInfo, NULL);
    EXPECT_NE(GetEffectiveWildEncounterTypes(header, TIME_OF_DAY_DEFAULT)->waterMonsInfo, NULL);
    EXPECT_NE(GetEffectiveWildEncounterTypes(header, TIME_OF_DAY_DEFAULT)->fishingMonsInfo, NULL);
    EXPECT(DoesCurrentMapHaveFishingMons());
    EXPECT_NE(GetLocalWaterMon(), SPECIES_NONE);
    EXPECT_EQ(GetTimeOfDayForEncounters(header, WILD_AREA_LAND), TIME_OF_DAY_DEFAULT);
}

TEST("Dynamic Encounter: normal generator consumes profile and outbreak remains separate")
{
    u16 header;
    gSaveBlock1Ptr->location.mapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->location.mapNum = MAP_NUM(MAP_ROUTE101);
    EXPECT(SetRegionWorldState(WORLD_REGION_HOENN, 1));
    header = GetCurrentMapWildMonHeaderId();
    EXPECT(TryGenerateWildMon(GetEffectiveWildEncounterTypes(header, TIME_OF_DAY_DEFAULT)->landMonsInfo, WILD_AREA_LAND, 0));
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES),
        GetEffectiveWildEncounterTypes(header, TIME_OF_DAY_DEFAULT)->landMonsInfo->wildPokemon[0].species);
    gSaveBlock1Ptr->outbreakLocationMapGroup = MAP_GROUP(MAP_ROUTE101);
    gSaveBlock1Ptr->outbreakLocationMapNum = MAP_NUM(MAP_ROUTE101);
    gSaveBlock1Ptr->outbreakDaysLeft = 1;
    gSaveBlock1Ptr->outbreakPokemonProbability = 100;
    gSaveBlock1Ptr->outbreakPokemonSpecies = SPECIES_SEEDOT;
    gSaveBlock1Ptr->outbreakPokemonLevel = 3;
    EXPECT(DoMassOutbreakEncounterTest());
    EXPECT(SetUpMassOutbreakEncounter(0));
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_SEEDOT);
    EXPECT_EQ(GetCurrentDynamicEncounterProfile(), 1);
}
