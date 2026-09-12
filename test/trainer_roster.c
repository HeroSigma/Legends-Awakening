#include "global.h"
#include "battle.h"
#include "battle_setup.h"
#include "debug.h"
#include "pokemon.h"
#include "random.h"
#include "trainer_roster.h"
#include "trainer_pools.h"
#include "trainer_evolution.h"
#include "trainer_scaling.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "test/test.h"

extern void TestCreateLATrainerParty(struct Pokemon *, const struct Trainer *, u16);
extern void TestCreateLATrainerPartyWithPolicyForId(struct Pokemon *, const struct Trainer *,
    u16, struct LATrainerPolicy);
extern void TestCreateLATrainerRosterParty(struct Pokemon *, const struct Trainer *,
    u16, struct LATrainerPolicy, const struct LARosterProfile *);
extern void (*gTestLARosterSourceObserver)(u32);
extern const struct TrainerEvolutionProfile *TestFindTrainerEvolutionProfile(
    const struct TrainerEvolutionAssignment *, u32, u16, enum DifficultyLevel, u32, enum Species);

#define MON(speciesId, level) {.species = speciesId, .lvl = level, .gender = TRAINER_MON_RANDOM_GENDER}
static const struct TrainerMon sAuthored[] = {MON(SPECIES_GEODUDE, 21)};
static const struct Trainer sTrainer = {.party = sAuthored, .partySize = ARRAY_COUNT(sAuthored)};
static const struct LARosterAuthoredRef sRetained[] = {{0, SPECIES_GEODUDE}};
static const struct LARosterSupplement sSupplements[] =
{
    {1, MON(SPECIES_NUMEL, 21)}, {2, MON(SPECIES_MACHOP, 21)},
    {3, MON(SPECIES_ARON, 21)}, {4, MON(SPECIES_ONIX, 21)}, {5, MON(SPECIES_TORKOAL, 21)},
};
static const struct LARosterProfile sProfile =
{
    .retained = sRetained, .supplements = sSupplements, .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sRetained), .supplementCount = ARRAY_COUNT(sSupplements),
};

static void ExpectFailure(struct LARosterSelection result, enum LARosterSelectionStatus status)
{
    EXPECT_EQ(result.status, status);
    EXPECT_EQ(result.count, 0);
    for (u32 i = 0; i < PARTY_SIZE; i++)
    {
        EXPECT(result.members[i].source == NULL);
        EXPECT_EQ(result.members[i].sourceKey, 0);
    }
}

TEST("Trainer Roster: authored and supplemental keys occupy separate namespaces")
{
    struct LARosterSelection r = SelectLARoster(&sTrainer, &sProfile, 6);
    EXPECT_EQ(r.status, LA_ROSTER_SELECTION_COMPLETE);
    EXPECT_EQ(r.members[0].sourceKey, 0);
    for (u32 i = 1; i < 6; i++)
    {
        EXPECT_EQ(r.members[i].sourceKey, 0x80010000u | i);
        EXPECT(r.members[i].source == &sSupplements[i - 1].mon);
    }
    EXPECT_EQ(LA_ROSTER_SUPPLEMENT_KEY(32767, 65535), 0xFFFFFFFFu);
}

TEST("Trainer Roster: keys survive priority and output reorder")
{
    struct LARosterSupplement supplements[ARRAY_COUNT(sSupplements)];
    memcpy(supplements, sSupplements, sizeof(supplements));
    struct LARosterSupplement temp = supplements[0];
    supplements[0] = supplements[4];
    supplements[4] = temp;
    struct LARosterProfile p = sProfile;
    p.supplements = supplements;
    struct LARosterSelection r = SelectLARoster(&sTrainer, &p, 6);
    EXPECT_EQ(r.members[1].sourceKey, 0x80010005u);
    EXPECT_EQ(r.members[5].sourceKey, 0x80010001u);
    p.retainedCount = 0;
    r = SelectLARoster(&sTrainer, &p, 3);
    EXPECT_EQ(r.members[0].sourceKey, 0x80010005u);
}

TEST("Trainer Roster: exact difficulty wins and ANY supplies fallback")
{
    const struct LARosterProfile other = sProfile;
    const struct LARosterAssignment rows[] = {
        {0, LA_ROSTER_DIFFICULTY_ANY, &sProfile}, {0, DIFFICULTY_HARD, &other}};
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_HARD) == &other);
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_NORMAL) == &sProfile);
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 3, DIFFICULTY_NORMAL) == NULL);
}

TEST("Trainer Roster: duplicate exact and ANY assignments fail closed")
{
    const struct LARosterAssignment exact[] = {
        {0, LA_ROSTER_DIFFICULTY_ANY, &sProfile},
        {0, DIFFICULTY_NORMAL, &sProfile}, {0, DIFFICULTY_NORMAL, &sProfile}};
    const struct LARosterAssignment any[] = {
        {0, LA_ROSTER_DIFFICULTY_ANY, &sProfile}, {0, LA_ROSTER_DIFFICULTY_ANY, &sProfile}};
    EXPECT(FindLARosterProfile(exact, ARRAY_COUNT(exact), 0, DIFFICULTY_NORMAL) == NULL);
    EXPECT(FindLARosterProfile(any, ARRAY_COUNT(any), 0, DIFFICULTY_NORMAL) == NULL);
}

