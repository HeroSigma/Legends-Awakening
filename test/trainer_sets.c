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
#include "trainer_scaling.h"
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
    const u8 fastPhysical[] = {0, 252, 0, 0, 4, 252};
    const u8 fastSpecial[] = {0, 0, 0, 252, 4, 252};
    const u8 specialDefense[] = {252, 0, 4, 0, 252, 0};
    const u8 *expected = physical;
    if (set->training->nature == NATURE_MODEST)
        expected = special;
    else if (set->training->nature == NATURE_IMPISH || set->training->nature == NATURE_BOLD)
        expected = utility;
    else if (set->training->nature == NATURE_JOLLY)
        expected = fastPhysical;
    else if (set->training->nature == NATURE_TIMID)
        expected = fastSpecial;
    else if (set->training->nature == NATURE_CALM)
        expected = specialDefense;
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

// Frozen Batch 1 expectations. All lookups below use production assignments.
enum SouthwestSetId
{
    SW_SET_Swellow,
    SW_SET_LinooneField,
    SW_SET_LinooneWinston,
    SW_SET_LinooneCindy,
    SW_SET_Mightyena,
    SW_SET_Breloom,
    SW_SET_Manectric,
    SW_SET_Swalot,
    SW_SET_Grumpig,
    SW_SET_Kecleon,
    SW_SET_Mawile,
    SW_SET_Pelipper,
    SW_SET_Skitty,
    SW_SET_Roselia,
    SW_SET_Swablu,
    SW_SET_Altaria,
    SW_SET_Beautifly,
    SW_SET_Azumarill,
    SW_SET_Lombre,
    SW_SET_Illumise,
    SW_SET_Masquerain,
    SW_SET_NinjaskAce,
    SW_SET_NinjaskCoverage,
    SW_SET_Dustox,
    SW_SET_Volbeat,
    SW_SET_COUNT,
};
struct SouthwestSetExpected
{
    enum Species species;
    u8 evs[6];
    u8 nature;
    u8 slot;
    enum Ability ability;
    enum Move moves[4];
    enum Item item;
};
static const struct SouthwestSetExpected sSouthwestSets[] =
{
    [SW_SET_Swellow] = {SPECIES_SWELLOW, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_GUTS,
        {MOVE_BRAVE_BIRD, MOVE_QUICK_ATTACK, MOVE_FACADE, MOVE_STEEL_WING}, ITEM_SHARP_BEAK},
    [SW_SET_LinooneField] = {SPECIES_LINOONE, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_PICKUP,
        {MOVE_BODY_SLAM, MOVE_THIEF, MOVE_DIG, MOVE_THUNDER_WAVE}, ITEM_SILK_SCARF},
    [SW_SET_LinooneWinston] = {SPECIES_LINOONE, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 1, ABILITY_GLUTTONY,
        {MOVE_BELLY_DRUM, MOVE_DOUBLE_EDGE, MOVE_THIEF, MOVE_ROCK_SMASH}, ITEM_SITRUS_BERRY},
    [SW_SET_LinooneCindy] = {SPECIES_LINOONE, {252, 0, 252, 0, 4, 0}, NATURE_IMPISH, 0, ABILITY_PICKUP,
        {MOVE_BODY_SLAM, MOVE_BABY_DOLL_EYES, MOVE_THUNDER_WAVE, MOVE_REST}, ITEM_LEFTOVERS},
    [SW_SET_Mightyena] = {SPECIES_MIGHTYENA, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 0, ABILITY_INTIMIDATE,
        {MOVE_CRUNCH, MOVE_SUCKER_PUNCH, MOVE_ICE_FANG, MOVE_HOWL}, ITEM_BLACK_GLASSES},
    [SW_SET_Breloom] = {SPECIES_BRELOOM, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 2, ABILITY_TECHNICIAN,
        {MOVE_BULLET_SEED, MOVE_MACH_PUNCH, MOVE_ROCK_TOMB, MOVE_SWORDS_DANCE}, ITEM_BLACK_BELT},
    [SW_SET_Manectric] = {SPECIES_MANECTRIC, {0, 0, 0, 252, 4, 252}, NATURE_TIMID, 1, ABILITY_LIGHTNING_ROD,
        {MOVE_THUNDERBOLT, MOVE_LIGHT_SCREEN, MOVE_FLAMETHROWER, MOVE_THUNDER_WAVE}, ITEM_MAGNET},
    [SW_SET_Swalot] = {SPECIES_SWALOT, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 1, ABILITY_STICKY_HOLD,
        {MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_YAWN, MOVE_ENCORE}, ITEM_SITRUS_BERRY},
    [SW_SET_Grumpig] = {SPECIES_GRUMPIG, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 0, ABILITY_THICK_FAT,
        {MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_THUNDER_WAVE, MOVE_TAUNT}, ITEM_TWISTED_SPOON},
    [SW_SET_Kecleon] = {SPECIES_KECLEON, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 0, ABILITY_COLOR_CHANGE,
        {MOVE_BODY_SLAM, MOVE_SHADOW_SNEAK, MOVE_BRICK_BREAK, MOVE_RECOVER}, ITEM_SITRUS_BERRY},
    [SW_SET_Mawile] = {SPECIES_MAWILE, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 1, ABILITY_INTIMIDATE,
        {MOVE_PLAY_ROUGH, MOVE_IRON_HEAD, MOVE_SUCKER_PUNCH, MOVE_SWORDS_DANCE}, ITEM_METAL_COAT},
    [SW_SET_Pelipper] = {SPECIES_PELIPPER, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 0, ABILITY_KEEN_EYE,
        {MOVE_SURF, MOVE_AIR_SLASH, MOVE_ROOST, MOVE_ICY_WIND}, ITEM_MYSTIC_WATER},
    [SW_SET_Skitty] = {SPECIES_SKITTY, {252, 0, 252, 0, 4, 0}, NATURE_IMPISH, 0, ABILITY_CUTE_CHARM,
        {MOVE_FAKE_OUT, MOVE_BODY_SLAM, MOVE_WISH, MOVE_HEAL_BELL}, ITEM_EVIOLITE},
    [SW_SET_Roselia] = {SPECIES_ROSELIA, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 0, ABILITY_NATURAL_CURE,
        {MOVE_GIGA_DRAIN, MOVE_SLUDGE_BOMB, MOVE_SPIKES, MOVE_SYNTHESIS}, ITEM_EVIOLITE},
    [SW_SET_Swablu] = {SPECIES_SWABLU, {252, 0, 252, 0, 4, 0}, NATURE_IMPISH, 0, ABILITY_NATURAL_CURE,
        {MOVE_BODY_SLAM, MOVE_ROOST, MOVE_SAFEGUARD, MOVE_SING}, ITEM_EVIOLITE},
    [SW_SET_Altaria] = {SPECIES_ALTARIA, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 0, ABILITY_NATURAL_CURE,
        {MOVE_DRAGON_PULSE, MOVE_FLAMETHROWER, MOVE_REST, MOVE_SLEEP_TALK}, ITEM_LEFTOVERS},
    [SW_SET_Beautifly] = {SPECIES_BEAUTIFLY, {0, 0, 0, 252, 4, 252}, NATURE_TIMID, 0, ABILITY_SWARM,
        {MOVE_BUG_BUZZ, MOVE_AIR_CUTTER, MOVE_GIGA_DRAIN, MOVE_QUIVER_DANCE}, ITEM_SITRUS_BERRY},
    [SW_SET_Azumarill] = {SPECIES_AZUMARILL, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 1, ABILITY_HUGE_POWER,
        {MOVE_WATERFALL, MOVE_PLAY_ROUGH, MOVE_BRICK_BREAK, MOVE_ICE_PUNCH}, ITEM_MYSTIC_WATER},
    [SW_SET_Lombre] = {SPECIES_LOMBRE, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 2, ABILITY_OWN_TEMPO,
        {MOVE_SURF, MOVE_GIGA_DRAIN, MOVE_ICE_BEAM, MOVE_FAKE_OUT}, ITEM_EVIOLITE},
    [SW_SET_Illumise] = {SPECIES_ILLUMISE, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 2, ABILITY_PRANKSTER,
        {MOVE_ENCORE, MOVE_THUNDER_WAVE, MOVE_BUG_BUZZ, MOVE_MOONLIGHT}, ITEM_SITRUS_BERRY},
    [SW_SET_Masquerain] = {SPECIES_MASQUERAIN, {0, 0, 0, 252, 4, 252}, NATURE_TIMID, 0, ABILITY_INTIMIDATE,
        {MOVE_BUG_BUZZ, MOVE_AIR_SLASH, MOVE_STUN_SPORE, MOVE_GIGA_DRAIN}, ITEM_SITRUS_BERRY},
    [SW_SET_NinjaskAce] = {SPECIES_NINJASK, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_SPEED_BOOST,
        {MOVE_X_SCISSOR, MOVE_AERIAL_ACE, MOVE_SWORDS_DANCE, MOVE_PROTECT}, ITEM_LUM_BERRY},
    [SW_SET_NinjaskCoverage] = {SPECIES_NINJASK, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_SPEED_BOOST,
        {MOVE_X_SCISSOR, MOVE_AERIAL_ACE, MOVE_DIG, MOVE_SCREECH}, ITEM_SHARP_BEAK},
    [SW_SET_Dustox] = {SPECIES_DUSTOX, {252, 0, 4, 0, 252, 0}, NATURE_CALM, 0, ABILITY_SHIELD_DUST,
        {MOVE_BUG_BUZZ, MOVE_SLUDGE_BOMB, MOVE_MOONLIGHT, MOVE_LIGHT_SCREEN}, ITEM_LEFTOVERS},
    [SW_SET_Volbeat] = {SPECIES_VOLBEAT, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 2, ABILITY_PRANKSTER,
        {MOVE_THUNDER_WAVE, MOVE_ENCORE, MOVE_BUG_BUZZ, MOVE_MOONLIGHT}, ITEM_SITRUS_BERRY},
};
struct SouthwestAssignmentExpected
{
    u16 trainerId;
    u32 key;
    enum Species authored;
    enum SouthwestSetId setId;
};
static const struct SouthwestAssignmentExpected sSouthwestAssignments[] =
{
    {TRAINER_CALVIN_4, 0, SPECIES_SWELLOW, SW_SET_Swellow},
    {TRAINER_CALVIN_4, 1, SPECIES_LINOONE, SW_SET_LinooneField},
    {TRAINER_CALVIN_4, 2, SPECIES_MIGHTYENA, SW_SET_Mightyena},
    {TRAINER_CALVIN_5, 0, SPECIES_SWELLOW, SW_SET_Swellow},
    {TRAINER_CALVIN_5, 1, SPECIES_LINOONE, SW_SET_LinooneField},
    {TRAINER_CALVIN_5, 2, SPECIES_MIGHTYENA, SW_SET_Mightyena},
    {TRAINER_CALVIN_5, 0x80020001u, SPECIES_SHROOMISH, SW_SET_Breloom},
    {TRAINER_CALVIN_5, 0x80020002u, SPECIES_ELECTRIKE, SW_SET_Manectric},
    {TRAINER_CALVIN_5, 0x80020003u, SPECIES_GULPIN, SW_SET_Swalot},
    {TRAINER_WINSTON_4, 0, SPECIES_LINOONE, SW_SET_LinooneWinston},
    {TRAINER_WINSTON_5, 0, SPECIES_LINOONE, SW_SET_LinooneWinston},
    {TRAINER_WINSTON_5, 0x80030001u, SPECIES_ELECTRIKE, SW_SET_Manectric},
    {TRAINER_WINSTON_5, 0x80030002u, SPECIES_SPOINK, SW_SET_Grumpig},
    {TRAINER_WINSTON_5, 0x80030003u, SPECIES_KECLEON, SW_SET_Kecleon},
    {TRAINER_WINSTON_5, 0x80030004u, SPECIES_MAWILE, SW_SET_Mawile},
    {TRAINER_WINSTON_5, 0x80030005u, SPECIES_WINGULL, SW_SET_Pelipper},
    {TRAINER_CINDY_5, 0, SPECIES_LINOONE, SW_SET_LinooneCindy},
    {TRAINER_CINDY_6, 0, SPECIES_LINOONE, SW_SET_LinooneCindy},
    {TRAINER_CINDY_6, 0x80040001u, SPECIES_SKITTY, SW_SET_Skitty},
    {TRAINER_CINDY_6, 0x80040002u, SPECIES_ROSELIA, SW_SET_Roselia},
    {TRAINER_CINDY_6, 0x80040003u, SPECIES_SWABLU, SW_SET_Swablu},
    {TRAINER_CINDY_6, 0x80040004u, SPECIES_BEAUTIFLY, SW_SET_Beautifly},
    {TRAINER_CINDY_6, 0x80040005u, SPECIES_MARILL, SW_SET_Azumarill},
    {TRAINER_HALEY_4, 0, SPECIES_LOMBRE, SW_SET_Lombre},
    {TRAINER_HALEY_4, 1, SPECIES_BRELOOM, SW_SET_Breloom},
    {TRAINER_HALEY_5, 0, SPECIES_SWELLOW, SW_SET_Swellow},
    {TRAINER_HALEY_5, 1, SPECIES_LOMBRE, SW_SET_Lombre},
    {TRAINER_HALEY_5, 2, SPECIES_BRELOOM, SW_SET_Breloom},
    {TRAINER_HALEY_5, 0x80050001u, SPECIES_ROSELIA, SW_SET_Roselia},
    {TRAINER_HALEY_5, 0x80050002u, SPECIES_WINGULL, SW_SET_Pelipper},
    {TRAINER_HALEY_5, 0x80050003u, SPECIES_ILLUMISE, SW_SET_Illumise},
    {TRAINER_JAMES_4, 0, SPECIES_SURSKIT, SW_SET_Masquerain},
    {TRAINER_JAMES_4, 1, SPECIES_DUSTOX, SW_SET_Dustox},
    {TRAINER_JAMES_4, 2, SPECIES_NINJASK, SW_SET_NinjaskAce},
    {TRAINER_JAMES_5, 0, SPECIES_SURSKIT, SW_SET_Masquerain},
    {TRAINER_JAMES_5, 1, SPECIES_NINJASK, SW_SET_NinjaskCoverage},
    {TRAINER_JAMES_5, 2, SPECIES_DUSTOX, SW_SET_Dustox},
    {TRAINER_JAMES_5, 3, SPECIES_NINJASK, SW_SET_NinjaskAce},
    {TRAINER_JAMES_5, 0x80060001u, SPECIES_BEAUTIFLY, SW_SET_Beautifly},
    {TRAINER_JAMES_5, 0x80060002u, SPECIES_VOLBEAT, SW_SET_Volbeat},
};

