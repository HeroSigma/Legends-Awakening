#include "global.h"
#include "pokemon.h"
#include "battle.h"
#include "battle_setup.h"
#include "debug.h"
#include "trainer_util.h"
#include "trainer_pools.h"
#include "trainer_evolution.h"
#include "trainer_scaling.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "test/test.h"

extern void TestCreateLATrainerParty(struct Pokemon *, const struct Trainer *, u16);
extern void TestCreateLATrainerPartyWithPolicy(struct Pokemon *, const struct Trainer *, struct LATrainerPolicy);
extern enum DifficultyLevel TestLATrainerConstructionDifficulty(u16 trainerId);
extern bool32 TestApplyTrainerEvolutionWithAbilities(struct TrainerMon *, enum Species, const enum Ability *);
extern const struct TrainerEvolutionProfile *TestFindTrainerEvolutionProfile(
    const struct TrainerEvolutionAssignment *, u32, u16, enum DifficultyLevel, u32, enum Species);

static struct TrainerMon Entry(enum Species species, u8 level)
{
    return (struct TrainerMon){.species = species, .lvl = level, .gender = TRAINER_MON_RANDOM_GENDER};
}

TEST("Trainer Evolution: single stage unchanged")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_PINSIR, 100, NULL), SPECIES_PINSIR);
}

TEST("Trainer Evolution: Shinx below threshold")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 14, NULL), SPECIES_SHINX);
}

TEST("Trainer Evolution: Shinx exactly at threshold")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 15, NULL), SPECIES_LUXIO);
}

TEST("Trainer Evolution: Shinx middle stage")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 20, NULL), SPECIES_LUXIO);
}

TEST("Trainer Evolution: Shinx final stage")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 30, NULL), SPECIES_LUXRAY);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 100, NULL), SPECIES_LUXRAY);
}

TEST("Trainer Evolution: authored middle never devolves")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_LUXIO, 1, NULL), SPECIES_LUXIO);
}

TEST("Trainer Evolution: authored middle advances")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_LUXIO, 30, NULL), SPECIES_LUXRAY);
}

TEST("Trainer Evolution: authored final unchanged")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_LUXRAY, 1, NULL), SPECIES_LUXRAY);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_LUXRAY, 100, NULL), SPECIES_LUXRAY);
}

TEST("Trainer Evolution: deterministic and monotonic across every level")
{
    for (u32 level = MIN_LEVEL; level <= MAX_LEVEL; level++)
    {
        enum Species expected = level < 15 ? SPECIES_SHINX : level < 30 ? SPECIES_LUXIO : SPECIES_LUXRAY;
        EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, level, NULL), expected);
        EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, level, NULL), expected);
    }
}

TEST("Trainer Evolution: safe Ralts prefix survives later ambiguity")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 19, NULL), SPECIES_RALTS);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 20, NULL), SPECIES_KIRLIA);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 100, NULL), SPECIES_KIRLIA);
}

TEST("Trainer Evolution: current branching nodes never choose automatically")
{
    const enum Species species[] = {SPECIES_EEVEE, SPECIES_KIRLIA, SPECIES_WURMPLE,
        SPECIES_SLOWPOKE, SPECIES_TYROGUE, SPECIES_POLIWHIRL, SPECIES_CLAMPERL,
        SPECIES_APPLIN, SPECIES_CHARCADET, SPECIES_ROCKRUFF};
    for (u32 i = 0; i < ARRAY_COUNT(species); i++)
        EXPECT_EQ(ResolveTrainerScaledSpecies(species[i], 100, NULL), species[i]);
}

TEST("Trainer Evolution: explicit canonical branch keeps safe prefix")
{
    const struct TrainerEvolutionStep steps[] = {
        {SPECIES_KIRLIA, SPECIES_GARDEVOIR, TRAINER_EVO_CANONICAL_LEVEL, 0},
    };
    const struct TrainerEvolutionProfile profile = {steps, ARRAY_COUNT(steps), FALSE};
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 29, &profile), SPECIES_KIRLIA);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 30, &profile), SPECIES_GARDEVOIR);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_RALTS, 100, &profile), SPECIES_GARDEVOIR);
}