TEST("Trainer Roster: malformed or invalid exact assignment never falls through")
{
    struct LARosterAssignment rows[] = {
        {0, LA_ROSTER_DIFFICULTY_ANY, &sProfile}, {0, DIFFICULTY_NORMAL, NULL}};
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_NORMAL) == NULL);
    rows[1].difficulty = DIFFICULTY_COUNT;
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_NORMAL) == NULL);
    EXPECT(FindLARosterProfile(NULL, 1, 0, DIFFICULTY_NORMAL) == NULL);
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0xFFFF, DIFFICULTY_NORMAL) == NULL);
    EXPECT(FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_COUNT) == NULL);
    struct LARosterProfile invalid = sProfile;
    invalid.namespaceId = 0;
    rows[1] = (struct LARosterAssignment){0, DIFFICULTY_NORMAL, &invalid};
    const struct LARosterProfile *resolved = FindLARosterProfile(rows, ARRAY_COUNT(rows), 0, DIFFICULTY_NORMAL);
    EXPECT(resolved == &invalid);
    ExpectFailure(SelectLARoster(&sTrainer, resolved, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: NULL storage with nonzero count rejected")
{
    struct LARosterProfile p = sProfile;
    p.retained = NULL;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
    p = sProfile;
    p.supplements = NULL;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
    struct Trainer trainer = sTrainer;
    trainer.party = NULL;
    ExpectFailure(SelectLARoster(&trainer, &sProfile, 6), LA_ROSTER_SELECTION_INVALID);
    ExpectFailure(SelectLARoster(NULL, &sProfile, 6), LA_ROSTER_SELECTION_INVALID);
    ExpectFailure(SelectLARoster(&sTrainer, NULL, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: invalid namespace rejected")
{
    struct LARosterProfile p = sProfile;
    p.namespaceId = 0;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
    p.namespaceId = 32768;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: zero candidate ID and duplicate keys rejected")
{
    struct LARosterSupplement candidates[ARRAY_COUNT(sSupplements)];
    memcpy(candidates, sSupplements, sizeof(candidates));
    struct LARosterProfile p = sProfile;
    p.supplements = candidates;
    candidates[4].candidateId = 0;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 3), LA_ROSTER_SELECTION_INVALID);
    candidates[4].candidateId = candidates[0].candidateId;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: NONE Egg out-of-range and disabled species rejected")
{
    const enum Species invalid[] = {SPECIES_NONE, SPECIES_EGG, NUM_SPECIES};
    struct LARosterSupplement candidate = sSupplements[0];
    struct LARosterProfile p = sProfile;
    p.supplements = &candidate;
    p.supplementCount = 1;
    for (u32 i = 0; i < ARRAY_COUNT(invalid); i++)
    {
        candidate.mon.species = invalid[i];
        ExpectFailure(SelectLARoster(&sTrainer, &p, 1), LA_ROSTER_SELECTION_INVALID);
    }
    for (u32 species = 1; species < NUM_SPECIES; species++)
    {
        if (species != SPECIES_EGG && !IsSpeciesEnabled(species))
        {
            candidate.mon.species = species;
            ExpectFailure(SelectLARoster(&sTrainer, &p, 1), LA_ROSTER_SELECTION_INVALID);
            break;
        }
    }
}

TEST("Trainer Roster: supplemental levels must be 1 through MAX_LEVEL")
{
    struct LARosterSupplement candidate = sSupplements[0];
    struct LARosterProfile p = sProfile;
    p.supplements = &candidate;
    p.supplementCount = 1;
    candidate.mon.lvl = 0;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 1), LA_ROSTER_SELECTION_INVALID);
    candidate.mon.lvl = MAX_LEVEL + 1;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 1), LA_ROSTER_SELECTION_INVALID);
    candidate.mon.lvl = MAX_LEVEL;
    EXPECT_EQ(SelectLARoster(&sTrainer, &p, 2).status, LA_ROSTER_SELECTION_COMPLETE);
}

TEST("Trainer Roster: stale and out-of-bounds authored references rejected")
{
    struct LARosterAuthoredRef ref = {0, SPECIES_PINSIR};
    struct LARosterProfile p = sProfile;
    p.retained = &ref;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
    ref = (struct LARosterAuthoredRef){1, SPECIES_GEODUDE};
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
    ref.sourceIndex = 0x80010001u;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: duplicate retained references rejected")
{
    const struct LARosterAuthoredRef refs[] = {{0, SPECIES_GEODUDE}, {0, SPECIES_GEODUDE}};
    struct LARosterProfile p = sProfile;
    p.retained = refs;
    p.retainedCount = ARRAY_COUNT(refs);
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INVALID);
}

TEST("Trainer Roster: insufficient candidates expose no partial result")
{
    struct LARosterProfile p = sProfile;
    p.supplementCount--;
    ExpectFailure(SelectLARoster(&sTrainer, &p, 6), LA_ROSTER_SELECTION_INCOMPLETE);
}

TEST("Trainer Roster: invalid targets rejected and exact target three supported")
{
    ExpectFailure(SelectLARoster(&sTrainer, &sProfile, 0), LA_ROSTER_SELECTION_INVALID);
    ExpectFailure(SelectLARoster(&sTrainer, &sProfile, 7), LA_ROSTER_SELECTION_INVALID);
    struct LARosterSelection r = SelectLARoster(&sTrainer, &sProfile, 3);
    EXPECT_EQ(r.status, LA_ROSTER_SELECTION_COMPLETE);
    EXPECT_EQ(r.count, 3);
    EXPECT_EQ(r.members[2].source->species, SPECIES_MACHOP);
    EXPECT(r.members[3].source == NULL);
}

TEST("Trainer Roster: intentional duplicate species with distinct identities are valid")
{
    const struct LARosterSupplement candidates[] = {
        {1, MON(SPECIES_GEODUDE, 21)}, {2, MON(SPECIES_GEODUDE, 21)}};
    struct LARosterProfile p = sProfile;
    p.supplements = candidates;
    p.supplementCount = ARRAY_COUNT(candidates);
    struct LARosterSelection r = SelectLARoster(&sTrainer, &p, 3);
    EXPECT_EQ(r.status, LA_ROSTER_SELECTION_COMPLETE);
    EXPECT_EQ(r.count, 3);
    EXPECT(r.members[0].sourceKey != r.members[1].sourceKey);
    EXPECT(r.members[1].sourceKey != r.members[2].sourceKey);
}

TEST("Trainer Roster: deterministic selection preserves RNG and ROM")
{
    rng_value_t rng1 = gRngValue, rng2 = gRng2Value;
    struct TrainerMon before = sAuthored[0];
    struct LARosterSupplement supplements[ARRAY_COUNT(sSupplements)];
    memcpy(supplements, sSupplements, sizeof(supplements));
    struct LARosterSelection a = SelectLARoster(&sTrainer, &sProfile, 6);
    struct LARosterSelection b = SelectLARoster(&sTrainer, &sProfile, 6);
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT(a.members[i].source == b.members[i].source);
        EXPECT_EQ(a.members[i].sourceKey, b.members[i].sourceKey);
    }
    EXPECT_EQ(memcmp(&rng1, &gRngValue, sizeof(rng1)), 0);
    EXPECT_EQ(memcmp(&rng2, &gRng2Value, sizeof(rng2)), 0);
    EXPECT_EQ(memcmp(&before, sAuthored, sizeof(before)), 0);
    EXPECT_EQ(memcmp(supplements, sSupplements, sizeof(supplements)), 0);
}

