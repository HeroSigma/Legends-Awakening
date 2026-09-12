#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "debug.h"
#include "pokemon.h"
#include "random.h"
#include "trainer_sets.h"
#include "trainer_roster.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "trainer_evolution.h"
#include "test/test.h"

extern void TestCreateLATrainerPartyWithPolicyForId(struct Pokemon *, const struct Trainer *, u16, struct LATrainerPolicy);
extern void TestCreateLATrainerSetParty(struct Pokemon *, const struct Trainer *, u16,
    struct LATrainerPolicy, const struct LASetAssignment *, u32);

#define MON(s, l) {.species = s, .lvl = l, .gender = TRAINER_MON_RANDOM_GENDER}
static const struct TrainerMon sSources[5][3] = {
    {MON(SPECIES_GEODUDE, 21)},
    {MON(SPECIES_GEODUDE, 26), MON(SPECIES_NUMEL, 26)},
    {MON(SPECIES_MACHOP, 28), MON(SPECIES_NUMEL, 28), MON(SPECIES_GRAVELER, 28)},
    {MON(SPECIES_MACHOP, 30), MON(SPECIES_NUMEL, 30), MON(SPECIES_GRAVELER, 30)},
    {MON(SPECIES_MACHOKE, 33), MON(SPECIES_CAMERUPT, 33), MON(SPECIES_GOLEM, 33)},
};
static const u16 sIds[] = {TRAINER_SAWYER_1, TRAINER_SAWYER_2, TRAINER_SAWYER_3, TRAINER_SAWYER_4, TRAINER_SAWYER_5};
static const u8 sCounts[] = {1, 2, 3, 3, 3};
static const u8 sLevels[] = {21, 26, 28, 30, 33};
static const enum Species sExpected[5][6] = {
    {SPECIES_GEODUDE, SPECIES_NUMEL, SPECIES_MACHOP, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_GRAVELER, SPECIES_NUMEL, SPECIES_MACHOP, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOKE, SPECIES_NUMEL, SPECIES_GRAVELER, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOKE, SPECIES_NUMEL, SPECIES_GRAVELER, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOKE, SPECIES_CAMERUPT, SPECIES_GOLEM, SPECIES_LAIRON, SPECIES_ONIX, SPECIES_TORKOAL},
};
static const struct LASetBundle *GeodudeBundle(void)
{
    return GetLASetBundle(TRAINER_SAWYER_1, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE);
}

static void Setup(void)
{
    gIsDebugBattle = FALSE;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    ZeroPlayerPartyMons();
    SetTrainerRank(TRAINER_RANK_ROOKIE);
    SetWorldPhase(WORLD_PHASE_BEGINNING);
}

TEST("Trainer Sets: lookup authority, identity and fail closed")
{
    const struct LASetBundle *bundle = GeodudeBundle();
    ASSUME(bundle != NULL);
    struct LASetAssignment rows[] = {
        {0, LA_SET_DIFFICULTY_ANY, 7, SPECIES_GEODUDE, bundle},
        {0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE, bundle},
    };
    EXPECT(FindLASetBundle(rows, 1, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == bundle);
    rows[1].bundle = NULL;
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    rows[1].bundle = bundle;
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == bundle);
    EXPECT(FindLASetBundle(rows, 2, 3, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 8, SPECIES_GEODUDE) == NULL);
    rows[1].authoredSpecies = SPECIES_ONIX;
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    rows[1] = rows[0];
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    rows[0].difficulty = rows[1].difficulty = DIFFICULTY_NORMAL;
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    rows[1].difficulty = DIFFICULTY_COUNT;
    EXPECT(FindLASetBundle(rows, 2, 0, DIFFICULTY_NORMAL, 7, SPECIES_GEODUDE) == NULL);
    EXPECT(GetLASetBundle(0, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE) == NULL);
    EXPECT(GetLASetBundle(TRAINER_SAWYER_1, DIFFICULTY_HARD, 0, SPECIES_GEODUDE) == NULL);
}

static void ExpectAtomicFailure(const struct LACompetitiveSet *set, enum Species species)
{
    const struct LACompetitiveSet *variants[] = {set};
    const struct LASetBundle bundle = {variants, 1};
    struct TrainerMon mon = MON(species, 37);
    mon.iv = 123;
    struct TrainerMon before = mon;
    EXPECT(!ApplyLACompetitiveSet(&mon, &bundle));
    EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
}