static void ExpectSouthwestSet(const struct LACompetitiveSet *set, enum SouthwestSetId id)
{
    const struct SouthwestSetExpected *e = &sSouthwestSets[id];
    ASSUME(set != NULL);
    ASSUME(set->training != NULL);
    EXPECT_EQ(set->finalSpecies, e->species);
    EXPECT_EQ(set->training->nature, e->nature);
    EXPECT_EQ(set->abilitySlot, e->slot);
    EXPECT_EQ(gSpeciesInfo[set->finalSpecies].abilities[set->abilitySlot], e->ability);
    EXPECT_EQ(set->heldItem, e->item);
    EXPECT_EQ(memcmp(set->moves, e->moves, sizeof(e->moves)), 0);
    EXPECT_EQ(memcmp(set->training->evs, e->evs, sizeof(e->evs)), 0);
    u32 total = 0;
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT(set->training->evs[i] <= 252);
        total += set->training->evs[i];
    }
    EXPECT_EQ(total, 508);
    for (u32 i = 0; i < 4; i++)
        EXPECT(LASetSpeciesCanLearnMove(set->finalSpecies, set->moves[i]));
}

TEST("Trainer Sets: Southwest 40 exact assignments and 25 complete immutable records")
{
    bool32 seen[SW_SET_COUNT] = {0};
    u32 records = 0;
    rng_value_t r1 = gRngValue, r2 = gRng2Value;
    for (u32 i = 0; i < ARRAY_COUNT(sSouthwestAssignments); i++)
    {
        const struct SouthwestAssignmentExpected *e = &sSouthwestAssignments[i];
        const struct LASetBundle *bundle = GetLASetBundle(e->trainerId, DIFFICULTY_NORMAL, e->key, e->authored);
        ASSUME(bundle != NULL);
        EXPECT_EQ(bundle->count, e->setId == SW_SET_Swablu ? 2 : 1);
        EXPECT(GetLASetBundle(e->trainerId, DIFFICULTY_EASY, e->key, e->authored) == NULL);
        EXPECT(GetLASetBundle(e->trainerId, DIFFICULTY_HARD, e->key, e->authored) == NULL);
        EXPECT(GetLASetBundle(e->trainerId, DIFFICULTY_NORMAL, e->key, SPECIES_MAGIKARP) == NULL);
        for (u32 j = 0; j < bundle->count; j++)
        {
            enum SouthwestSetId setId = j == 0 ? e->setId : SW_SET_Altaria;
            const struct LACompetitiveSet *set = bundle->variants[j];
            ExpectSouthwestSet(set, setId);
            struct LACompetitiveSet before = *set;
            struct LASetTraining trainingBefore = *set->training;
            struct TrainerMon mon = MON(set->finalSpecies, 50);
            EXPECT(ApplyLACompetitiveSet(&mon, bundle));
            EXPECT_EQ(mon.iv, LA_SET_PERFECT_IVS);
            EXPECT_EQ(mon.ability, sSouthwestSets[setId].ability);
            EXPECT_EQ(mon.species, set->finalSpecies);
            EXPECT_EQ(mon.lvl, 50);
            EXPECT_EQ(memcmp(set, &before, sizeof(before)), 0);
            EXPECT_EQ(memcmp(set->training, &trainingBefore, sizeof(trainingBefore)), 0);
            if (!seen[setId])
            {
                seen[setId] = TRUE;
                records++;
            }
        }
    }
    EXPECT_EQ(ARRAY_COUNT(sSouthwestAssignments), 40);
    EXPECT_EQ(records, 25);
    EXPECT_EQ(memcmp(&r1, &gRngValue, sizeof(r1)), 0);
    EXPECT_EQ(memcmp(&r2, &gRng2Value, sizeof(r2)), 0);
}