TEST("Trainer Roster: retained order and pool source indices preserved")
{
    const struct TrainerMon entries[] = {MON(SPECIES_GEODUDE, 21), MON(SPECIES_PINSIR, 21)};
    const struct Trainer trainer = {.party = entries, .partySize = 1, .poolSize = ARRAY_COUNT(entries)};
    const struct LARosterAuthoredRef refs[] = {{1, SPECIES_PINSIR}, {0, SPECIES_GEODUDE}};
    struct LARosterProfile p = sProfile;
    p.retained = refs;
    p.retainedCount = ARRAY_COUNT(refs);
    struct LARosterSelection r = SelectLARoster(&trainer, &p, 3);
    EXPECT_EQ(r.status, LA_ROSTER_SELECTION_COMPLETE);
    EXPECT_EQ(r.members[0].sourceKey, 1);
    EXPECT_EQ(r.members[1].sourceKey, 0);
    EXPECT_EQ(r.members[2].sourceKey, 0x80010001u);
}

static void SetupConstruction(void)
{
    gIsDebugBattle = FALSE;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    ZeroPlayerPartyMons();
    SetTrainerRank(TRAINER_RANK_ROOKIE);
    SetWorldPhase(WORLD_PHASE_BEGINNING);
    gTestLARosterSourceObserver = NULL;
}

static u32 CountConstructed(void)
{
    u32 count = 0;
    for (u32 i = 0; i < PARTY_SIZE; i++)
        if (GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES) != SPECIES_NONE)
            count++;
    return count;
}