TEST("Trainer Sets: malformed training fails atomically")
{
    struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
    struct LASetTraining training = *set.training;
    set.training = &training;
    training.evs[0] = 253;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    training.evs[0] = 252;
    training.evs[2] = 252;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    training = *GeodudeBundle()->variants[0]->training;
    training.nature = NUM_NATURES;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    set.training = NULL;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
}

TEST("Trainer Sets: invalid or unavailable ability fails atomically")
{
    struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
    set.abilitySlot = NUM_ABILITY_SLOTS;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    set.finalSpecies = SPECIES_MAGIKARP;
    set.abilitySlot = 1;
    ASSUME(gSpeciesInfo[SPECIES_MAGIKARP].abilities[1] == ABILITY_NONE);
    ExpectAtomicFailure(&set, SPECIES_MAGIKARP);
}

TEST("Trainer Sets: malformed and incompatible moves fail atomically")
{
    const enum Move invalid[] = {MOVE_NONE, MOVES_COUNT, MOVES_COUNT_ALL, MOVE_UNAVAILABLE, MOVE_SPORE, MOVE_RETURN, MOVE_FRUSTRATION};
    for (u32 i = 0; i < ARRAY_COUNT(invalid); i++)
    {
        struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
        set.moves[0] = invalid[i];
        ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    }
    struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
    set.moves[0] = set.moves[1];
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
}

TEST("Trainer Sets: items reject invalid, gimmicks and fully evolved Eviolite")
{
    const enum Item invalid[] = {ITEM_NONE, ITEMS_COUNT, ITEM_RED_ORB, ITEM_VENUSAURITE, ITEM_NORMALIUM_Z, ITEM_EVERSTONE};
    for (u32 i = 0; i < ARRAY_COUNT(invalid); i++)
    {
        struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
        set.heldItem = invalid[i];
        ExpectAtomicFailure(&set, SPECIES_GEODUDE);
    }
    struct LACompetitiveSet set = *GetLASetBundle(TRAINER_SAWYER_5, DIFFICULTY_NORMAL, 2, SPECIES_GOLEM)->variants[0];
    set.heldItem = ITEM_EVIOLITE;
    ExpectAtomicFailure(&set, SPECIES_GOLEM);
}

TEST("Trainer Sets: missing and ambiguous final variants preserve bytes")
{
    struct TrainerMon mon = MON(SPECIES_LUXRAY, 40);
    struct TrainerMon before = mon;
    EXPECT(!ApplyLACompetitiveSet(&mon, GeodudeBundle()));
    EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
    const struct LACompetitiveSet *variants[] = {GeodudeBundle()->variants[0], GeodudeBundle()->variants[0]};
    struct LASetBundle bundle = {variants, 2};
    mon.species = SPECIES_GEODUDE;
    before = mon;
    EXPECT(!ApplyLACompetitiveSet(&mon, &bundle));
    EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
}

TEST("Trainer Sets: success preserves all unrelated bytes, source, metadata and RNG")
{
    struct TrainerMon source = MON(SPECIES_GEODUDE, 5);
    source.friendship = 91;
    source.teraType = TYPE_WATER;
    source.shouldUseDynamax = TRUE;
    source.dynamaxLevel = 7;
    source.gigantamaxFactor = TRUE;
    source.tags = 12345;
    source.ball = BALL_ULTRA;
    source.isShiny = TRUE;
    struct TrainerMon mon = source, expected = source, sourceBefore = source;
    const struct LACompetitiveSet *set = GeodudeBundle()->variants[0];
    struct LACompetitiveSet setBefore = *set;
    struct LASetTraining trainingBefore = *set->training;
    rng_value_t r1 = gRngValue, r2 = gRng2Value;
    EXPECT(ApplyLACompetitiveSet(&mon, GeodudeBundle()));
    expected.iv = LA_SET_PERFECT_IVS;
    expected.ev = set->training->evs;
    expected.nature = set->training->nature;
    expected.ability = ABILITY_STURDY;
    expected.heldItem = ITEM_EVIOLITE;
    memcpy(expected.moves, set->moves, sizeof(expected.moves));
    EXPECT_EQ(memcmp(&mon, &expected, sizeof(mon)), 0);
    EXPECT_EQ(memcmp(&source, &sourceBefore, sizeof(source)), 0);
    EXPECT_EQ(memcmp(set, &setBefore, sizeof(setBefore)), 0);
    EXPECT_EQ(memcmp(set->training, &trainingBefore, sizeof(trainingBefore)), 0);
    EXPECT_EQ(memcmp(&r1, &gRngValue, sizeof(r1)), 0);
    EXPECT_EQ(memcmp(&r2, &gRng2Value, sizeof(r2)), 0);
}