TEST("Trainer Evolution: explicit Eevee branch threshold is deterministic")
{
    // Test-only threshold, deliberately not a production balance decision.
    const struct TrainerEvolutionStep step = {SPECIES_EEVEE, SPECIES_JOLTEON, TRAINER_EVO_EXPLICIT_LEVEL, 25};
    const struct TrainerEvolutionProfile profile = {&step, 1, FALSE};
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_EEVEE, 24, &profile), SPECIES_EEVEE);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_EEVEE, 25, &profile), SPECIES_JOLTEON);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_EEVEE, 100, &profile), SPECIES_JOLTEON);
}

TEST("Trainer Evolution: duplicate methods to one target accept explicit threshold")
{
    const struct TrainerEvolutionStep step = {SPECIES_KADABRA, SPECIES_ALAKAZAM, TRAINER_EVO_EXPLICIT_LEVEL, 40};
    const struct TrainerEvolutionProfile profile = {&step, 1, FALSE};
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_KADABRA, 39, &profile), SPECIES_KADABRA);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_KADABRA, 40, &profile), SPECIES_ALAKAZAM);
}

TEST("Trainer Evolution: absent non-level metadata preserves species")
{
    const enum Species species[] = {SPECIES_KADABRA, SPECIES_MACHOKE, SPECIES_HAUNTER, SPECIES_ROSELIA, SPECIES_ONIX};
    for (u32 i = 0; i < ARRAY_COUNT(species); i++)
        EXPECT_EQ(ResolveTrainerScaledSpecies(species[i], 100, NULL), species[i]);
}

TEST("Trainer Evolution: conditional level zero is not an immediate evolution")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_BUNEARY, 100, NULL), SPECIES_BUNEARY);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_PANCHAM, 100, NULL), SPECIES_PANCHAM);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_COMBEE, 100, NULL), SPECIES_COMBEE);
}

TEST("Trainer Evolution: malformed profiles cannot invent backwards edges")
{
    const struct TrainerEvolutionStep steps[] = {
        {SPECIES_SHINX, SPECIES_LUXIO, TRAINER_EVO_CANONICAL_LEVEL, 0},
        {SPECIES_LUXIO, SPECIES_SHINX, TRAINER_EVO_EXPLICIT_LEVEL, 20},
    };
    const struct TrainerEvolutionProfile profile = {steps, ARRAY_COUNT(steps), FALSE};
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 100, &profile), SPECIES_LUXIO);
}

TEST("Trainer Evolution: malformed and preserve profiles stop safely")
{
    const struct TrainerEvolutionProfile malformed = {NULL, 1, FALSE};
    const struct TrainerEvolutionProfile preserve = {NULL, 0, TRUE};
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 100, &malformed), SPECIES_SHINX);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 100, &preserve), SPECIES_SHINX);
}

TEST("Trainer Evolution: invalid species and levels are safe")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_NONE, 100, NULL), SPECIES_NONE);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_EGG, 100, NULL), SPECIES_EGG);
    EXPECT_EQ(ResolveTrainerScaledSpecies(0xFFFF, 100, NULL), 0xFFFF);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 0, NULL), SPECIES_SHINX);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_SHINX, 255, NULL), SPECIES_SHINX);
}

TEST("Trainer Evolution: restricted species and alternate forms unchanged")
{
    const enum Species species[] = {SPECIES_COSMOG, SPECIES_COSMOEM, SPECIES_MEW,
        SPECIES_POIPOLE, SPECIES_IRON_VALIANT, SPECIES_ARTICUNO, SPECIES_RAICHU_ALOLA,
        SPECIES_CHARIZARD_MEGA_X, SPECIES_ROCKRUFF_OWN_TEMPO, SPECIES_PALAFIN_ZERO,
        SPECIES_CHERRIM_OVERCAST, SPECIES_KYUREM_BLACK};
    for (u32 i = 0; i < ARRAY_COUNT(species); i++)
        EXPECT_EQ(ResolveTrainerScaledSpecies(species[i], 100, NULL), species[i]);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_FINIZEN, 100, NULL), SPECIES_FINIZEN);
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_CHERUBI, 100, NULL), SPECIES_CHERUBI);
}

TEST("Trainer Evolution: split evolution is never simulated")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_NINCADA, 100, NULL), SPECIES_NINCADA);
}

TEST("Trainer Evolution: ordinary species with Mega forms are not blanket excluded")
{
    EXPECT_EQ(ResolveTrainerScaledSpecies(SPECIES_CHARMANDER, 100, NULL), SPECIES_CHARIZARD);
}