// The test ROM replaces gTrainers. These retain the real authored values,
// including Nugget and the final Linoone custom moves, before the production pipeline.
#define SW_MON(s, level, ivs, item) { .species = SPECIES_##s, .lvl = level, \
    .iv = TRAINER_PARTY_IVS(ivs, ivs, ivs, ivs, ivs, ivs), .heldItem = ITEM_##item, \
    .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT, .nature = NATURE_HARDY, .dynamaxLevel = MAX_DYNAMAX_LEVEL }
#define SW_FINAL_LINOONE(ivs) { .species = SPECIES_LINOONE, .lvl = 36, \
    .iv = TRAINER_PARTY_IVS(ivs, ivs, ivs, ivs, ivs, ivs), .heldItem = ITEM_NUGGET, \
    .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT, .nature = NATURE_HARDY, .dynamaxLevel = MAX_DYNAMAX_LEVEL, \
    .moves = {MOVE_FURY_SWIPES, MOVE_MUD_SPORT, MOVE_ODOR_SLEUTH, MOVE_SAND_ATTACK} }
struct SouthwestEncounter
{
    u16 id;
    u8 sourceCount;
    u8 selectedCount;
    u8 anchor;
    struct TrainerMon sources[4];
    u32 keys[6];
    enum Species species[6];
    u8 levels[6];
};
static const struct SouthwestEncounter sSouthwestEncounters[] =
{
    {TRAINER_CALVIN_4, 3, 3, 33,
     {SW_MON(SWELLOW, 31, 3, NONE), SW_MON(LINOONE, 29, 3, NONE), SW_MON(MIGHTYENA, 33, 3, NONE)},
     {0, 1, 2},
     {SPECIES_SWELLOW, SPECIES_LINOONE, SPECIES_MIGHTYENA},
     {31, 29, 33}},
    {TRAINER_CALVIN_5, 3, 6, 36,
     {SW_MON(SWELLOW, 34, 4, NONE), SW_MON(LINOONE, 32, 4, NONE), SW_MON(MIGHTYENA, 36, 4, NONE)},
     {0, 1, 2, 0x80020001u, 0x80020002u, 0x80020003u},
     {SPECIES_SWELLOW, SPECIES_LINOONE, SPECIES_MIGHTYENA, SPECIES_BRELOOM, SPECIES_MANECTRIC, SPECIES_SWALOT},
     {34, 32, 36, 32, 32, 32}},
    {TRAINER_WINSTON_4, 1, 1, 33,
     {SW_MON(LINOONE, 33, 0, NUGGET)},
     {0},
     {SPECIES_LINOONE},
     {33}},
    {TRAINER_WINSTON_5, 1, 6, 36,
     {SW_FINAL_LINOONE(0)},
     {0, 0x80030001u, 0x80030002u, 0x80030003u, 0x80030004u, 0x80030005u},
     {SPECIES_LINOONE, SPECIES_MANECTRIC, SPECIES_GRUMPIG, SPECIES_KECLEON, SPECIES_MAWILE, SPECIES_PELIPPER},
     {36, 34, 34, 34, 34, 34}},
    {TRAINER_CINDY_5, 1, 1, 33,
     {SW_MON(LINOONE, 33, 3, NUGGET)},
     {0},
     {SPECIES_LINOONE},
     {33}},
    {TRAINER_CINDY_6, 1, 6, 36,
     {SW_FINAL_LINOONE(4)},
     {0, 0x80040001u, 0x80040002u, 0x80040003u, 0x80040004u, 0x80040005u},
     {SPECIES_LINOONE, SPECIES_SKITTY, SPECIES_ROSELIA, SPECIES_SWABLU, SPECIES_BEAUTIFLY, SPECIES_AZUMARILL},
     {36, 34, 34, 34, 34, 34}},
    {TRAINER_HALEY_4, 2, 2, 32,
     {SW_MON(LOMBRE, 32, 3, NONE), SW_MON(BRELOOM, 32, 3, NONE)},
     {0, 1},
     {SPECIES_LOMBRE, SPECIES_BRELOOM},
     {32, 32}},
    {TRAINER_HALEY_5, 3, 6, 34,
     {SW_MON(SWELLOW, 34, 4, NONE), SW_MON(LOMBRE, 34, 4, NONE), SW_MON(BRELOOM, 34, 4, NONE)},
     {0, 1, 2, 0x80050001u, 0x80050002u, 0x80050003u},
     {SPECIES_SWELLOW, SPECIES_LOMBRE, SPECIES_BRELOOM, SPECIES_ROSELIA, SPECIES_PELIPPER, SPECIES_ILLUMISE},
     {34, 34, 34, 34, 34, 34}},
    {TRAINER_JAMES_4, 3, 3, 31,
     {SW_MON(SURSKIT, 31, 3, NONE), SW_MON(DUSTOX, 31, 3, NONE), SW_MON(NINJASK, 31, 3, NONE)},
     {0, 1, 2},
     {SPECIES_MASQUERAIN, SPECIES_DUSTOX, SPECIES_NINJASK},
     {31, 31, 31}},
    {TRAINER_JAMES_5, 4, 6, 33,
     {SW_MON(SURSKIT, 33, 4, NONE), SW_MON(NINJASK, 33, 4, NONE), SW_MON(DUSTOX, 33, 4, NONE), SW_MON(NINJASK, 33, 4, NONE)},
     {0, 1, 2, 3, 0x80060001u, 0x80060002u},
     {SPECIES_MASQUERAIN, SPECIES_NINJASK, SPECIES_DUSTOX, SPECIES_NINJASK, SPECIES_BEAUTIFLY, SPECIES_VOLBEAT},
     {33, 33, 33, 33, 33, 33}},
};
#undef SW_MON
#undef SW_FINAL_LINOONE

extern void (*gTestLARosterSourceObserver)(u32);
static u32 sSouthwestObservedKeys[6];
static u32 sSouthwestObservedCount;
static void ObserveSouthwestKey(u32 key)
{
    if (sSouthwestObservedCount < ARRAY_COUNT(sSouthwestObservedKeys))
        sSouthwestObservedKeys[sSouthwestObservedCount] = key;
    sSouthwestObservedCount++;
}