// The test ROM has a tiny replacement trainer table. These source fixtures
// reproduce the production Sawyer/Gabrielle species and levels; profiles below
// are the real production profiles, not test-local replacements.
static const struct TrainerMon sSawyerAuthored[5][3] =
{
    {MON(SPECIES_GEODUDE, 21)},
    {MON(SPECIES_GEODUDE, 26), MON(SPECIES_NUMEL, 26)},
    {MON(SPECIES_MACHOP, 28), MON(SPECIES_NUMEL, 28), MON(SPECIES_GRAVELER, 28)},
    {MON(SPECIES_MACHOP, 30), MON(SPECIES_NUMEL, 30), MON(SPECIES_GRAVELER, 30)},
    {MON(SPECIES_MACHOKE, 33), MON(SPECIES_CAMERUPT, 33), MON(SPECIES_GOLEM, 33)},
};
static const u16 sSawyerIds[] = {TRAINER_SAWYER_1, TRAINER_SAWYER_2, TRAINER_SAWYER_3, TRAINER_SAWYER_4, TRAINER_SAWYER_5};
static const u8 sSawyerCounts[] = {1, 2, 3, 3, 3};
static const u8 sSawyerLevels[] = {21, 26, 28, 30, 33};
static const enum Species sSawyerExpected[5][6] =
{
    {SPECIES_GEODUDE, SPECIES_NUMEL, SPECIES_MACHOP, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_GEODUDE, SPECIES_NUMEL, SPECIES_MACHOP, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOP, SPECIES_NUMEL, SPECIES_GRAVELER, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOP, SPECIES_NUMEL, SPECIES_GRAVELER, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
    {SPECIES_MACHOKE, SPECIES_CAMERUPT, SPECIES_GOLEM, SPECIES_ARON, SPECIES_ONIX, SPECIES_TORKOAL},
};

static void CheckSawyer(u32 stage)
{
    SetupConstruction();
    const struct Trainer trainer = {.party = sSawyerAuthored[stage], .partySize = sSawyerCounts[stage]};
    const struct LARosterProfile *profile = GetLARosterProfile(sSawyerIds[stage], DIFFICULTY_NORMAL);
    struct LARosterSelection r = SelectLARoster(&trainer, profile, 6);
    EXPECT_EQ(r.status, LA_ROSTER_SELECTION_COMPLETE);
    EXPECT_EQ(r.count, 6);
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT_EQ(r.members[i].source->species, sSawyerExpected[stage][i]);
        EXPECT_EQ(r.members[i].source->lvl, sSawyerLevels[stage]);
    }
    // Sawyer 1's numeric ID is a Rival in the test ROM's replacement table.
    // Supply his production ORDINARY policy while retaining the real resolver.
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &trainer,
        sSawyerIds[stage], GetLATrainerPolicy(0));
    EXPECT_EQ(CountConstructed(), 6);
    for (u32 i = 0; i < 6; i++)
    {
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES),
            ResolveTrainerScaledSpecies(sSawyerExpected[stage][i], sSawyerLevels[stage], NULL));
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_LEVEL), sSawyerLevels[stage]);
    }
}

TEST("Trainer Roster: Sawyer 1 exact six and real construction") { CheckSawyer(0); }

TEST("Trainer Roster: Sawyer 2 exact six and real construction") { CheckSawyer(1); }

TEST("Trainer Roster: Sawyer 3 exact six and real construction") { CheckSawyer(2); }

TEST("Trainer Roster: Sawyer 4 exact six and real construction") { CheckSawyer(3); }

TEST("Trainer Roster: Sawyer 5 exact six and real construction") { CheckSawyer(4); }

TEST("Trainer Roster: production candidate identities stable across Sawyer stages")
{
    for (u32 stage = 0; stage < ARRAY_COUNT(sSawyerIds); stage++)
    {
        const struct LARosterProfile *p = GetLARosterProfile(sSawyerIds[stage], DIFFICULTY_NORMAL);
        EXPECT(p != NULL);
        for (u32 i = 0; i < p->supplementCount; i++)
        {
            const struct LARosterSupplement *s = &p->supplements[i];
            EXPECT_EQ(s->mon.species, sSupplements[s->candidateId - 1].mon.species);
            EXPECT_EQ(LA_ROSTER_SUPPLEMENT_KEY(p->namespaceId, s->candidateId), 0x80010000u | s->candidateId);
        }
    }
}

TEST("Trainer Roster: Gabrielle unassigned original six control")
{
    SetupConstruction();
    const struct TrainerMon entries[] = {
        MON(SPECIES_SKITTY, 26), MON(SPECIES_POOCHYENA, 26), MON(SPECIES_ZIGZAGOON, 26),
        MON(SPECIES_LOTAD, 26), MON(SPECIES_SEEDOT, 26), MON(SPECIES_TAILLOW, 26)};
    const struct Trainer trainer = {.party = entries, .partySize = ARRAY_COUNT(entries)};
    EXPECT(GetLARosterProfile(TRAINER_GABRIELLE_1, DIFFICULTY_NORMAL) == NULL);
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, TRAINER_GABRIELLE_1);
    EXPECT_EQ(CountConstructed(), 6);
    for (u32 i = 0; i < 6; i++)
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES),
            ResolveTrainerScaledSpecies(entries[i].species, 26, NULL));
}

TEST("Trainer Roster: unassigned short ordinary trainer retains original count")
{
    SetupConstruction();
    EXPECT(GetLARosterProfile(0, DIFFICULTY_NORMAL) == NULL);
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &sTrainer, 0);
    EXPECT_EQ(CountConstructed(), 1);
}

TEST("Trainer Roster: missing flag HANDCRAFTED MAJOR SPECIAL and EXEMPT bypass")
{
    SetupConstruction();
    struct LATrainerPolicy policies[] = {
        {LA_TRAINER_ORDINARY, LA_POLICY_ORDINARY & ~LA_TRAINER_POLICY_FULL_PARTY},
        {LA_TRAINER_ORDINARY, LA_POLICY_ORDINARY | LA_TRAINER_POLICY_HANDCRAFTED},
        {LA_TRAINER_MAJOR, LA_POLICY_MAJOR},
        {LA_TRAINER_SPECIAL, LA_POLICY_ORDINARY},
        {LA_TRAINER_EXEMPT, LA_POLICY_EXEMPT},
    };
    for (u32 i = 0; i < ARRAY_COUNT(policies); i++)
    {
        TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &sTrainer, 0, policies[i], &sProfile);
        EXPECT_EQ(CountConstructed(), 1);
    }
}