TEST("Trainer Evolution: final scaled level drives transformation and is preserved")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 8);
    mon.lvl = ApplyTrainerLevelDelta(mon.lvl, CalculateTrainerLevelDelta(8, 20, 0));
    EXPECT(ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_LUXIO);
    EXPECT_EQ(mon.lvl, 20);
}

TEST("Trainer Evolution: zero delta and evolution-only use authored level")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 30);
    EXPECT_EQ(CalculateTrainerLevelDelta(30, 6, 0), 0);
    EXPECT(ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_LUXRAY);
    EXPECT_EQ(mon.lvl, 30);
}

TEST("Trainer Evolution: source immutable and authored fields retained")
{
    const struct TrainerMon source = {.species = SPECIES_SHINX, .lvl = 35,
        .gender = TRAINER_MON_FEMALE, .moves = {MOVE_SURF, MOVE_NONE, MOVE_TACKLE, MOVE_NONE},
        .ability = ABILITY_INTIMIDATE, .heldItem = ITEM_LEFTOVERS, .friendship = 123,
        .iv = 123, .nature = NATURE_ADAMANT, .tags = 17, .isShiny = TRUE,
        .teraType = TYPE_WATER, .shouldUseDynamax = TRUE, .dynamaxLevel = 5};
    struct TrainerMon working = source;
    EXPECT(ApplyTrainerEvolution(&working, NULL));
    EXPECT_EQ(source.species, SPECIES_SHINX);
    EXPECT_EQ(working.species, SPECIES_LUXRAY);
    working.species = source.species;
    EXPECT_EQ(memcmp(&source, &working, sizeof(source)), 0);
}

TEST("Trainer Evolution: normal and hidden ability slots survive each step")
{
    const enum Ability abilities[] = {ABILITY_RIVALRY, ABILITY_INTIMIDATE, ABILITY_GUTS};
    for (u32 i = 0; i < ARRAY_COUNT(abilities); i++)
    {
        struct TrainerMon mon = Entry(SPECIES_SHINX, 40);
        mon.ability = abilities[i];
        EXPECT(ApplyTrainerEvolution(&mon, NULL));
        EXPECT_EQ(mon.species, SPECIES_LUXRAY);
        EXPECT_EQ(mon.ability, gSpeciesInfo[SPECIES_LUXRAY].abilities[i]);
    }
}

TEST("Trainer Evolution: ability ID changes while slot intent survives")
{
    struct TrainerMon mon = Entry(SPECIES_CATERPIE, 10);
    mon.ability = ABILITY_SHIELD_DUST;
    EXPECT(ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_BUTTERFREE);
    EXPECT_EQ(mon.ability, ABILITY_COMPOUND_EYES);
}

TEST("Trainer Evolution: missing hidden ability retains safe prefix")
{
    // Weedle's HA cannot be represented by Kakuna; no downgrade is allowed.
    struct TrainerMon mon = Entry(SPECIES_WEEDLE, 100);
    mon.ability = ABILITY_RUN_AWAY;
    EXPECT(!ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_WEEDLE);
    EXPECT_EQ(mon.ability, ABILITY_RUN_AWAY);
}

TEST("Trainer Evolution: unspecified ability remains unspecified")
{
    struct TrainerMon mon = Entry(SPECIES_CATERPIE, 10);
    EXPECT(ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.ability, ABILITY_NONE);
}

TEST("Trainer Evolution: incompatible fixed gender rejects edge")
{
    const struct TrainerEvolutionStep step = {SPECIES_KIRLIA, SPECIES_GALLADE, TRAINER_EVO_EXPLICIT_LEVEL, 30};
    const struct TrainerEvolutionProfile profile = {&step, 1, FALSE};
    struct TrainerMon mon = Entry(SPECIES_RALTS, 40);
    mon.gender = TRAINER_MON_FEMALE;
    EXPECT(ApplyTrainerEvolution(&mon, &profile));
    EXPECT_EQ(mon.species, SPECIES_KIRLIA);
    EXPECT(mon.gender == TRAINER_MON_FEMALE);
}

TEST("Trainer Evolution: random gender never guesses a conditional branch")
{
    const struct TrainerEvolutionStep step = {SPECIES_KIRLIA, SPECIES_GALLADE, TRAINER_EVO_EXPLICIT_LEVEL, 30};
    const struct TrainerEvolutionProfile profile = {&step, 1, FALSE};
    struct TrainerMon mon = Entry(SPECIES_KIRLIA, 40);
    EXPECT(!ApplyTrainerEvolution(&mon, &profile));
}