static void CheckSouthwestEncounter(u32 index, bool32 deltaOne)
{
    Setup();
    const struct SouthwestEncounter *c = &sSouthwestEncounters[index];
    if (deltaOne)
    {
        // Existing Phase 2 cell: ACE / MASTER = 37, empty player party.
        SetTrainerRank(TRAINER_RANK_ACE);
        SetWorldPhase(WORLD_PHASE_MASTER);
    }
    const struct Trainer trainer = {.party = c->sources, .partySize = c->sourceCount};
    const struct LARosterProfile *profile = GetLARosterProfile(c->id, DIFFICULTY_NORMAL);
    EXPECT_EQ(profile != NULL, c->selectedCount == 6);
    struct LARosterSelectedMon selected[6] = {0};
    if (profile != NULL)
    {
        struct LARosterSelection result = SelectLARoster(&trainer, profile, 6);
        ASSUME(result.count == 6);
        memcpy(selected, result.members, sizeof(selected));
    }
    else
        for (u32 i = 0; i < c->sourceCount; i++)
            selected[i] = (struct LARosterSelectedMon){&c->sources[i], i};
    struct TrainerMon sourcesBefore[6];
    u32 anchor = 0;
    for (u32 i = 0; i < c->selectedCount; i++)
    {
        sourcesBefore[i] = *selected[i].source;
        if (sourcesBefore[i].lvl > anchor)
            anchor = sourcesBefore[i].lvl;
    }
    EXPECT_EQ(anchor, c->anchor);
    u8 world = CalculateTrainerScalingWorldLevel(GetTrainerRank(), GetWorldPhase(), 0, 0);
    u8 delta = CalculateTrainerLevelDelta(anchor, world, 0);
    EXPECT_EQ(delta, deltaOne ? 1 : 0);
    struct BattleHistory history = {0};
    history.trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_FULL_RESTORE;
    history.trainerItems[B_TRAINER_OPPONENT_A][1] = ITEM_HYPER_POTION;
    struct BattleHistory before = history;
    struct BattleHistory *oldHistory = gBattleHistory;
    gBattleHistory = &history;
    void (*oldObserver)(u32) = gTestLARosterSourceObserver;
    sSouthwestObservedCount = 0;
    gTestLARosterSourceObserver = ObserveSouthwestKey;
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer, c->id, GetLATrainerPolicy(0));
    gTestLARosterSourceObserver = oldObserver;
    gBattleHistory = oldHistory;
    EXPECT_EQ(memcmp(&before, &history, sizeof(history)), 0);
    EXPECT_EQ(sSouthwestObservedCount, c->selectedCount);
    for (u32 i = 0; i < c->selectedCount; i++)
    {
        EXPECT_EQ(sSouthwestObservedKeys[i], c->keys[i]);
        EXPECT_EQ(selected[i].sourceKey, c->keys[i]);
        EXPECT_EQ(memcmp(selected[i].source, &sourcesBefore[i], sizeof(sourcesBefore[i])), 0);
        enum Species species = c->species[i];
        if (deltaOne && species == SPECIES_SWABLU)
            species = SPECIES_ALTARIA;
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), species);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), c->levels[i] + delta);
        const struct LASetBundle *bundle = GetLASetBundle(c->id, DIFFICULTY_NORMAL, c->keys[i], selected[i].source->species);
        ASSUME(bundle != NULL);
        const struct LACompetitiveSet *set = NULL;
        for (u32 j = 0; j < bundle->count; j++)
            if (bundle->variants[j]->finalSpecies == species)
                set = bundle->variants[j];
        ASSUME(set != NULL);
        CheckGenerated(mon, set);
        if (selected[i].source->heldItem == ITEM_NUGGET)
            EXPECT(GetMonData(mon, MON_DATA_HELD_ITEM) != ITEM_NUGGET);
    }
    for (u32 i = c->selectedCount; i < 6; i++)
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES), SPECIES_NONE);
    Setup();
}

TEST("Trainer Sets: Southwest CALVIN_4 exact production construction") { CheckSouthwestEncounter(0, FALSE); }

TEST("Trainer Sets: Southwest CALVIN_5 exact production construction") { CheckSouthwestEncounter(1, FALSE); }

TEST("Trainer Sets: Southwest WINSTON_4 exact production construction") { CheckSouthwestEncounter(2, FALSE); }

TEST("Trainer Sets: Southwest WINSTON_5 exact production construction") { CheckSouthwestEncounter(3, FALSE); }

TEST("Trainer Sets: Southwest CINDY_5 exact production construction") { CheckSouthwestEncounter(4, FALSE); }

TEST("Trainer Sets: Southwest CINDY_6 exact production construction") { CheckSouthwestEncounter(5, FALSE); }

TEST("Trainer Sets: Southwest HALEY_4 exact production construction") { CheckSouthwestEncounter(6, FALSE); }

TEST("Trainer Sets: Southwest HALEY_5 exact production construction") { CheckSouthwestEncounter(7, FALSE); }

TEST("Trainer Sets: Southwest JAMES_4 exact production construction") { CheckSouthwestEncounter(8, FALSE); }

TEST("Trainer Sets: Southwest JAMES_5 exact production construction") { CheckSouthwestEncounter(9, FALSE); }

TEST("Trainer Sets: Southwest Cindy real Phase 2 delta evolves bird at 35")
{
    CheckSouthwestEncounter(5, TRUE);
}

TEST("Trainer Sets: Southwest exactly twelve explicit held items are permitted")
{
    const enum Item allowed[] = {ITEM_LEFTOVERS, ITEM_EVIOLITE, ITEM_SITRUS_BERRY,
        ITEM_LUM_BERRY, ITEM_SILK_SCARF, ITEM_BLACK_BELT, ITEM_SHARP_BEAK,
        ITEM_MYSTIC_WATER, ITEM_MAGNET, ITEM_BLACK_GLASSES, ITEM_TWISTED_SPOON, ITEM_METAL_COAT};
    struct LACompetitiveSet set = *GeodudeBundle()->variants[0];
    const struct LACompetitiveSet *variants[] = {&set};
    const struct LASetBundle bundle = {variants, 1};
    // Enumerate every item ID: catches accidental broadening by effect/category.
    for (u32 item = 0; item < ITEMS_COUNT; item++)
    {
        bool32 expected = FALSE;
        for (u32 i = 0; i < ARRAY_COUNT(allowed); i++)
            if (item == allowed[i])
                expected = TRUE;
        set.heldItem = item;
        struct TrainerMon mon = MON(SPECIES_GEODUDE, 21);
        struct TrainerMon before = mon;
        EXPECT_EQ(ApplyLACompetitiveSet(&mon, &bundle), expected);
        if (!expected)
            EXPECT_EQ(memcmp(&mon, &before, sizeof(mon)), 0);
        else
            EXPECT_EQ(mon.heldItem, item);
    }
    set.heldItem = ITEMS_COUNT;
    ExpectAtomicFailure(&set, SPECIES_GEODUDE);
}

struct SouthwestInactive
{
    u16 id;
    u8 count;
    struct TrainerMon sources[4];
};
#define SW_INACTIVE(s, l) MON(SPECIES_##s, l)
static const struct SouthwestInactive sSouthwestInactive[] =
{
    {TRAINER_CALVIN_1, 1, {SW_INACTIVE(POOCHYENA, 5)}},
    {TRAINER_CALVIN_2, 1, {SW_INACTIVE(MIGHTYENA, 27)}},
    {TRAINER_CALVIN_3, 2, {SW_INACTIVE(SWELLOW, 28), SW_INACTIVE(MIGHTYENA, 30)}},
    {TRAINER_WINSTON_1, 1, {SW_INACTIVE(ZIGZAGOON, 7)}},
    {TRAINER_WINSTON_2, 1, {SW_INACTIVE(LINOONE, 27)}},
    {TRAINER_WINSTON_3, 1, {SW_INACTIVE(LINOONE, 30)}},
    {TRAINER_CINDY_1, 1, {SW_INACTIVE(ZIGZAGOON, 7)}},
    {TRAINER_CINDY_2, 1, {SW_INACTIVE(ZIGZAGOON, 11)}},
    {TRAINER_CINDY_3, 1, {SW_INACTIVE(LINOONE, 27)}},
    {TRAINER_CINDY_4, 1, {SW_INACTIVE(LINOONE, 30)}},
    {TRAINER_HALEY_1, 2, {SW_INACTIVE(LOTAD, 6), SW_INACTIVE(SHROOMISH, 6)}},
    {TRAINER_HALEY_2, 2, {SW_INACTIVE(LOMBRE, 26), SW_INACTIVE(SHROOMISH, 26)}},
    {TRAINER_HALEY_3, 2, {SW_INACTIVE(LOMBRE, 29), SW_INACTIVE(BRELOOM, 29)}},
    {TRAINER_JAMES_1, 2, {SW_INACTIVE(NINCADA, 6), SW_INACTIVE(NINCADA, 6)}},
    {TRAINER_JAMES_2, 1, {SW_INACTIVE(NINJASK, 27)}},
    {TRAINER_JAMES_3, 2, {SW_INACTIVE(DUSTOX, 29), SW_INACTIVE(NINJASK, 29)}},
    {TRAINER_RICK, 2, {SW_INACTIVE(WURMPLE, 4), SW_INACTIVE(WURMPLE, 4)}},
    {TRAINER_TIANA, 2, {SW_INACTIVE(ZIGZAGOON, 4), SW_INACTIVE(SHROOMISH, 4)}},
    {TRAINER_ALLEN, 2, {SW_INACTIVE(ZIGZAGOON, 4), SW_INACTIVE(TAILLOW, 3)}},
    {TRAINER_BILLY, 2, {SW_INACTIVE(ZIGZAGOON, 5), SW_INACTIVE(SEEDOT, 7)}},
    {TRAINER_DARIAN, 1, {SW_INACTIVE(MAGIKARP, 9)}},
    {TRAINER_IVAN, 3, {SW_INACTIVE(MAGIKARP, 5), SW_INACTIVE(MAGIKARP, 6), SW_INACTIVE(MAGIKARP, 7)}},
    {TRAINER_LYLE, 4, {SW_INACTIVE(WURMPLE, 3), SW_INACTIVE(WURMPLE, 3), SW_INACTIVE(WURMPLE, 3), SW_INACTIVE(WURMPLE, 3)}},
};
#undef SW_INACTIVE