TEST("Trainer Roster: debug invalid partner and ID-less construction bypass")
{
    SetupConstruction();
    gIsDebugBattle = TRUE;
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &sTrainer, 0, GetLATrainerPolicy(0), &sProfile);
    EXPECT_EQ(CountConstructed(), 1);
    gIsDebugBattle = FALSE;
    const u16 ids[] = {0xFFFF, TRAINERS_COUNT, TRAINER_SECRET_BASE, TRAINER_PARTNER(1)};
    for (u32 i = 0; i < ARRAY_COUNT(ids); i++)
    {
        TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &sTrainer, ids[i], GetLATrainerPolicy(0), &sProfile);
        EXPECT_EQ(CountConstructed(), 1);
    }
    CreateNPCTrainerPartyFromTrainer(gParties[B_TRAINER_OPPONENT_A], &sTrainer);
    EXPECT_EQ(CountConstructed(), 1);
}

TEST("Trainer Roster: multi two-opponent and special controller contexts bypass")
{
    SetupConstruction();
    const u32 excluded[] = {BATTLE_TYPE_TWO_OPPONENTS, BATTLE_TYPE_MULTI, BATTLE_TYPE_INGAME_PARTNER,
        BATTLE_TYPE_LINK, BATTLE_TYPE_LINK_IN_BATTLE, BATTLE_TYPE_FRONTIER, BATTLE_TYPE_SAFARI,
        BATTLE_TYPE_FIRST_BATTLE, BATTLE_TYPE_RECORDED, BATTLE_TYPE_EREADER_TRAINER,
        BATTLE_TYPE_SECRET_BASE, BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_ROAMER, BATTLE_TYPE_RAID};
    for (u32 i = 0; i < ARRAY_COUNT(excluded); i++)
    {
        gBattleTypeFlags = BATTLE_TYPE_TRAINER | excluded[i];
        TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &sTrainer, 0, GetLATrainerPolicy(0), &sProfile);
        EXPECT_EQ(CountConstructed(), 1);
    }
    EXPECT(!CanApplyLARoster(0, GetLATrainerPolicy(0), TRUE, BATTLE_TYPE_TRAINER, TRUE));
    EXPECT(!CanApplyLARoster(0, GetLATrainerPolicy(0), TRUE, 0, FALSE));
}

TEST("Trainer Roster: one-trainer doubles expands to six")
{
    SetupConstruction();
    gBattleTypeFlags |= BATTLE_TYPE_DOUBLE;
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &sTrainer,
        TRAINER_SAWYER_1, GetLATrainerPolicy(0));
    EXPECT_EQ(CountConstructed(), 6);
}

TEST("Trainer Roster: original two-opponent half-team limit retained")
{
    SetupConstruction();
    const struct TrainerMon entries[] = {
        MON(SPECIES_PINSIR, 21), MON(SPECIES_PINSIR, 21),
        MON(SPECIES_PINSIR, 21), MON(SPECIES_PINSIR, 21)};
    const struct Trainer trainer = {.party = entries, .partySize = ARRAY_COUNT(entries), .multiTeamSize = MULTI_TEAM_SIZE_HALF};
    gBattleTypeFlags |= BATTLE_TYPE_TWO_OPPONENTS;
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &sProfile);
    EXPECT_EQ(CountConstructed(), 3);
}

TEST("Trainer Roster: complete selection bypasses pool and failure uses original pool")
{
    SetupConstruction();
    const struct TrainerMon entries[] = {MON(SPECIES_GEODUDE, 21), MON(SPECIES_PINSIR, 21)};
    const struct Trainer trainer = {.party = entries, .partySize = 1, .poolSize = ARRAY_COUNT(entries), .poolPickIndex = POOL_PICK_LOWEST};
    const struct LARosterAuthoredRef ref = {1, SPECIES_PINSIR};
    struct LARosterProfile p = sProfile;
    p.retained = &ref;
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &p);
    EXPECT_EQ(CountConstructed(), 6);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_PINSIR);
    p.supplementCount = 0;
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &p);
    EXPECT_EQ(CountConstructed(), 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_GEODUDE);
    p.namespaceId = 0;
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &p);
    EXPECT_EQ(CountConstructed(), 1);
}

TEST("Trainer Roster: Phase 2 anchor includes supplement and sources remain immutable")
{
    SetupConstruction();
    SetTrainerRank(TRAINER_RANK_LEGEND);
    const struct TrainerMon entries[] = {MON(SPECIES_PINSIR, 8)};
    const struct Trainer trainer = {.party = entries, .partySize = 1};
    const struct LARosterAuthoredRef ref = {0, SPECIES_PINSIR};
    struct LARosterSupplement candidates[ARRAY_COUNT(sSupplements)];
    memcpy(candidates, sSupplements, sizeof(candidates));
    candidates[0].mon.lvl = 100;
    struct LARosterProfile p = sProfile;
    p.retained = &ref;
    p.supplements = candidates;
    struct LARosterSupplement before[ARRAY_COUNT(candidates)];
    memcpy(before, candidates, sizeof(before));
    TestCreateLATrainerRosterParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0, GetLATrainerPolicy(0), &p);
    EXPECT_EQ(CountConstructed(), 6);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 8);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_LEVEL), 100);
    EXPECT_EQ(entries[0].lvl, 8);
    EXPECT_EQ(memcmp(before, candidates, sizeof(before)), 0);
}

