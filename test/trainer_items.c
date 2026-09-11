#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_ai_main.h"
#include "battle_ai_items.h"
#include "battle_ai_util.h"
#include "battle_controllers.h"
#include "battle_util.h"
#include "debug.h"
#include "malloc.h"
#include "pokemon.h"
#include "trainer_items.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "test/test.h"

extern const struct Trainer *(*gTestLATrainerItemSource)(const struct Trainer *);

static const enum Item sEmpty[MAX_TRAINER_ITEMS] = {ITEM_NONE};
static const enum Item sExpectedTiers[][MAX_TRAINER_ITEMS] =
{
    {ITEM_POTION, ITEM_POTION},
    {ITEM_SUPER_POTION, ITEM_FULL_HEAL},
    {ITEM_HYPER_POTION, ITEM_FULL_HEAL},
    {ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_FULL_HEAL},
    {ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_FULL_HEAL, ITEM_FULL_HEAL},
};

static void ExpectItems(const enum Item *actual, const enum Item *expected)
{
    for (u32 i = 0; i < MAX_TRAINER_ITEMS; i++)
        EXPECT_EQ(actual[i], expected[i]);
}

static void CheckProgress(u8 rank, u8 phase, u32 tier)
{
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, rank, phase, sEmpty);
    ExpectItems(result.items, sExpectedTiers[tier]);
}

TEST("Trainer Items: progress 0 minimum ordinary inventory") { CheckProgress(0, 0, 0); }
TEST("Trainer Items: progress 1") { CheckProgress(1, 0, 0); }
TEST("Trainer Items: progress 2") { CheckProgress(0, 2, 1); }
TEST("Trainer Items: progress 3") { CheckProgress(3, 0, 1); }
TEST("Trainer Items: progress 4") { CheckProgress(0, 4, 2); }
TEST("Trainer Items: progress 5") { CheckProgress(5, 0, 2); }
TEST("Trainer Items: progress 6") { CheckProgress(0, 6, 2); }
TEST("Trainer Items: progress 7") { CheckProgress(1, 6, 3); }
TEST("Trainer Items: progress 8") { CheckProgress(2, 6, 3); }
TEST("Trainer Items: progress 9") { CheckProgress(3, 6, 3); }
TEST("Trainer Items: progress 10") { CheckProgress(4, 6, 4); }
TEST("Trainer Items: progress 11") { CheckProgress(5, 6, 4); }

TEST("Trainer Items: invalid progression inputs clamp independently")
{
    CheckProgress(255, 0, 2);
    CheckProgress(0, 255, 2);
    CheckProgress(255, 255, 4);
}

TEST("Trainer Items: all rank phase combinations deterministic and capacity bounded")
{
    for (u32 rank = 0; rank < TRAINER_RANK_COUNT; rank++)
    {
        for (u32 phase = 0; phase < WORLD_PHASE_COUNT; phase++)
        {
            u32 sum = rank + phase;
            u32 tier = sum < 2 ? 0 : sum < 4 ? 1 : sum < 7 ? 2 : sum < 10 ? 3 : 4;
            struct LATrainerItemSet first = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, rank, phase, sEmpty);
            struct LATrainerItemSet second = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, rank, phase, sEmpty);
            ExpectItems(first.items, sExpectedTiers[tier]);
            ExpectItems(second.items, first.items);
            for (u32 i = 0; i < MAX_TRAINER_ITEMS; i++)
                EXPECT(first.items[i] == ITEM_NONE || first.items[i] == ITEM_POTION
                    || first.items[i] == ITEM_SUPER_POTION || first.items[i] == ITEM_HYPER_POTION
                    || first.items[i] == ITEM_FULL_HEAL);
        }
    }
}

TEST("Trainer Items: Major promotes every tier and caps at End")
{
    const u8 ranks[] = {0, 2, 4, 5, 5};
    const u8 phases[] = {0, 0, 0, 2, 6};
    for (u32 i = 0; i < ARRAY_COUNT(ranks); i++)
    {
        struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(1), TRUE, ranks[i], phases[i], sEmpty);
        ExpectItems(result.items, sExpectedTiers[min(i + 1, 4)]);
    }
}