TEST("Trainer Sets: Southwest earlier stages Cindy 2 and seven controls remain unassigned")
{
    for (u32 i = 0; i < ARRAY_COUNT(sSouthwestInactive); i++)
    {
        Setup();
        const struct SouthwestInactive *c = &sSouthwestInactive[i];
        EXPECT(GetLARosterProfile(c->id, DIFFICULTY_NORMAL) == NULL);
        struct TrainerMon sources[4];
        memcpy(sources, c->sources, sizeof(sources));
        // All earlier Winston/Cindy encounters retain their authored Nugget.
        bool32 nugget = i >= 3 && i <= 9;
        if (nugget)
            sources[0].heldItem = ITEM_NUGGET;
        if (c->id == TRAINER_CINDY_2)
        {
            sources[0].moves[0] = MOVE_TACKLE;
            sources[0].moves[1] = MOVE_TAIL_WHIP;
        }
        for (u32 j = 0; j < c->count; j++)
            EXPECT(GetLASetBundle(c->id, DIFFICULTY_NORMAL, j, sources[j].species) == NULL);
        struct TrainerMon before[4];
        memcpy(before, sources, sizeof(before));
        const struct Trainer trainer = {.party = sources, .partySize = c->count};
        TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer, c->id, GetLATrainerPolicy(0));
        EXPECT_EQ(memcmp(before, sources, sizeof(before)), 0);
        for (u32 j = 0; j < c->count; j++)
        {
            struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][j];
            EXPECT_EQ(GetMonData(mon, MON_DATA_HP_IV), 0);
            EXPECT_EQ(GetMonData(mon, MON_DATA_HP_EV), 0);
            EXPECT_EQ(GetMonData(mon, MON_DATA_HELD_ITEM), sources[j].heldItem);
        }
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][c->count], MON_DATA_SPECIES), SPECIES_NONE);
        if (c->id == TRAINER_CINDY_2)
        {
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MOVE1), MOVE_TACKLE);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MOVE2), MOVE_TAIL_WHIP);
            EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_MOVE3), MOVE_NONE);
        }
    }
}

TEST("Trainer Sets: Southwest James duplicate Ninjask retain distinct intent")
{
    const struct LASetBundle *coverage = GetLASetBundle(TRAINER_JAMES_5, DIFFICULTY_NORMAL, 1, SPECIES_NINJASK);
    const struct LASetBundle *ace = GetLASetBundle(TRAINER_JAMES_5, DIFFICULTY_NORMAL, 3, SPECIES_NINJASK);
    ASSUME(coverage != NULL && ace != NULL);
    EXPECT(coverage != ace);
    ExpectSouthwestSet(coverage->variants[0], SW_SET_NinjaskCoverage);
    ExpectSouthwestSet(ace->variants[0], SW_SET_NinjaskAce);
    EXPECT(GetLASetBundle(TRAINER_JAMES_4, DIFFICULTY_NORMAL, 2, SPECIES_NINJASK) == ace);
    EXPECT(GetLASetBundle(TRAINER_JAMES_5, DIFFICULTY_NORMAL, 2, SPECIES_NINJASK) == NULL);
}

TEST("Trainer Sets: Southwest no manufactured stone evolutions even at level 100")
{
    const struct SouthwestAssignmentExpected *rows[] = {
        &sSouthwestAssignments[18], // Cindy Skitty
        &sSouthwestAssignments[19], // Cindy Roselia
        &sSouthwestAssignments[23], // Haley Lombre
    };
    for (u32 i = 0; i < ARRAY_COUNT(rows); i++)
    {
        const struct SouthwestAssignmentExpected *row = rows[i];
        EXPECT(row->authored == SPECIES_SKITTY || row->authored == SPECIES_ROSELIA || row->authored == SPECIES_LOMBRE);
        struct TrainerMon mon = MON(row->authored, 100);
        ApplyTrainerEvolution(&mon, NULL);
        EXPECT_EQ(mon.species, row->authored);
        const struct LASetBundle *bundle = GetLASetBundle(row->trainerId, DIFFICULTY_NORMAL, row->key, row->authored);
        EXPECT(ApplyLACompetitiveSet(&mon, bundle));
        EXPECT_EQ(mon.species, row->authored);
        EXPECT_EQ(mon.heldItem, ITEM_EVIOLITE);
    }
}

TEST("Trainer Sets: Southwest shared species reuse explicit bundle pointers")
{
    EXPECT(GetLASetBundle(TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0, SPECIES_SWELLOW)
        == GetLASetBundle(TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0, SPECIES_SWELLOW));
    EXPECT(GetLASetBundle(TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0x80020002u, SPECIES_ELECTRIKE)
        == GetLASetBundle(TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030001u, SPECIES_ELECTRIKE));
    EXPECT(GetLASetBundle(TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030005u, SPECIES_WINGULL)
        == GetLASetBundle(TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0x80050002u, SPECIES_WINGULL));
}

// A test-local copy exposes the ROM-const assignment census without adding a
// runtime API. All behavioral assertions below call production lookup/construction.
#include "../src/data/trainer_sets.h"

static const struct SouthwestSetExpected sBatch2Sets[] =
{
    {SPECIES_GYARADOS, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_INTIMIDATE,
        {MOVE_WATERFALL, MOVE_EARTHQUAKE, MOVE_ICE_FANG, MOVE_DRAGON_DANCE}, ITEM_LUM_BERRY},
    {SPECIES_CARVANHA, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 2, ABILITY_SPEED_BOOST,
        {MOVE_LIQUIDATION, MOVE_CRUNCH, MOVE_ICE_FANG, MOVE_PROTECT}, ITEM_MYSTIC_WATER},
    {SPECIES_SHARPEDO, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 2, ABILITY_SPEED_BOOST,
        {MOVE_LIQUIDATION, MOVE_CRUNCH, MOVE_ICE_FANG, MOVE_PROTECT}, ITEM_MYSTIC_WATER},
    {SPECIES_TENTACOOL, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 1, ABILITY_LIQUID_OOZE,
        {MOVE_SURF, MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_HAZE}, ITEM_EVIOLITE},
    {SPECIES_TENTACRUEL, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 1, ABILITY_LIQUID_OOZE,
        {MOVE_SURF, MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_TOXIC}, ITEM_LEFTOVERS},
    {SPECIES_LOUDRED, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 2, ABILITY_SCRAPPY,
        {MOVE_HYPER_VOICE, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_SHADOW_BALL}, ITEM_EVIOLITE},
    {SPECIES_EXPLOUD, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 2, ABILITY_SCRAPPY,
        {MOVE_BOOMBURST, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_SURF}, ITEM_SILK_SCARF},
    {SPECIES_KIRLIA, {252, 0, 0, 252, 4, 0}, NATURE_MODEST, 1, ABILITY_TRACE,
        {MOVE_PSYCHIC, MOVE_DRAINING_KISS, MOVE_CALM_MIND, MOVE_THUNDERBOLT}, ITEM_EVIOLITE},
    {SPECIES_BANETTE, {252, 252, 0, 0, 4, 0}, NATURE_ADAMANT, 0, ABILITY_INSOMNIA,
        {MOVE_KNOCK_OFF, MOVE_SHADOW_SNEAK, MOVE_WILL_O_WISP, MOVE_SUCKER_PUNCH}, ITEM_SITRUS_BERRY},
    {SPECIES_MEDICHAM, {0, 252, 0, 0, 4, 252}, NATURE_JOLLY, 0, ABILITY_PURE_POWER,
        {MOVE_HIGH_JUMP_KICK, MOVE_ZEN_HEADBUTT, MOVE_ICE_PUNCH, MOVE_THUNDER_PUNCH}, ITEM_BLACK_BELT},
    {SPECIES_CHIMECHO, {252, 0, 252, 0, 4, 0}, NATURE_BOLD, 0, ABILITY_LEVITATE,
        {MOVE_PSYCHIC, MOVE_RECOVER, MOVE_HEAL_BELL, MOVE_THUNDER_WAVE}, ITEM_LEFTOVERS},
    {SPECIES_SABLEYE, {252, 0, 252, 0, 4, 0}, NATURE_IMPISH, 2, ABILITY_PRANKSTER,
        {MOVE_KNOCK_OFF, MOVE_RECOVER, MOVE_TAUNT, MOVE_THUNDER_WAVE}, ITEM_LEFTOVERS},
};