TEST("Trainer Evolution: Everstone veto")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 100);
    mon.heldItem = ITEM_EVERSTONE;
    EXPECT(!ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_SHINX);
}

TEST("Trainer Evolution: Eviolite veto")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 100);
    mon.heldItem = ITEM_EVIOLITE;
    EXPECT(!ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.heldItem, ITEM_EVIOLITE);
}

TEST("Trainer Evolution: Gigantamax factor veto")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 100);
    mon.gigantamaxFactor = TRUE;
    EXPECT(!ApplyTrainerEvolution(&mon, NULL));
    EXPECT(mon.gigantamaxFactor);
}

static void Generate(struct Pokemon *mon, const struct TrainerMon *entry)
{
    const struct Trainer trainer = {.trainerClass = TRAINER_CLASS_PKMN_TRAINER_1};
    struct TrainerGenerator gen = {0};
    MakeTrainerGenerator(&gen, &trainer);
    GenerateMonFromTrainerMon(mon, entry, &gen);
}

TEST("Trainer Evolution: generator preserves custom moves including empty slots")
{
    struct TrainerMon entry = Entry(SPECIES_SHINX, 35);
    struct Pokemon mon;
    entry.moves[0] = MOVE_SURF; // Intentional non-learnable authored move.
    entry.moves[2] = MOVE_TACKLE;
    EXPECT(ApplyTrainerEvolution(&entry, NULL));
    Generate(&mon, &entry);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        EXPECT_EQ(GetMonData(&mon, MON_DATA_MOVE1 + i), entry.moves[i]);
}

TEST("Trainer Evolution: fallback is evolved species moves at final level")
{
    struct TrainerMon entry = Entry(SPECIES_SHINX, 35);
    struct TrainerMon expectedEntry = Entry(SPECIES_LUXRAY, 35);
    struct Pokemon actual, expected;
    EXPECT(ApplyTrainerEvolution(&entry, NULL));
    Generate(&actual, &entry);
    Generate(&expected, &expectedEntry);
    EXPECT_EQ(GetMonData(&actual, MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&actual, MON_DATA_LEVEL), 35);
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
        EXPECT_EQ(GetMonData(&actual, MON_DATA_MOVE1 + i), GetMonData(&expected, MON_DATA_MOVE1 + i));
}

TEST("Trainer Evolution: profile lookup checks all four ownership fields")
{
    const struct TrainerEvolutionProfile profile = {.preserveSpecies = TRUE};
    const struct TrainerEvolutionAssignment row = {3, DIFFICULTY_HARD, 4, SPECIES_EEVEE, &profile};
    EXPECT(TestFindTrainerEvolutionProfile(&row, 1, 3, DIFFICULTY_HARD, 4, SPECIES_EEVEE) == &profile);
    EXPECT(TestFindTrainerEvolutionProfile(&row, 1, 2, DIFFICULTY_HARD, 4, SPECIES_EEVEE) == NULL);
    EXPECT(TestFindTrainerEvolutionProfile(&row, 1, 3, DIFFICULTY_NORMAL, 4, SPECIES_EEVEE) == NULL);
    EXPECT(TestFindTrainerEvolutionProfile(&row, 1, 3, DIFFICULTY_HARD, 0, SPECIES_EEVEE) == NULL);
    EXPECT(TestFindTrainerEvolutionProfile(&row, 1, 3, DIFFICULTY_HARD, 4, SPECIES_SHINX) == NULL);
    EXPECT(GetTrainerEvolutionProfile(3, DIFFICULTY_HARD, 4, SPECIES_EEVEE) == NULL);
}

static void SetupConstruction(void)
{
    gIsDebugBattle = FALSE;
    gBattleTypeFlags = BATTLE_TYPE_TRAINER;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    ZeroPlayerPartyMons();
    SetTrainerRank(TRAINER_RANK_ROOKIE);
    SetWorldPhase(WORLD_PHASE_BEGINNING);
}

TEST("Trainer Evolution: real construction evolves ordinary at zero delta")
{
    SetupConstruction();
    const struct TrainerMon entries[] = {{.species = SPECIES_SHINX, .lvl = 30, .gender = TRAINER_MON_RANDOM_GENDER}};
    const struct Trainer trainer = {.party = entries, .partySize = 1};
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 30);
    EXPECT_EQ(entries[0].species, SPECIES_SHINX);
}