TEST("Trainer Items: authored order retained and desired occurrences matched once")
{
    const enum Item authored[MAX_TRAINER_ITEMS] = {ITEM_NONE, ITEM_FULL_HEAL, ITEM_HYPER_POTION};
    const enum Item expected[MAX_TRAINER_ITEMS] = {ITEM_FULL_HEAL, ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_FULL_HEAL};
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, 5, 6, authored);
    ExpectItems(result.items, expected);
    EXPECT_EQ(authored[0], ITEM_NONE);
    EXPECT_EQ(authored[1], ITEM_FULL_HEAL);
}

TEST("Trainer Items: partial ordinary inventory preserves unrelated authored item")
{
    const enum Item authored[MAX_TRAINER_ITEMS] = {ITEM_REVIVE};
    const enum Item expected[MAX_TRAINER_ITEMS] = {ITEM_REVIVE, ITEM_HYPER_POTION, ITEM_HYPER_POTION};
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, 5, 2, authored);
    ExpectItems(result.items, expected);
}

TEST("Trainer Items: full authored inventory and above-target sparse inventory unchanged")
{
    const enum Item full[MAX_TRAINER_ITEMS] = {ITEM_MAX_POTION, ITEM_FULL_RESTORE, ITEM_REVIVE, ITEM_MAX_REVIVE};
    const enum Item sparse[MAX_TRAINER_ITEMS] = {ITEM_NONE, ITEM_MAX_POTION, ITEM_NONE, ITEM_FULL_RESTORE};
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, 5, 6, full);
    ExpectItems(result.items, full);
    result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), TRUE, 0, 0, sparse);
    ExpectItems(result.items, sparse);
}

TEST("Trainer Items: HANDCRAFTED nonempty exact array precedence")
{
    const enum Item authored[MAX_TRAINER_ITEMS] = {ITEM_NONE, ITEM_FULL_RESTORE, ITEM_NONE, ITEM_REVIVE};
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(1), TRUE, 5, 6, authored);
    ExpectItems(result.items, authored);
}

TEST("Trainer Items: HANDCRAFTED empty inventory gets promoted tier")
{
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(1), TRUE, 0, 0, sEmpty);
    ExpectItems(result.items, sExpectedTiers[1]);
}

TEST("Trainer Items: SPECIAL EXEMPT missing policy and runtime exclusion preserve exact arrays")
{
    const enum Item authored[MAX_TRAINER_ITEMS] = {ITEM_NONE, ITEM_REVIVE, ITEM_NONE, ITEM_POTION};
    struct LATrainerPolicy policies[] = {GetLATrainerPolicy(TRAINER_PARTNER(1)), GetLATrainerPolicy(0xFFFF), GetLATrainerPolicy(0)};
    policies[2].flags &= ~LA_TRAINER_POLICY_COMPETITIVE_ITEMS;
    for (u32 i = 0; i < ARRAY_COUNT(policies); i++)
    {
        struct LATrainerItemSet result = BuildLATrainerBattleItems(policies[i], TRUE, 5, 6, authored);
        ExpectItems(result.items, authored);
    }
    struct LATrainerItemSet result = BuildLATrainerBattleItems(GetLATrainerPolicy(0), FALSE, 5, 6, authored);
    ExpectItems(result.items, authored);
}

// Exercise the real setup and consumption functions with their engine state.
// Only the authored source can be substituted; selection/eligibility/ownership
// and the item AI execute unchanged.
static void SetupItemContext(u32 flags)
{
    gBattleHistory = AllocZeroed(sizeof(*gBattleHistory));
    gBattleStruct = AllocZeroed(sizeof(*gBattleStruct));
    gAiLogicData = AllocZeroed(sizeof(*gAiLogicData));
    gAiBattleData = AllocZeroed(sizeof(*gAiBattleData));
    gAiThinkingStruct = AllocZeroed(sizeof(*gAiThinkingStruct));
    gBattleResources = AllocZeroed(sizeof(*gBattleResources));
    gAiPartyData = AllocZeroed(sizeof(*gAiPartyData));
    gIsDebugBattle = FALSE;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER | flags;
    gBattlersCount = flags & BATTLE_TYPE_DOUBLE ? 4 : 2;
    TRAINER_BATTLE_PARAM.opponentA = 0;
    TRAINER_BATTLE_PARAM.opponentB = 1;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    SetTrainerRank(0);
    SetWorldPhase(0);
    gTestLATrainerItemSource = NULL;
    memset(gBattleMons, 0, sizeof(gBattleMons));
    memset(gParties, 0, sizeof(gParties));
    gAiBattleData->actionFlee = TRUE; // No planned KO in the default fixture.
    for (u32 i = 0; i < MAX_BATTLERS_COUNT; i++)
    {
        gBattlerPositions[i] = i;
        gBattlerPartyIndexes[i] = 0;
        gBattlerBattleController[i] = i & BIT_SIDE ? BATTLE_CONTROLLER_OPPONENT : BATTLE_CONTROLLER_PLAYER;
        gBattleStruct->itemPartyIndex[i] = PARTY_SIZE;
        gBattleMons[i].hp = gBattleMons[i].maxHP = 100;
        gBattleMons[i].species = SPECIES_WOBBUFFET;
        gAiThinkingStruct->aiFlags[i] = LA_SMART_AI_MASK;
        CreateMon(&gParties[i][0], SPECIES_WOBBUFFET, 50, 0, (struct OriginalTrainerId){0});
        CalculateMonStats(&gParties[i][0]);
    }
}