struct Batch2Encounter
{
    u16 id;
    u8 sourceCount;
    u8 anchor;
    struct TrainerMon sources[4];
    u32 keys[6];
    enum Species guards[6];
    enum Species species[6];
    u8 levels[6];
    const struct LASetBundle *bundles[6];
};
#define B2_MON(s, level, ivs) { .species = SPECIES_##s, .lvl = level, \
    .iv = TRAINER_PARTY_IVS(ivs, ivs, ivs, ivs, ivs, ivs), \
    .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT, \
    .nature = NATURE_HARDY, .dynamaxLevel = MAX_DYNAMAX_LEVEL }
static const struct Batch2Encounter sBatch2Encounters[] =
{
    {TRAINER_ELLIOT_3, 4, 29,
     {B2_MON(GYARADOS, 29, 2), B2_MON(CARVANHA, 26, 2), B2_MON(TENTACOOL, 26, 2), B2_MON(GYARADOS, 29, 2)},
     {0, 1, 2, 3, 0x80070001u, 0x80070002u},
     {SPECIES_GYARADOS, SPECIES_CARVANHA, SPECIES_TENTACOOL, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {SPECIES_GYARADOS, SPECIES_CARVANHA, SPECIES_TENTACOOL, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {29, 26, 26, 29, 29, 29},
     {&sGyaradosBundle, &sCarvanhaLineBundle, &sTentacoolLineBundle, &sGyaradosBundle, &sTentacruelBundle, &sSharpedoBundle}},
    {TRAINER_ELLIOT_4, 4, 31,
     {B2_MON(GYARADOS, 31, 3), B2_MON(CARVANHA, 30, 3), B2_MON(TENTACRUEL, 30, 3), B2_MON(GYARADOS, 31, 3)},
     {0, 1, 2, 3, 0x80070001u, 0x80070002u},
     {SPECIES_GYARADOS, SPECIES_CARVANHA, SPECIES_TENTACRUEL, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {SPECIES_GYARADOS, SPECIES_SHARPEDO, SPECIES_TENTACRUEL, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {31, 30, 30, 31, 31, 31},
     {&sGyaradosBundle, &sCarvanhaLineBundle, &sTentacruelBundle, &sGyaradosBundle, &sTentacruelBundle, &sSharpedoBundle}},
    {TRAINER_ELLIOT_5, 4, 35,
     {B2_MON(GYARADOS, 33, 4), B2_MON(SHARPEDO, 33, 4), B2_MON(GYARADOS, 33, 4), B2_MON(TENTACRUEL, 35, 4)},
     {0, 1, 2, 3, 0x80070001u, 0x80070002u},
     {SPECIES_GYARADOS, SPECIES_SHARPEDO, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {SPECIES_GYARADOS, SPECIES_SHARPEDO, SPECIES_GYARADOS, SPECIES_TENTACRUEL, SPECIES_TENTACRUEL, SPECIES_SHARPEDO},
     {33, 33, 33, 35, 33, 33},
     {&sGyaradosBundle, &sSharpedoBundle, &sGyaradosBundle, &sTentacruelBundle, &sTentacruelBundle, &sSharpedoBundle}},
    {TRAINER_KAREN_4, 2, 32,
     {B2_MON(BRELOOM, 32, 3), B2_MON(LOUDRED, 32, 3)},
     {0, 1, 0x80080001u, 0x80080002u, 0x80080003u, 0x80080004u},
     {SPECIES_BRELOOM, SPECIES_LOUDRED, SPECIES_BEAUTIFLY, SPECIES_SURSKIT, SPECIES_ROSELIA, SPECIES_NINJASK},
     {SPECIES_BRELOOM, SPECIES_LOUDRED, SPECIES_BEAUTIFLY, SPECIES_MASQUERAIN, SPECIES_ROSELIA, SPECIES_NINJASK},
     {32, 32, 32, 32, 32, 32},
     {&sBreloomBundle, &sLoudredLineBundle, &sBeautiflyBundle, &sMasquerainBundle, &sRoseliaBundle, &sNinjaskAceBundle}},
    {TRAINER_KAREN_5, 2, 35,
     {B2_MON(BRELOOM, 35, 4), B2_MON(EXPLOUD, 35, 4)},
     {0, 1, 0x80080001u, 0x80080002u, 0x80080003u, 0x80080004u},
     {SPECIES_BRELOOM, SPECIES_EXPLOUD, SPECIES_BEAUTIFLY, SPECIES_SURSKIT, SPECIES_ROSELIA, SPECIES_NINJASK},
     {SPECIES_BRELOOM, SPECIES_EXPLOUD, SPECIES_BEAUTIFLY, SPECIES_MASQUERAIN, SPECIES_ROSELIA, SPECIES_NINJASK},
     {35, 35, 35, 35, 35, 35},
     {&sBreloomBundle, &sLoudredLineBundle, &sBeautiflyBundle, &sMasquerainBundle, &sRoseliaBundle, &sNinjaskAceBundle}},
    {TRAINER_JERRY_4, 2, 32,
     {B2_MON(KIRLIA, 32, 3), B2_MON(MEDICHAM, 32, 3)},
     {0, 1, 0x80090001u, 0x80090002u, 0x80090003u, 0x80090004u},
     {SPECIES_KIRLIA, SPECIES_MEDICHAM, SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE, SPECIES_BANETTE},
     {SPECIES_KIRLIA, SPECIES_MEDICHAM, SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE, SPECIES_BANETTE},
     {32, 32, 32, 32, 32, 32},
     {&sKirliaBundle, &sMedichamBundle, &sGrumpigBundle, &sChimechoBundle, &sSableyeBundle, &sBanetteBundle}},
    {TRAINER_JERRY_5, 3, 34,
     {B2_MON(KIRLIA, 34, 4), B2_MON(BANETTE, 34, 4), B2_MON(MEDICHAM, 34, 4)},
     {0, 1, 2, 0x80090001u, 0x80090002u, 0x80090003u},
     {SPECIES_KIRLIA, SPECIES_BANETTE, SPECIES_MEDICHAM, SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE},
     {SPECIES_KIRLIA, SPECIES_BANETTE, SPECIES_MEDICHAM, SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE},
     {34, 34, 34, 34, 34, 34},
     {&sKirliaBundle, &sBanetteBundle, &sMedichamBundle, &sGrumpigBundle, &sChimechoBundle, &sSableyeBundle}},
};
#undef B2_MON

static void ExpectBatch2Set(const struct LACompetitiveSet *set, u32 id)
{
    const struct SouthwestSetExpected *e = &sBatch2Sets[id];
    ASSUME(set != NULL);
    ASSUME(set->training != NULL);
    EXPECT_EQ(set->finalSpecies, e->species);
    EXPECT_EQ(set->training->nature, e->nature);
    EXPECT_EQ(set->abilitySlot, e->slot);
    EXPECT_EQ(gSpeciesInfo[set->finalSpecies].abilities[set->abilitySlot], e->ability);
    EXPECT_EQ(set->heldItem, e->item);
    EXPECT_EQ(memcmp(set->moves, e->moves, sizeof(e->moves)), 0);
    EXPECT_EQ(memcmp(set->training->evs, e->evs, sizeof(e->evs)), 0);
    u32 total = 0;
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT(set->training->evs[i] <= 252);
        total += set->training->evs[i];
    }
    EXPECT_EQ(total, 508);
    for (u32 i = 0; i < 4; i++)
        EXPECT(LASetSpeciesCanLearnMove(set->finalSpecies, set->moves[i]));
}


static void ExpectBatch2Record(const struct LACompetitiveSet *set)
{
    for (u32 i = 0; i < ARRAY_COUNT(sBatch2Sets); i++)
        if (sBatch2Sets[i].species == set->finalSpecies)
        {
            ExpectBatch2Set(set, i);
            return;
        }
    const enum SouthwestSetId reused[] = {SW_SET_Breloom, SW_SET_Beautifly,
        SW_SET_Masquerain, SW_SET_Roselia, SW_SET_NinjaskAce, SW_SET_Grumpig};
    for (u32 i = 0; i < ARRAY_COUNT(reused); i++)
        if (sSouthwestSets[reused[i]].species == set->finalSpecies)
        {
            ExpectSouthwestSet(set, reused[i]);
            return;
        }
    EXPECT(FALSE);
}

TEST("Trainer Sets: Batch 2 42 exact assignments twelve new records and complete variants")
{
    bool32 seen[ARRAY_COUNT(sBatch2Sets)] = {0};
    u32 assignments = 0;
    rng_value_t r1 = gRngValue, r2 = gRng2Value;
    for (u32 i = 0; i < ARRAY_COUNT(sBatch2Encounters); i++)
    {
        const struct Batch2Encounter *c = &sBatch2Encounters[i];
        for (u32 j = 0; j < PARTY_SIZE; j++)
        {
            const struct LASetBundle *bundle = GetLASetBundle(c->id, DIFFICULTY_NORMAL, c->keys[j], c->guards[j]);
            ASSUME(bundle != NULL);
            EXPECT_EQ(bundle->count, c->bundles[j]->count);
            EXPECT(GetLASetBundle(c->id, DIFFICULTY_EASY, c->keys[j], c->guards[j]) == NULL);
            EXPECT(GetLASetBundle(c->id, DIFFICULTY_HARD, c->keys[j], c->guards[j]) == NULL);
            EXPECT(GetLASetBundle(c->id, DIFFICULTY_NORMAL, c->keys[j], SPECIES_MAGIKARP) == NULL);
            for (u32 k = 0; k < bundle->count; k++)
            {
                const struct LACompetitiveSet *set = bundle->variants[k];
                EXPECT_EQ(set->finalSpecies, c->bundles[j]->variants[k]->finalSpecies);
                ExpectBatch2Record(set);
                struct LACompetitiveSet before = *set;
                struct LASetTraining trainingBefore = *set->training;
                struct TrainerMon mon = MON(set->finalSpecies, 50);
                struct TrainerMon original = mon;
                EXPECT(ApplyLACompetitiveSet(&mon, bundle));
                EXPECT_EQ(mon.iv, LA_SET_PERFECT_IVS);
                EXPECT_EQ(mon.ability, gSpeciesInfo[set->finalSpecies].abilities[set->abilitySlot]);
                EXPECT_EQ(mon.heldItem, set->heldItem);
                EXPECT_EQ((u32)mon.nature, set->training->nature);
                EXPECT_EQ(memcmp(mon.moves, set->moves, sizeof(mon.moves)), 0);
                EXPECT_EQ(memcmp(mon.ev, set->training->evs, 6), 0);
                EXPECT_EQ(mon.species, original.species);
                EXPECT_EQ(mon.lvl, original.lvl);
                EXPECT_EQ(memcmp(set, &before, sizeof(before)), 0);
                EXPECT_EQ(memcmp(set->training, &trainingBefore, sizeof(trainingBefore)), 0);
                for (u32 m = 0; m < ARRAY_COUNT(sBatch2Sets); m++)
                    if (set->finalSpecies == sBatch2Sets[m].species)
                        seen[m] = TRUE;
            }
            // Compare bundle identity relationships, not test-local pointers.
            for (u32 k = 0; k <= i; k++)
                for (u32 m = 0; m < PARTY_SIZE; m++)
                {
                    const struct Batch2Encounter *other = &sBatch2Encounters[k];
                    const struct LASetBundle *otherBundle = GetLASetBundle(other->id, DIFFICULTY_NORMAL, other->keys[m], other->guards[m]);
                    EXPECT_EQ(bundle == otherBundle, c->bundles[j] == other->bundles[m]);
                }
            assignments++;
        }
    }
    EXPECT_EQ(assignments, 42);
    EXPECT_EQ(ARRAY_COUNT(sBatch2Sets), 12);
    for (u32 i = 0; i < ARRAY_COUNT(seen); i++)
        EXPECT(seen[i]);
    EXPECT_EQ(memcmp(&r1, &gRngValue, sizeof(r1)), 0);
    EXPECT_EQ(memcmp(&r2, &gRng2Value, sizeof(r2)), 0);
}

TEST("Trainer Sets: production census 112 unique Normal rows with exactly 42 Batch 2")
{
    u32 batch2 = 0;
    EXPECT_EQ(ARRAY_COUNT(sLASetAssignments), 112);
    for (u32 i = 0; i < ARRAY_COUNT(sLASetAssignments); i++)
    {
        const struct LASetAssignment *row = &sLASetAssignments[i];
        EXPECT_EQ(row->difficulty, DIFFICULTY_NORMAL);
        EXPECT(GetLASetBundle(row->trainerId, row->difficulty, row->sourceKey, row->authoredSpecies) != NULL);
        for (u32 j = 0; j < i; j++)
        {
            const struct LASetAssignment *other = &sLASetAssignments[j];
            EXPECT(row->trainerId != other->trainerId || row->difficulty != other->difficulty
                || row->sourceKey != other->sourceKey);
        }
        for (u32 j = 0; j < ARRAY_COUNT(sBatch2Encounters); j++)
            if (row->trainerId == sBatch2Encounters[j].id)
            {
                const struct Batch2Encounter *c = &sBatch2Encounters[j];
                u32 matches = 0;
                for (u32 k = 0; k < PARTY_SIZE; k++)
                    if (row->sourceKey == c->keys[k] && row->authoredSpecies == c->guards[k]
                     && row->bundle == c->bundles[k])
                        matches++;
                EXPECT_EQ(matches, 1);
                batch2++;
            }
    }
    EXPECT_EQ(batch2, 42);
}

static void CheckBatch2Encounter(u32 index, u8 delta)
{
    Setup();
    const struct Batch2Encounter *c = &sBatch2Encounters[index];
    if (delta != 0)
    {
        // Real Phase 2 cells: ACE/ELITE=31, ACE/MASTER=37.
        SetTrainerRank(TRAINER_RANK_ACE);
        u8 base = index == 0 ? 31 : 37;
        SetWorldPhase(index == 0 ? WORLD_PHASE_ELITE : WORLD_PHASE_MASTER);
        u8 playerLevel = base + 3 * (c->anchor + delta - base);
        CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_MAGIKARP, playerLevel, 0, OTID_STRUCT_PLAYER_ID);
    }
    const struct Trainer trainer = {.party = c->sources, .partySize = c->sourceCount};
    const struct LARosterProfile *profile = GetLARosterProfile(c->id, DIFFICULTY_NORMAL);
    struct LARosterSelection selected = SelectLARoster(&trainer, profile, PARTY_SIZE);
    ASSUME(selected.count == PARTY_SIZE);
    struct TrainerMon before[PARTY_SIZE];
    u8 anchor = 0;
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        before[i] = *selected.members[i].source;
        anchor = max(anchor, before[i].lvl);
        EXPECT_EQ(selected.members[i].sourceKey, c->keys[i]);
        EXPECT_EQ(before[i].species, c->guards[i]);
    }
    EXPECT_EQ(anchor, c->anchor);
    struct LAPartyStrength strength = CalculateTrainerPartyStrength();
    u8 world = CalculateTrainerScalingWorldLevel(GetTrainerRank(), GetWorldPhase(), strength.avgLevel, strength.usableCount);
    EXPECT_EQ(CalculateTrainerLevelDelta(anchor, world, 0), delta);
    struct BattleHistory history = {0};
    history.trainerItems[B_TRAINER_OPPONENT_A][0] = ITEM_FULL_RESTORE;
    history.trainerItems[B_TRAINER_OPPONENT_A][1] = ITEM_HYPER_POTION;
    struct BattleHistory historyBefore = history;
    struct BattleHistory *oldHistory = gBattleHistory;
    gBattleHistory = &history;
    void (*oldObserver)(u32) = gTestLARosterSourceObserver;
    sSouthwestObservedCount = 0;
    gTestLARosterSourceObserver = ObserveSouthwestKey;
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer, c->id, GetLATrainerPolicy(0));
    gTestLARosterSourceObserver = oldObserver;
    gBattleHistory = oldHistory;
    EXPECT_EQ(memcmp(&history, &historyBefore, sizeof(history)), 0);
    EXPECT_EQ(sSouthwestObservedCount, PARTY_SIZE);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        enum Species expected = c->species[i];
        if (index == 0 && delta >= 4 && i == 1)
            expected = SPECIES_SHARPEDO;
        if (index == 0 && delta >= 4 && i == 2)
            expected = SPECIES_TENTACRUEL;
        if (index == 3 && delta >= 8 && i == 1)
            expected = SPECIES_EXPLOUD;
        struct Pokemon *mon = &gParties[B_TRAINER_OPPONENT_A][i];
        EXPECT_EQ(GetMonData(mon, MON_DATA_SPECIES), expected);
        EXPECT_EQ(GetMonData(mon, MON_DATA_LEVEL), c->levels[i] + delta);
        EXPECT_EQ(sSouthwestObservedKeys[i], c->keys[i]);
        EXPECT_EQ(memcmp(&before[i], selected.members[i].source, sizeof(before[i])), 0);
        const struct LASetBundle *bundle = GetLASetBundle(c->id, DIFFICULTY_NORMAL, c->keys[i], c->guards[i]);
        const struct LACompetitiveSet *set = NULL;
        for (u32 j = 0; j < bundle->count; j++)
            if (bundle->variants[j]->finalSpecies == expected)
                set = bundle->variants[j];
        ASSUME(set != NULL);
        ExpectBatch2Record(set);
        CheckGenerated(mon, set);
    }
    Setup();
}