TEST("Trainer Evolution: real construction scales major but preserves species")
{
    SetupConstruction();
    SetTrainerRank(TRAINER_RANK_LEGEND);
    SetWorldPhase(WORLD_PHASE_LEGEND);
    const struct TrainerMon entries[] = {{.species = SPECIES_SHINX, .lvl = 8, .gender = TRAINER_MON_RANDOM_GENDER}};
    const struct Trainer trainer = {.party = entries, .partySize = 1};
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 1);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_SHINX);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 83);
}

TEST("Trainer Evolution: special exempt debug and public no-ID paths bypass")
{
    SetupConstruction();
    const struct TrainerMon entries[] = {{.species = SPECIES_SHINX, .lvl = 30, .gender = TRAINER_MON_RANDOM_GENDER}};
    const struct Trainer trainer = {.party = entries, .partySize = 1};
    const u16 ids[] = {TRAINER_PARTNER(1), 0xFFFF, TRAINER_SECRET_BASE};
    for (u32 i = 0; i < ARRAY_COUNT(ids); i++)
    {
        TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, ids[i]);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_SHINX);
    }
    gIsDebugBattle = TRUE;
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_SHINX);
    gIsDebugBattle = FALSE;
    CreateNPCTrainerPartyFromTrainer(gParties[B_TRAINER_OPPONENT_A], &trainer);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_SHINX);
}

TEST("Trainer Evolution: selected pool subset determines anchor and evolves alone")
{
    SetupConstruction();
    SetTrainerRank(TRAINER_RANK_LEGEND);
    const struct TrainerMon entries[] = {
        {.species = SPECIES_SHINX, .lvl = 8, .gender = TRAINER_MON_RANDOM_GENDER},
        {.species = SPECIES_PINSIR, .lvl = 100, .gender = TRAINER_MON_RANDOM_GENDER},
    };
    const struct Trainer trainer = {.party = entries, .partySize = 1, .poolSize = 2, .poolPickIndex = POOL_PICK_LOWEST};
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 30);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][1], MON_DATA_SPECIES), SPECIES_NONE);
    EXPECT_EQ(entries[0].species, SPECIES_SHINX);
    EXPECT_EQ(entries[1].lvl, 100);
}

TEST("Trainer Evolution: two opponent half teams retain destination limits")
{
    SetupConstruction();
    gBattleTypeFlags |= BATTLE_TYPE_TWO_OPPONENTS;
    const struct TrainerMon entries[] = {
        {.species = SPECIES_SHINX, .lvl = 30, .gender = TRAINER_MON_RANDOM_GENDER},
        {.species = SPECIES_SHINX, .lvl = 30, .gender = TRAINER_MON_RANDOM_GENDER},
        {.species = SPECIES_SHINX, .lvl = 30, .gender = TRAINER_MON_RANDOM_GENDER},
        {.species = SPECIES_SHINX, .lvl = 100, .gender = TRAINER_MON_RANDOM_GENDER},
    };
    const struct Trainer trainer = {.party = entries, .partySize = 4, .multiTeamSize = MULTI_TEAM_SIZE_HALF};
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0);
    for (u32 i = 0; i < PARTY_SIZE / 2; i++)
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][i], MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][PARTY_SIZE / 2], MON_DATA_SPECIES), SPECIES_NONE);
}

TEST("Trainer Evolution: actual construction keeps level and evolution permissions independent")
{
    SetupConstruction();
    SetTrainerRank(TRAINER_RANK_LEGEND);
    SetWorldPhase(WORLD_PHASE_LEGEND);
    const struct TrainerMon entries[] = {{.species = SPECIES_SHINX, .lvl = 20, .gender = TRAINER_MON_RANDOM_GENDER}};
    const struct Trainer trainer = {.party = entries, .partySize = 1};
    const struct LATrainerPolicy policies[] = {
        {LA_TRAINER_ORDINARY, 0},
        {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_SCALE_LEVEL},
        {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_SCALE_EVOLUTION},
        {LA_TRAINER_ORDINARY, LA_TRAINER_POLICY_SCALE_LEVEL | LA_TRAINER_POLICY_SCALE_EVOLUTION},
    };
    const enum Species species[] = {SPECIES_SHINX, SPECIES_SHINX, SPECIES_LUXIO, SPECIES_LUXRAY};
    const u8 levels[] = {20, 80, 20, 80};
    for (u32 i = 0; i < ARRAY_COUNT(policies); i++)
    {
        TestCreateLATrainerPartyWithPolicy(gParties[B_TRAINER_OPPONENT_A], &trainer, policies[i]);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), species[i]);
        EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), levels[i]);
    }
}