static u32 sObservedKeys[PARTY_SIZE];
static u32 sObservedCount;
static void ObserveSourceKey(u32 key)
{
    EXPECT(sObservedCount < PARTY_SIZE);
    sObservedKeys[sObservedCount++] = key;
}

TEST("Trainer Roster: Phase 3 receives and matches stable supplemental source keys")
{
    SetupConstruction();
    sObservedCount = 0;
    gTestLARosterSourceObserver = ObserveSourceKey;
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &sTrainer,
        TRAINER_SAWYER_1, GetLATrainerPolicy(0));
    gTestLARosterSourceObserver = NULL;
    EXPECT_EQ(sObservedCount, 6);
    EXPECT_EQ(sObservedKeys[0], 0);
    const struct TrainerEvolutionProfile profile = {.preserveSpecies = TRUE};
    for (u32 i = 1; i < 6; i++)
    {
        EXPECT_EQ(sObservedKeys[i], 0x80010000u | i);
        const struct TrainerEvolutionAssignment row = {
            TRAINER_SAWYER_1, DIFFICULTY_NORMAL, sObservedKeys[i], sSupplements[i - 1].mon.species, &profile};
        EXPECT(TestFindTrainerEvolutionProfile(&row, 1, TRAINER_SAWYER_1,
            DIFFICULTY_NORMAL, sObservedKeys[i], row.authoredSpecies) == &profile);
        EXPECT(TestFindTrainerEvolutionProfile(&row, 1, TRAINER_SAWYER_1,
            DIFFICULTY_NORMAL, i, row.authoredSpecies) == NULL);
    }
}

TEST("Trainer Roster: selected difficulty fallback activates Normal Sawyer profile")
{
    SetupConstruction();
    SetCurrentDifficultyLevel(DIFFICULTY_HARD);
    TestCreateLATrainerPartyWithPolicyForId(gParties[B_TRAINER_OPPONENT_A], &sTrainer,
        TRAINER_SAWYER_1, GetLATrainerPolicy(0));
    EXPECT_EQ(CountConstructed(), 6);
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
}

// Batch 1 fixtures reproduce authored species/levels; profiles are production data.
struct SouthwestRosterCase
{
    u16 trainerId;
    u8 namespaceId;
    u8 retainedCount;
    u8 anchor;
    struct TrainerMon authored[4];
    enum Species supplements[5];
    u8 supplementLevel;
};
static const struct SouthwestRosterCase sSouthwestRosters[] =
{
    {TRAINER_CALVIN_5, 2, 3, 36,
     {MON(SPECIES_SWELLOW, 34), MON(SPECIES_LINOONE, 32), MON(SPECIES_MIGHTYENA, 36)},
     {SPECIES_SHROOMISH, SPECIES_ELECTRIKE, SPECIES_GULPIN}, 32},
    {TRAINER_WINSTON_5, 3, 1, 36,
     {MON(SPECIES_LINOONE, 36)},
     {SPECIES_ELECTRIKE, SPECIES_SPOINK, SPECIES_KECLEON, SPECIES_MAWILE, SPECIES_WINGULL}, 34},
    {TRAINER_CINDY_6, 4, 1, 36,
     {MON(SPECIES_LINOONE, 36)},
     {SPECIES_SKITTY, SPECIES_ROSELIA, SPECIES_SWABLU, SPECIES_BEAUTIFLY, SPECIES_MARILL}, 34},
    {TRAINER_HALEY_5, 5, 3, 34,
     {MON(SPECIES_SWELLOW, 34), MON(SPECIES_LOMBRE, 34), MON(SPECIES_BRELOOM, 34)},
     {SPECIES_ROSELIA, SPECIES_WINGULL, SPECIES_ILLUMISE}, 34},
    {TRAINER_JAMES_5, 6, 4, 33,
     {MON(SPECIES_SURSKIT, 33), MON(SPECIES_NINJASK, 33), MON(SPECIES_DUSTOX, 33), MON(SPECIES_NINJASK, 33)},
     {SPECIES_BEAUTIFLY, SPECIES_VOLBEAT}, 33},
};