static void TearDownItemContext(void)
{
    gTestLATrainerItemSource = NULL;
    FREE_AND_SET_NULL(gBattleHistory);
    FREE_AND_SET_NULL(gBattleStruct);
    FREE_AND_SET_NULL(gAiLogicData);
    FREE_AND_SET_NULL(gAiBattleData);
    FREE_AND_SET_NULL(gAiThinkingStruct);
    FREE_AND_SET_NULL(gBattleResources);
    FREE_AND_SET_NULL(gAiPartyData);
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    gIsDebugBattle = FALSE;
}

TEST("Trainer Items: setup initializes A and resets whole history")
{
    SetupItemContext(0);
    memset(gBattleHistory, 0xFF, sizeof(*gBattleHistory));
    BattleAI_SetupItems();
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], sExpectedTiers[0]);
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_B], sEmpty);
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_PLAYER], sEmpty);
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_PARTNER], sEmpty);
    EXPECT_EQ(gBattleHistory->usedMoves[0][0], MOVE_NONE);
    TearDownItemContext();
}

TEST("Trainer Items: setup two opponents independently resolves ordinary and Major")
{
    SetupItemContext(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS);
    BattleAI_SetupItems();
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], sExpectedTiers[0]);
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_B], sExpectedTiers[1]);
    TearDownItemContext();
}

TEST("Trainer Items: existing whole-format exclusions leave all banks empty")
{
    const u32 excluded[] = {BATTLE_TYPE_LINK, BATTLE_TYPE_SAFARI, BATTLE_TYPE_BATTLE_TOWER,
        BATTLE_TYPE_EREADER_TRAINER, BATTLE_TYPE_SECRET_BASE, BATTLE_TYPE_FRONTIER,
        BATTLE_TYPE_INGAME_PARTNER, BATTLE_TYPE_RECORDED_LINK};
    SetupItemContext(0);
    for (u32 i = 0; i < ARRAY_COUNT(excluded); i++)
    {
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | excluded[i];
        memset(gBattleHistory, 0xFF, sizeof(*gBattleHistory));
        BattleAI_SetupItems();
        for (u32 owner = 0; owner < MAX_BATTLE_TRAINERS; owner++)
            ExpectItems(gBattleHistory->trainerItems[owner], sEmpty);
    }
    gBattleTypeFlags = 0;
    BattleAI_SetupItems();
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], sEmpty);
    TearDownItemContext();
}

static const struct Trainer sRequester = {.items = {ITEM_REVIVE}, .overrideTrainer = 1};
static const struct Trainer sDonor = {.items = {ITEM_MAX_POTION}};
static const struct Trainer sHard = {.items = {ITEM_FULL_HEAL}};
static const struct Trainer sFallback = {.items = {ITEM_NONE, ITEM_MAX_REVIVE}};

static const struct Trainer *ItemSourceFixture(const struct Trainer *selected)
{
    if (selected == &gTrainers[DIFFICULTY_NORMAL][3])
        return &sRequester;
    if (selected == &gTrainers[DIFFICULTY_NORMAL][1])
        return &sDonor;
    if (selected == &gTrainers[DIFFICULTY_HARD][5])
        return &sHard;
    if (selected == &gTrainers[DIFFICULTY_NORMAL][0])
        return &sFallback;
    return selected;
}