TEST("Trainer Evolution: shuffled pool selects nonzero source and ignores unused anchor")
{
    SetupConstruction();
    SetTrainerRank(TRAINER_RANK_LEGEND);
    const struct TrainerMon entries[] = {
        {.species = SPECIES_PINSIR, .lvl = 100, .gender = TRAINER_MON_RANDOM_GENDER},
        {.species = SPECIES_SHINX, .lvl = 8, .gender = TRAINER_MON_RANDOM_GENDER, .tags = MON_POOL_TAG_LEAD},
    };
    const struct Trainer trainer = {.party = entries, .partySize = 1, .poolSize = 2};
    TestCreateLATrainerParty(gParties[B_TRAINER_OPPONENT_A], &trainer, 0);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_SPECIES), SPECIES_LUXRAY);
    EXPECT_EQ(GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL), 30);
    EXPECT_EQ(entries[1].species, SPECIES_SHINX);
    EXPECT_EQ(entries[0].lvl, 100);
}

TEST("Trainer Evolution: effective difficulty falls back and invalid IDs are guarded")
{
    SetupConstruction();
    SetCurrentDifficultyLevel(DIFFICULTY_HARD);
    EXPECT_EQ(TestLATrainerConstructionDifficulty(0), DIFFICULTY_NORMAL);
    EXPECT_EQ(TestLATrainerConstructionDifficulty(5), DIFFICULTY_HARD);
    EXPECT_EQ(TestLATrainerConstructionDifficulty(0xFFFF), DIFFICULTY_NORMAL);
    EXPECT_EQ(TestLATrainerConstructionDifficulty(TRAINER_PARTNER(1)), DIFFICULTY_NORMAL);
    EXPECT_EQ(TestLATrainerConstructionDifficulty(TRAINER_SECRET_BASE), DIFFICULTY_NORMAL);
    gIsDebugBattle = TRUE;
    EXPECT_EQ(TestLATrainerConstructionDifficulty(0), DIFFICULTY_NORMAL);
    gIsDebugBattle = FALSE;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
}

TEST("Trainer Evolution: duplicate ability IDs reject source and target ambiguity")
{
    struct TrainerMon source = Entry(SPECIES_VIBRAVA, 100);
    source.ability = ABILITY_LEVITATE;
    EXPECT(!ApplyTrainerEvolution(&source, NULL));
    EXPECT_EQ(source.species, SPECIES_VIBRAVA);
    struct TrainerMon target = Entry(SPECIES_TRAPINCH, 100);
    target.ability = ABILITY_HYPER_CUTTER;
    EXPECT(!ApplyTrainerEvolution(&target, NULL));
    EXPECT_EQ(target.species, SPECIES_TRAPINCH);
}

TEST("Trainer Evolution: invalid explicit ability never reaches generator")
{
    struct TrainerMon mon = Entry(SPECIES_SHINX, 100);
    mon.ability = ABILITY_LEVITATE;
    EXPECT(!ApplyTrainerEvolution(&mon, NULL));
    EXPECT_EQ(mon.species, SPECIES_SHINX);
    EXPECT_EQ(mon.ability, ABILITY_LEVITATE);
}

TEST("Trainer Evolution: later unavailable ability retains last accepted stage")
{
    // Inject a missing final-stage HA without changing canonical species data.
    const enum Ability abilities[] = {ABILITY_RIVALRY, ABILITY_INTIMIDATE, ABILITY_NONE};
    struct TrainerMon mon = Entry(SPECIES_SHINX, 100);
    mon.ability = ABILITY_GUTS;
    EXPECT(TestApplyTrainerEvolutionWithAbilities(&mon, SPECIES_LUXRAY, abilities));
    EXPECT_EQ(mon.species, SPECIES_LUXIO);
    EXPECT_EQ(mon.ability, ABILITY_GUTS);
    EXPECT_EQ(mon.lvl, 100);
}