static void CheckGenerated(struct Pokemon *mon, const struct LACompetitiveSet *set)
{
    const u8 physical[] = {252, 252, 0, 0, 4, 0};
    const u8 special[] = {252, 0, 0, 252, 4, 0};
    const u8 utility[] = {252, 0, 252, 0, 4, 0};
    const u8 *expected = physical;
    if (set->training->nature == NATURE_MODEST)
        expected = special;
    else if (set->training->nature == NATURE_IMPISH || set->training->nature == NATURE_BOLD)
        expected = utility;
    const s32 evFields[] = {MON_DATA_HP_EV, MON_DATA_ATK_EV, MON_DATA_DEF_EV, MON_DATA_SPATK_EV, MON_DATA_SPDEF_EV, MON_DATA_SPEED_EV};
    const s32 ivFields[] = {MON_DATA_HP_IV, MON_DATA_ATK_IV, MON_DATA_DEF_IV, MON_DATA_SPATK_IV, MON_DATA_SPDEF_IV, MON_DATA_SPEED_IV};
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT_EQ(GetMonData(mon, evFields[i]), set->training->evs[i]);
        EXPECT_EQ(GetMonData(mon, evFields[i]), expected[i]);
        EXPECT_EQ(GetMonData(mon, ivFields[i]), 31);
    }
    EXPECT_EQ(GetNature(mon), set->training->nature);
    EXPECT_EQ(GetAbilityBySpecies(set->finalSpecies, GetMonData(mon, MON_DATA_ABILITY_NUM)),
        gSpeciesInfo[set->finalSpecies].abilities[set->abilitySlot]);
    EXPECT_EQ(GetMonData(mon, MON_DATA_HELD_ITEM), set->heldItem);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        EXPECT_EQ(GetMonData(mon, MON_DATA_MOVE1 + i), set->moves[i]);
}

static void CheckSawyer(u32 stage)
{
    Setup();
    struct Trainer trainer = {.party = sSources[stage], .partySize = sCounts[stage]};
    struct LARosterSelection selection = SelectLARoster(&trainer, GetLARosterProfile(sIds[stage], DIFFICULTY_NORMAL), 6);
    ASSUME(selection.count == 6);
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer, sIds[stage], GetLATrainerPolicy(0));
    for (u32 i = 0; i < 6; i++)
    {
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), sExpected[stage][i]);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), sLevels[stage]);
        const struct LASetBundle *bundle = GetLASetBundle(sIds[stage], DIFFICULTY_NORMAL,
            selection.members[i].sourceKey, selection.members[i].source->species);
        ASSUME(bundle != NULL);
        const struct LACompetitiveSet *set = NULL;
        for (u32 j = 0; j < bundle->count; j++)
            if (bundle->variants[j]->finalSpecies == sExpected[stage][i])
                set = bundle->variants[j];
        ASSUME(set != NULL);
        CheckGenerated(mon, set);
    }
}

TEST("Trainer Sets: Sawyer 1 exact six competitive members") { CheckSawyer(0); }
TEST("Trainer Sets: Sawyer 2 exact six competitive members") { CheckSawyer(1); }
TEST("Trainer Sets: Sawyer 3 exact six competitive members") { CheckSawyer(2); }
TEST("Trainer Sets: Sawyer 4 exact six competitive members") { CheckSawyer(3); }
TEST("Trainer Sets: Sawyer 5 exact six competitive members") { CheckSawyer(4); }