TEST("Trainer Items: overrideTrainer retains requester inventory and ROM is unchanged")
{
    SetupItemContext(0);
    gTestLATrainerItemSource = ItemSourceFixture;
    TRAINER_BATTLE_PARAM.opponentA = 3;
    struct Trainer before = gTrainers[DIFFICULTY_NORMAL][3];
    BattleAI_SetupItems();
    const enum Item expected[MAX_TRAINER_ITEMS] = {ITEM_REVIVE, ITEM_POTION};
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], expected);
    EXPECT_EQ(sRequester.overrideTrainer, 1);
    EXPECT_EQ(memcmp(&before, &gTrainers[DIFFICULTY_NORMAL][3], sizeof(before)), 0);
    TearDownItemContext();
}

TEST("Trainer Items: selected difficulty and missing-variant fallback preserved")
{
    SetupItemContext(0);
    gTestLATrainerItemSource = ItemSourceFixture;
    SetCurrentDifficultyLevel(DIFFICULTY_HARD);
    TRAINER_BATTLE_PARAM.opponentA = 5;
    BattleAI_SetupItems();
    const enum Item hard[MAX_TRAINER_ITEMS] = {ITEM_FULL_HEAL, ITEM_POTION};
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], hard);
    TRAINER_BATTLE_PARAM.opponentA = 0;
    BattleAI_SetupItems();
    const enum Item fallback[MAX_TRAINER_ITEMS] = {ITEM_MAX_REVIVE, ITEM_POTION};
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], fallback);
    TearDownItemContext();
}

TEST("Trainer Items: mixed eligibility and invalid-ID fallback stay isolated")
{
    SetupItemContext(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS);
    gTestLATrainerItemSource = ItemSourceFixture;
    TRAINER_BATTLE_PARAM.opponentA = 3;
    TRAINER_BATTLE_PARAM.opponentB = TRAINERS_COUNT;
    BattleAI_SetupItems();
    const enum Item expected[MAX_TRAINER_ITEMS] = {ITEM_REVIVE, ITEM_POTION};
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], expected);
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_B], sFallback.items);
    TearDownItemContext();
}

TEST("Trainer Items: runtime HANDCRAFTED authored inventory is authoritative")
{
    SetupItemContext(0);
    gTestLATrainerItemSource = ItemSourceFixture;
    TRAINER_BATTLE_PARAM.opponentA = 1;
    SetTrainerRank(5);
    SetWorldPhase(6);
    BattleAI_SetupItems();
    ExpectItems(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A], sDonor.items);
    TearDownItemContext();
}

TEST("Trainer Items: Full Heal consumes only first matching finite use and emits action")
{
    SetupItemContext(0);
    enum Item *bank = gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A];
    bank[0] = bank[1] = ITEM_FULL_HEAL;
    gBattleMons[1].status1 = STATUS1_SLEEP;
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(bank[0], ITEM_NONE);
    EXPECT_EQ(bank[1], ITEM_FULL_HEAL);
    EXPECT_EQ(gBattleStruct->chosenItem[1], ITEM_FULL_HEAL);
    EXPECT_EQ(gBattleStruct->itemPartyIndex[1], 0);
    EXPECT_EQ(gBattleResources->bufferB[1][1], B_ACTION_USE_ITEM);
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(bank[1], ITEM_NONE);
    EXPECT(!ShouldUseItem(B_BATTLER_1));
    ExpectItems(bank, sEmpty);
    TearDownItemContext();
}

TEST("Trainer Items: one-trainer doubles share A consumption with no replenishment")
{
    SetupItemContext(BATTLE_TYPE_DOUBLE);
    enum Item *bank = gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A];
    bank[0] = ITEM_FULL_HEAL;
    gBattleMons[1].status1 = gBattleMons[3].status1 = STATUS1_SLEEP;
    EXPECT_EQ(GetBattlerTrainer(1), B_TRAINER_OPPONENT_A);
    EXPECT_EQ(GetBattlerTrainer(3), B_TRAINER_OPPONENT_A);
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT(!ShouldUseItem(B_BATTLER_3));
    EXPECT_EQ(bank[0], ITEM_NONE);
    TearDownItemContext();
}