TEST("Trainer Roster: Southwest five profiles retain exact sources and 18 stable supplements")
{
    u32 candidates = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sSouthwestRosters); i++)
    {
        const struct SouthwestRosterCase *c = &sSouthwestRosters[i];
        const struct LARosterProfile *p = GetLARosterProfile(c->trainerId, DIFFICULTY_NORMAL);
        ASSUME(p != NULL);
        EXPECT_EQ(p->namespaceId, c->namespaceId);
        EXPECT_EQ(p->retainedCount, c->retainedCount);
        EXPECT_EQ(p->supplementCount, 6 - c->retainedCount);
        const struct Trainer trainer = {.party = c->authored, .partySize = c->retainedCount};
        struct LARosterSelection selected = SelectLARoster(&trainer, p, 6);
        EXPECT_EQ(selected.status, LA_ROSTER_SELECTION_COMPLETE);
        EXPECT_EQ(selected.count, 6);
        u32 anchor = 0;
        for (u32 j = 0; j < selected.count; j++)
        {
            if (selected.members[j].source->lvl > anchor)
                anchor = selected.members[j].source->lvl;
            if (j < c->retainedCount)
            {
                EXPECT_EQ(p->retained[j].sourceIndex, j);
                EXPECT_EQ(p->retained[j].expectedSpecies, c->authored[j].species);
                EXPECT(selected.members[j].source == &c->authored[j]);
                EXPECT_EQ(selected.members[j].sourceKey, j);
            }
            else
            {
                u32 k = j - c->retainedCount;
                const struct LARosterSupplement *s = &p->supplements[k];
                EXPECT_EQ(s->candidateId, k + 1);
                EXPECT_EQ(s->mon.species, c->supplements[k]);
                EXPECT_EQ(s->mon.lvl, c->supplementLevel);
                EXPECT_EQ(selected.members[j].sourceKey, 0x80000000u | ((u32)c->namespaceId << 16) | (k + 1));
                struct TrainerMon expected = MON(c->supplements[k], c->supplementLevel);
                expected.ball = POKEBALL_COUNT;
                expected.nature = NATURE_HARDY;
                EXPECT_EQ(memcmp(&s->mon, &expected, sizeof(expected)), 0);
                candidates++;
            }
        }
        EXPECT_EQ(anchor, c->anchor);
    }
    EXPECT_EQ(candidates, 18);
}

// Batch 2 frozen roster expectations, independent of production profile storage.
static const struct SouthwestRosterCase sBatch2Rosters[] =
{
    {TRAINER_ELLIOT_3, 7, 4, 29,
     {MON(SPECIES_GYARADOS, 29), MON(SPECIES_CARVANHA, 26), MON(SPECIES_TENTACOOL, 26), MON(SPECIES_GYARADOS, 29)},
     {SPECIES_TENTACRUEL, SPECIES_SHARPEDO}, 29},
    {TRAINER_ELLIOT_4, 7, 4, 31,
     {MON(SPECIES_GYARADOS, 31), MON(SPECIES_CARVANHA, 30), MON(SPECIES_TENTACRUEL, 30), MON(SPECIES_GYARADOS, 31)},
     {SPECIES_TENTACRUEL, SPECIES_SHARPEDO}, 31},
    {TRAINER_ELLIOT_5, 7, 4, 35,
     {MON(SPECIES_GYARADOS, 33), MON(SPECIES_SHARPEDO, 33), MON(SPECIES_GYARADOS, 33), MON(SPECIES_TENTACRUEL, 35)},
     {SPECIES_TENTACRUEL, SPECIES_SHARPEDO}, 33},
    {TRAINER_KAREN_4, 8, 2, 32,
     {MON(SPECIES_BRELOOM, 32), MON(SPECIES_LOUDRED, 32)},
     {SPECIES_BEAUTIFLY, SPECIES_SURSKIT, SPECIES_ROSELIA, SPECIES_NINJASK}, 32},
    {TRAINER_KAREN_5, 8, 2, 35,
     {MON(SPECIES_BRELOOM, 35), MON(SPECIES_EXPLOUD, 35)},
     {SPECIES_BEAUTIFLY, SPECIES_SURSKIT, SPECIES_ROSELIA, SPECIES_NINJASK}, 35},
    {TRAINER_JERRY_4, 9, 2, 32,
     {MON(SPECIES_KIRLIA, 32), MON(SPECIES_MEDICHAM, 32)},
     {SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE, SPECIES_BANETTE}, 32},
    {TRAINER_JERRY_5, 9, 3, 34,
     {MON(SPECIES_KIRLIA, 34), MON(SPECIES_BANETTE, 34), MON(SPECIES_MEDICHAM, 34)},
     {SPECIES_GRUMPIG, SPECIES_CHIMECHO, SPECIES_SABLEYE}, 34},
};

TEST("Trainer Roster: exactly seventeen Sawyer Southwest and Batch 2 profiles")
{
    u32 normalCount = 0;
    for (u32 id = 0; id < TRAINERS_COUNT; id++)
    {
        bool32 expected = FALSE;
        for (u32 i = 0; i < ARRAY_COUNT(sSawyerIds); i++)
            if (id == sSawyerIds[i])
                expected = TRUE;
        for (u32 i = 0; i < ARRAY_COUNT(sSouthwestRosters); i++)
            if (id == sSouthwestRosters[i].trainerId)
                expected = TRUE;
        for (u32 i = 0; i < ARRAY_COUNT(sBatch2Rosters); i++)
            if (id == sBatch2Rosters[i].trainerId)
                expected = TRUE;
        EXPECT_EQ(GetLARosterProfile(id, DIFFICULTY_NORMAL) != NULL, expected);
        normalCount += GetLARosterProfile(id, DIFFICULTY_NORMAL) != NULL;
        EXPECT(GetLARosterProfile(id, DIFFICULTY_EASY) == NULL);
        EXPECT(GetLARosterProfile(id, DIFFICULTY_HARD) == NULL);
    }
    EXPECT_EQ(normalCount, 17);
}