TEST("Trainer Sets: Batch 2 ELLIOT_3 exact six production construction") { CheckBatch2Encounter(0, 0); }

TEST("Trainer Sets: Batch 2 ELLIOT_4 exact six production construction") { CheckBatch2Encounter(1, 0); }

TEST("Trainer Sets: Batch 2 ELLIOT_5 exact six production construction") { CheckBatch2Encounter(2, 0); }

TEST("Trainer Sets: Batch 2 KAREN_4 exact six production construction") { CheckBatch2Encounter(3, 0); }

TEST("Trainer Sets: Batch 2 KAREN_5 exact six production construction") { CheckBatch2Encounter(4, 0); }

TEST("Trainer Sets: Batch 2 JERRY_4 exact six production construction") { CheckBatch2Encounter(5, 0); }

TEST("Trainer Sets: Batch 2 JERRY_5 exact six production construction") { CheckBatch2Encounter(6, 0); }

TEST("Trainer Sets: Batch 2 Carvanha Sharpedo and Tentacool Tentacruel delta three boundary")
{
    CheckBatch2Encounter(0, 3);
}
TEST("Trainer Sets: Batch 2 Carvanha Sharpedo and Tentacool Tentacruel delta four boundary")
{
    CheckBatch2Encounter(0, 4);
}
TEST("Trainer Sets: Batch 2 Loudred Exploud delta seven boundary")
{
    CheckBatch2Encounter(3, 7);
}
TEST("Trainer Sets: Batch 2 Loudred Exploud delta eight boundary")
{
    CheckBatch2Encounter(3, 8);
}
TEST("Trainer Sets: Batch 2 Surskit Masquerain guard and immediate application")
{
    for (u32 i = 3; i <= 4; i++)
    {
        const struct Batch2Encounter *c = &sBatch2Encounters[i];
        const struct LASetBundle *bundle = GetLASetBundle(c->id, DIFFICULTY_NORMAL, 0x80080002u, SPECIES_SURSKIT);
        ASSUME(bundle != NULL);
        EXPECT_EQ(bundle->count, 1);
        EXPECT_EQ(bundle->variants[0]->finalSpecies, SPECIES_MASQUERAIN);
        EXPECT(GetLASetBundle(c->id, DIFFICULTY_NORMAL, 0x80080002u, SPECIES_MASQUERAIN) == NULL);
        struct TrainerMon mon = MON(SPECIES_SURSKIT, c->anchor);
        ApplyTrainerEvolution(&mon, NULL);
        EXPECT_EQ(mon.species, SPECIES_MASQUERAIN);
        EXPECT(ApplyLACompetitiveSet(&mon, bundle));
    }
}
TEST("Trainer Sets: Batch 2 Kirlia branch stop and Roselia stone stop through level 100")
{
    const u16 ids[] = {TRAINER_JERRY_4, TRAINER_JERRY_5, TRAINER_KAREN_4, TRAINER_KAREN_5};
    const u8 bases[] = {32, 34, 32, 35};
    for (u32 i = 0; i < ARRAY_COUNT(ids); i++)
    {
        enum Species species = i < 2 ? SPECIES_KIRLIA : SPECIES_ROSELIA;
        u32 key = i < 2 ? 0 : 0x80080003u;
        const struct LASetBundle *bundle = GetLASetBundle(ids[i], DIFFICULTY_NORMAL, key, species);
        for (u32 level = bases[i]; level <= MAX_LEVEL; level++)
        {
            struct TrainerMon mon = MON(species, level);
            ApplyTrainerEvolution(&mon, NULL);
            EXPECT_EQ(mon.species, species);
            EXPECT(ApplyLACompetitiveSet(&mon, bundle));
            EXPECT_EQ(mon.heldItem, ITEM_EVIOLITE);
        }
    }
}
TEST("Trainer Sets: Batch 2 six existing bundle pointers reused unchanged")
{
    EXPECT(GetLASetBundle(TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0, SPECIES_BRELOOM)
        == GetLASetBundle(TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0x80020001u, SPECIES_SHROOMISH));
    EXPECT(GetLASetBundle(TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080001u, SPECIES_BEAUTIFLY)
        == GetLASetBundle(TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040004u, SPECIES_BEAUTIFLY));
    EXPECT(GetLASetBundle(TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080002u, SPECIES_SURSKIT)
        == GetLASetBundle(TRAINER_JAMES_5, DIFFICULTY_NORMAL, 0, SPECIES_SURSKIT));
    EXPECT(GetLASetBundle(TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080003u, SPECIES_ROSELIA)
        == GetLASetBundle(TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040002u, SPECIES_ROSELIA));
    EXPECT(GetLASetBundle(TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080004u, SPECIES_NINJASK)
        == GetLASetBundle(TRAINER_JAMES_5, DIFFICULTY_NORMAL, 3, SPECIES_NINJASK));
    EXPECT(GetLASetBundle(TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0x80090001u, SPECIES_GRUMPIG)
        == GetLASetBundle(TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030002u, SPECIES_SPOINK));
}
TEST("Trainer Sets: Batch 2 earlier encounters controls doubles and majors remain unassigned")
{
    const u16 inactive[] = {
        TRAINER_ELLIOT_1, TRAINER_ELLIOT_2,
        TRAINER_KAREN_1, TRAINER_KAREN_2, TRAINER_KAREN_3,
        TRAINER_JERRY_1, TRAINER_JERRY_2, TRAINER_JERRY_3,
        TRAINER_JOEY, TRAINER_JOSE, TRAINER_DEVAN, TRAINER_DOUGLAS,
        TRAINER_KYLA, TRAINER_NED, TRAINER_KEVIN, TRAINER_KATE_AND_JOY,
        TRAINER_ROXANNE_1, TRAINER_ROXANNE_2, TRAINER_ROXANNE_3, TRAINER_ROXANNE_4, TRAINER_ROXANNE_5,
        TRAINER_BRAWLY_1, TRAINER_BRAWLY_2, TRAINER_BRAWLY_3, TRAINER_BRAWLY_4, TRAINER_BRAWLY_5,
    };
    for (u32 i = 0; i < ARRAY_COUNT(inactive); i++)
    {
        EXPECT(GetLARosterProfile(inactive[i], DIFFICULTY_NORMAL) == NULL);
        for (u32 j = 0; j < ARRAY_COUNT(sLASetAssignments); j++)
            EXPECT(sLASetAssignments[j].trainerId != inactive[i]);
    }
    EXPECT(GetLASetBundle(TRAINER_JERRY_5, DIFFICULTY_NORMAL, 0x80090004u, SPECIES_BANETTE) == NULL);
}