TEST("Trainer Items: two-opponent consumption isolates A and B")
{
    SetupItemContext(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_TWO_OPPONENTS);
    enum Item *a = gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A];
    enum Item *b = gBattleHistory->trainerItems[B_TRAINER_OPPONENT_B];
    a[0] = b[0] = ITEM_FULL_HEAL;
    gBattleMons[1].status1 = gBattleMons[3].status1 = STATUS1_SLEEP;
    EXPECT_EQ(GetBattlerTrainer(3), B_TRAINER_OPPONENT_B);
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(a[0], ITEM_NONE);
    EXPECT_EQ(b[0], ITEM_FULL_HEAL);
    EXPECT(ShouldUseItem(B_BATTLER_3));
    EXPECT_EQ(b[0], ITEM_NONE);
    EXPECT(!ShouldUseItem(B_BATTLER_1));
    TearDownItemContext();
}

TEST("Trainer Items: player partner and invalid battlers cannot consume enemy banks")
{
    SetupItemContext(BATTLE_TYPE_DOUBLE | BATTLE_TYPE_MULTI | BATTLE_TYPE_INGAME_PARTNER);
    for (u32 owner = 0; owner < MAX_BATTLE_TRAINERS; owner++)
        gBattleHistory->trainerItems[owner][0] = ITEM_FULL_HEAL;
    gBattleMons[0].status1 = gBattleMons[2].status1 = STATUS1_SLEEP;
    EXPECT(!ShouldUseItem(B_BATTLER_0));
    EXPECT(!ShouldUseItem(B_BATTLER_2));
    EXPECT(!ShouldUseItem(MAX_BATTLERS_COUNT));
    EXPECT(!ShouldUseItem(255));
    for (u32 owner = 0; owner < MAX_BATTLE_TRAINERS; owner++)
        EXPECT_EQ(gBattleHistory->trainerItems[owner][0], ITEM_FULL_HEAL);
    TearDownItemContext();
}

TEST("Trainer Items: useful healing consumes one of two Hyper Potions")
{
    SetupItemContext(0);
    gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_HYPER_POTION;
    gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][1] = ITEM_HYPER_POTION;
    gBattleMons[1].hp = 10;
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(gBattleStruct->chosenItem[1], ITEM_HYPER_POTION);
    EXPECT_EQ(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0], ITEM_NONE);
    EXPECT_EQ(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][1], ITEM_HYPER_POTION);
    TearDownItemContext();
}

static void SetDamage(u32 attacker, u32 defender, u16 damage)
{
    gAiLogicData->simulatedDmg[attacker][defender][0] = (struct SimulatedDamage)
        {.minimum = damage, .median = damage, .maximum = damage, .random = damage};
}

TEST("Trainer Items: futile healing declined without consuming inventory")
{
    SetupItemContext(0);
    gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_POTION;
    gBattleMons[1].hp = 10;
    gBattleMons[0].moves[0] = MOVE_TACKLE;
    SetDamage(0, 1, 200);
    EXPECT(AI_OpponentCanFaintAiWithMod(1, 20));
    EXPECT(!ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0], ITEM_POTION);
    TearDownItemContext();
}

TEST("Trainer Items: immediate KO preference beats status cure")
{
    SetupItemContext(0);
    gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_FULL_HEAL;
    gBattleMons[1].status1 = STATUS1_POISON;
    gBattleMons[1].moves[0] = MOVE_QUICK_ATTACK;
    gBattleMons[0].moves[0] = MOVE_TACKLE;
    gAiBattleData->actionFlee = FALSE;
    gAiBattleData->chosenTarget[1] = 0;
    gAiLogicData->speedStats[1] = 200;
    gAiLogicData->speedStats[0] = 10;
    SetDamage(1, 0, 200);
    EXPECT(AiExpectsToFaintPlayer(1));
    EXPECT(!ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0], ITEM_FULL_HEAL);
    TearDownItemContext();
}

TEST("Trainer Items: authored Revive retains upstream party targeting")
{
    SetupItemContext(0);
    gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_REVIVE;
    CreateMon(&gParties[B_TRAINER_OPPONENT_A][1], SPECIES_WOBBUFFET, 50, 0, (struct OriginalTrainerId){0});
    u16 hp = 0;
    SetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_HP, &hp);
    EXPECT(ShouldUseItem(B_BATTLER_1));
    EXPECT_EQ(gBattleStruct->chosenItem[1], ITEM_REVIVE);
    EXPECT_EQ(gBattleStruct->itemPartyIndex[1], 1);
    EXPECT_EQ(gBattleHistory->trainerItems[B_TRAINER_OPPONENT_A][0], ITEM_NONE);
    TearDownItemContext();
}