TEST("Trainer Roster: Batch 2 seven profiles exact sources anchors and stable candidates")
{
    u32 candidates = 0;
    rng_value_t r1 = gRngValue, r2 = gRng2Value;
    for (u32 i = 0; i < ARRAY_COUNT(sBatch2Rosters); i++)
    {
        const struct SouthwestRosterCase *c = &sBatch2Rosters[i];
        const struct LARosterProfile *p = GetLARosterProfile(c->trainerId, DIFFICULTY_NORMAL);
        ASSUME(p != NULL);
        EXPECT_EQ(p->namespaceId, c->namespaceId);
        EXPECT_EQ(p->retainedCount, c->retainedCount);
        EXPECT_EQ(p->supplementCount, 6 - c->retainedCount);
        const struct Trainer trainer = {.party = c->authored, .partySize = c->retainedCount};
        struct TrainerMon before[4];
        memcpy(before, c->authored, sizeof(before));
        struct LARosterSelection selected = SelectLARoster(&trainer, p, 6);
        EXPECT_EQ(selected.status, LA_ROSTER_SELECTION_COMPLETE);
        EXPECT_EQ(selected.count, 6);
        u32 anchor = 0;
        for (u32 j = 0; j < selected.count; j++)
        {
            if (selected.members[j].source->lvl > anchor)
                anchor = selected.members[j].source->lvl;
            if (j < c->retainedCount)
            {
                EXPECT_EQ(p->retained[j].sourceIndex, j);
                EXPECT_EQ(p->retained[j].expectedSpecies, c->authored[j].species);
                EXPECT(selected.members[j].source == &c->authored[j]);
                EXPECT_EQ(selected.members[j].sourceKey, j);
            }
            else
            {
                u32 k = j - c->retainedCount;
                const struct LARosterSupplement *s = &p->supplements[k];
                EXPECT_EQ(s->candidateId, k + 1);
                EXPECT_EQ(s->mon.species, c->supplements[k]);
                EXPECT_EQ(s->mon.lvl, c->supplementLevel);
                EXPECT_EQ(selected.members[j].sourceKey, 0x80000000u | ((u32)c->namespaceId << 16) | (k + 1));
                struct TrainerMon expected = MON(c->supplements[k], c->supplementLevel);
                expected.ball = POKEBALL_COUNT;
                expected.nature = NATURE_HARDY;
                EXPECT_EQ(memcmp(&s->mon, &expected, sizeof(expected)), 0);
                candidates++;
            }
        }
        EXPECT_EQ(anchor, c->anchor);
        EXPECT_EQ(memcmp(before, c->authored, sizeof(before)), 0);
    }
    EXPECT_EQ(candidates, 21);
    EXPECT_EQ(memcmp(&r1, &gRngValue, sizeof(r1)), 0);
    EXPECT_EQ(memcmp(&r2, &gRng2Value, sizeof(r2)), 0);
}


TEST("Trainer Roster: Batch 2 stage arrays preserve ten family identities")
{
    u32 identities = 0;
    for (u32 i = 0; i < ARRAY_COUNT(sBatch2Rosters); i++)
    {
        const struct LARosterProfile *p = GetLARosterProfile(sBatch2Rosters[i].trainerId, DIFFICULTY_NORMAL);
        ASSUME(p != NULL);
        for (u32 j = 0; j < p->supplementCount; j++)
        {
            bool32 seen = FALSE;
            for (u32 k = 0; k < i; k++)
            {
                const struct LARosterProfile *previous = GetLARosterProfile(sBatch2Rosters[k].trainerId, DIFFICULTY_NORMAL);
                if (previous->namespaceId != p->namespaceId)
                    continue;
                EXPECT(previous->supplements != p->supplements);
                for (u32 m = 0; m < previous->supplementCount; m++)
                    if (previous->supplements[m].candidateId == p->supplements[j].candidateId)
                    {
                        EXPECT_EQ(previous->supplements[m].mon.species, p->supplements[j].mon.species);
                        seen = TRUE;
                    }
            }
            if (!seen)
                identities++;
        }
    }
    EXPECT_EQ(identities, 10);
    const struct LARosterProfile *j4 = GetLARosterProfile(TRAINER_JERRY_4, DIFFICULTY_NORMAL);
    const struct LARosterProfile *j5 = GetLARosterProfile(TRAINER_JERRY_5, DIFFICULTY_NORMAL);
    EXPECT_EQ(j4->supplementCount, 4);
    EXPECT_EQ(j4->supplements[3].candidateId, 4);
    EXPECT_EQ(j4->supplements[3].mon.species, SPECIES_BANETTE);
    EXPECT_EQ(j5->supplementCount, 3);
    for (u32 i = 0; i < j5->supplementCount; i++)
        EXPECT_EQ(j5->supplements[i].candidateId, i + 1);
}