TEST("Trainer Sets: every reachable pilot variant validates and resolves named ability")
{
    u32 seen = 0;
    for (u32 stage = 0; stage < 5; stage++)
    {
        struct Trainer trainer = {.party = sSources[stage], .partySize = sCounts[stage]};
        struct LARosterSelection selection = SelectLARoster(&trainer, GetLARosterProfile(sIds[stage], DIFFICULTY_NORMAL), 6);
        for (u32 i = 0; i < selection.count; i++)
        {
            const struct LASetBundle *bundle = GetLASetBundle(sIds[stage], DIFFICULTY_NORMAL,
                selection.members[i].sourceKey, selection.members[i].source->species);
            ASSUME(bundle != NULL);
            seen++;
            for (u32 j = 0; j < bundle->count; j++)
            {
                const struct LACompetitiveSet *set = bundle->variants[j];
                struct TrainerMon mon = MON(set->finalSpecies, 5);
                EXPECT(ApplyLACompetitiveSet(&mon, bundle));
                enum Ability expected = ABILITY_STURDY;
                switch (mon.species)
                {
                case SPECIES_MACHOP: case SPECIES_MACHOKE: expected = ABILITY_NO_GUARD; break;
                case SPECIES_NUMEL: expected = ABILITY_SIMPLE; break;
                case SPECIES_CAMERUPT: expected = ABILITY_SOLID_ROCK; break;
                case SPECIES_TORKOAL: expected = ABILITY_WHITE_SMOKE; break;
                default: break;
                }
                EXPECT_EQ(mon.ability, expected);
            }
        }
    }
    EXPECT_EQ(seen, 30);
}

TEST("Trainer Sets: one mon without FULL PARTY or roster profile gets complete set")
{
    Setup();
    const struct TrainerMon source = MON(SPECIES_GEODUDE, 21);
    const struct Trainer trainer = {.party = &source, .partySize = 1};
    const struct LASetAssignment row = {0, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE, GeodudeBundle()};
    struct LATrainerPolicy policy = {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_COMPETITIVE_SETS};
    TestCreateLATrainerSetParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, policy, &row, 1);
    CheckGenerated(&gParties[B_TRAINER_OPPONENT_A][0], GeodudeBundle()->variants[0]);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_SPECIES), SPECIES_NONE);
}

TEST("Trainer Sets: absent assignment keeps authored IVs and held item")
{
    Setup();
    struct TrainerMon source = MON(SPECIES_GEODUDE, 21);
    source.heldItem = ITEM_ORAN_BERRY;
    const struct Trainer trainer = {.party = &source, .partySize = 1};
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0));
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV), 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_ORAN_BERRY);
}

TEST("Trainer Sets: protected categories and unsupported contexts bypass")
{
    struct LATrainerPolicy p = {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_COMPETITIVE_SETS};
    EXPECT(CanApplyLACompetitiveSets(0, p, TRUE, BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE, FALSE));
    EXPECT(!CanApplyLACompetitiveSets(0, p, FALSE, BATTLE_TYPE_TRAINER, FALSE));
    EXPECT(!CanApplyLACompetitiveSets(0, p, TRUE, BATTLE_TYPE_TRAINER, TRUE));
    const u32 flags[] = {0, BATTLE_TYPE_TRAINER | BATTLE_TYPE_TWO_OPPONENTS,
        BATTLE_TYPE_TRAINER | BATTLE_TYPE_MULTI, BATTLE_TYPE_TRAINER | BATTLE_TYPE_LINK,
        BATTLE_TYPE_TRAINER | BATTLE_TYPE_FRONTIER, BATTLE_TYPE_TRAINER | BATTLE_TYPE_INGAME_PARTNER};
    for (u32 i = 0; i < ARRAY_COUNT(flags); i++)
        EXPECT(!CanApplyLACompetitiveSets(0, p, TRUE, flags[i], FALSE));
    for (u32 category = LA_TRAINER_MAJOR; category <= LA_TRAINER_EXEMPT; category++)
    {
        p.category = category;
        EXPECT(!CanApplyLACompetitiveSets(0, p, TRUE, BATTLE_TYPE_TRAINER, FALSE));
    }
    p.category = LA_TRAINER_ORDINARY;
    p.flags |= LA_TRAINER_POLICY_HANDCRAFTED;
    EXPECT(!CanApplyLACompetitiveSets(0, p, TRUE, BATTLE_TYPE_TRAINER, FALSE));
}

static const struct LASetTraining sStageTraining = {{252, 0, 0, 252, 4, 0}, NATURE_MODEST};
#define STAGE_SET(s, item) {s, &sStageTraining, {MOVE_THUNDERBOLT, MOVE_THUNDER_WAVE, MOVE_PROTECT, MOVE_DOUBLE_TEAM}, item, 2}
static const struct LACompetitiveSet sStageSets[] = {
    STAGE_SET(SPECIES_SHINX, ITEM_EVIOLITE),
    STAGE_SET(SPECIES_LUXIO, ITEM_EVIOLITE),
    STAGE_SET(SPECIES_LUXRAY, ITEM_LEFTOVERS),
};
static const struct LACompetitiveSet *const sStageVariants[] = {&sStageSets[0], &sStageSets[1], &sStageSets[2]};
static const struct LASetBundle sStageBundle = {sStageVariants, 3};

TEST("Trainer Sets: Shinx source selects exact post-evolution stage and Hidden Ability")
{
    const u8 levels[] = {5, 20, 30};
    const u8 scaledLevels[] = {6, 20, 30}; // Existing rookie world floor is six.
    for (u32 i = 0; i < 3; i++)
    {
        Setup();
        struct TrainerMon source = MON(SPECIES_SHINX, levels[i]);
        struct TrainerMon before = source;
        const struct Trainer trainer = {.party = &source, .partySize = 1};
        const struct LASetAssignment row = {0, DIFFICULTY_NORMAL, 0, SPECIES_SHINX, &sStageBundle};
        struct LATrainerPolicy policy = GetLATrainerPolicy(0);
        policy.flags &= ~LA_TRAINER_POLICY_FULL_PARTY;
        TestCreateLATrainerSetParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, policy, &row, 1);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), sStageSets[i].finalSpecies);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), scaledLevels[i]);
        CheckGenerated(&gParties[B_TRAINER_OPPONENT_A][0], &sStageSets[i]);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_ABILITY_NUM), 2);
        EXPECT_EQ(memcmp(&source, &before, sizeof(source)), 0);
    }
}

TEST("Trainer Sets: missing evolved variant preserves scaled evolved source defaults")
{
    Setup();
    const struct TrainerMon source = MON(SPECIES_SHINX, 30);
    const struct Trainer trainer = {.party = &source, .partySize = 1};
    const struct LASetBundle bundle = {sStageVariants, 1};
    const struct LASetAssignment row = {0, DIFFICULTY_NORMAL, 0, SPECIES_SHINX, &bundle};
    TestCreateLATrainerSetParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &row, 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 30);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV), 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_NONE);
}

TEST("Trainer Sets: actual construction bypasses protected policy and debug contexts")
{
    const struct LATrainerPolicy policies[] = {
        {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_COMPETITIVE_SETS | LA_TRAINER_POLICY_HANDCRAFTED},
        {LA_TRAINER_SPECIAL, LA_TRAINER_POLICY_COMPETITIVE_SETS},
        {LA_TRAINER_EXEMPT, LA_TRAINER_POLICY_COMPETITIVE_SETS},
        {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_COMPETITIVE_SETS},
    };
    const struct TrainerMon source = MON(SPECIES_GEODUDE, 21);
    const struct Trainer trainer = {.party = &source, .partySize = 1};
    const struct LASetAssignment row = {0, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE, GeodudeBundle()};
    for (u32 i = 0; i < ARRAY_COUNT(policies); i++)
    {
        Setup();
        gIsDebugBattle = i == 3;
        TestCreateLATrainerSetParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, policies[i], &row, 1);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HP_IV), 0);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_HELD_ITEM), ITEM_NONE);
    }
    gIsDebugBattle = FALSE;
}

TEST("Trainer Sets: constructing held items preserves Phase 4 trainer consumables")
{
    struct BattleHistory history = {0};
    struct BattleHistory *previous = gBattleHistory;
    history.trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_HYPER_POTION;
    history.trainerItems[B_TRAINER_OPPONENT_A][1] = ITEM_FULL_HEAL;
    struct BattleHistory before = history;
    gBattleHistory = &history;
    CheckSawyer(0);
    EXPECT_EQ(memcmp(&history, &before, sizeof(history)), 0);
    gBattleHistory = previous;
}

TEST("Trainer Sets: canonical level-up, teachable and egg sources ignore current level")
{
    EXPECT(LASetSpeciesCanLearnMove(SPECIES_NUMEL, MOVE_EARTH_POWER));
    EXPECT(LASetSpeciesCanLearnMove(SPECIES_GEODUDE, MOVE_STEALTH_ROCK));
    EXPECT(LASetSpeciesCanLearnMove(SPECIES_TORKOAL, MOVE_YAWN));
    EXPECT(!LASetSpeciesCanLearnMove(SPECIES_GEODUDE, MOVE_SPORE));
}
