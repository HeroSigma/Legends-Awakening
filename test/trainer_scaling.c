#include "global.h"
#include "data.h"
#include "pokemon.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "la_trainer.h"
#include "trainer_scaling.h"
#include "test/test.h"

// --- Table-sanity helpers -------------------------------------------------

static void CheckTableCell(u8 rank, u8 phase, u8 expected)
{
    // Best-effort: exercise the exact function the runtime uses.
    EXPECT_EQ(CalculateTrainerScalingWorldLevel(rank, phase, 0, 0), expected);
}

// --- Player-party helpers ---------------------------------------------------

static void AddPartyMon(u8 slot, u16 species, u8 level)
{
    CreateMon(&gParties[B_TRAINER_PLAYER][slot], species, level, 0, OTID_STRUCT_PLAYER_ID);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][slot]);
}

static void MarkEgg(u8 slot)
{
    bool8 isEgg = TRUE;
    SetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_IS_EGG, &isEgg);
}

static void Faint(u8 slot)
{
    u32 hp = 0;
    SetMonData(&gParties[B_TRAINER_PLAYER][slot], MON_DATA_HP, &hp);
}

// --- Tests ------------------------------------------------------------------

TEST("Trainer Scaling: exact provisional table cells")
{
    // LA scaling clamps world level to a [6, MAX_LEVEL] floor; the table's
    // ROOKIE/BEGINNING cell is 5 but the function returns the 6 floor.
    CheckTableCell(TRAINER_RANK_ROOKIE, WORLD_PHASE_BEGINNING, 6);
    CheckTableCell(TRAINER_RANK_ROOKIE, WORLD_PHASE_LEGEND, 16);
    CheckTableCell(TRAINER_RANK_LEGEND, WORLD_PHASE_BEGINNING, 30);
    CheckTableCell(TRAINER_RANK_LEGEND, WORLD_PHASE_LEGEND, 80);
}

TEST("Trainer Scaling: table monotonic by rank and phase")
{
    u8 rank, phase;
    // All cells within intended nonzero range ([5, 80] full table pre-boost).
    for (rank = 0; rank < TRAINER_RANK_COUNT; rank++)
    {
        for (phase = 0; phase < WORLD_PHASE_COUNT; phase++)
        {
            u8 v = CalculateTrainerScalingWorldLevel(rank, phase, 0, 0);
            EXPECT(v >= 5 && v <= 80);
        }
    }
    // Monotonic in phase (each row non-decreasing).
    for (rank = 0; rank < TRAINER_RANK_COUNT; rank++)
    {
        for (phase = 1; phase < WORLD_PHASE_COUNT; phase++)
            EXPECT(CalculateTrainerScalingWorldLevel(rank, phase, 0, 0) >= CalculateTrainerScalingWorldLevel(rank, phase - 1, 0, 0));
    }
    // Monotonic in rank (each column non-decreasing).
    for (phase = 0; phase < WORLD_PHASE_COUNT; phase++)
    {
        for (rank = 1; rank < TRAINER_RANK_COUNT; rank++)
            EXPECT(CalculateTrainerScalingWorldLevel(rank, phase, 0, 0) >= CalculateTrainerScalingWorldLevel(rank - 1, phase, 0, 0));
    }
}

TEST("Trainer Scaling: world-level floor and ceiling")
{
    // Out-of-range rank/phase clamps to table bounds, never OOB.
    EXPECT(CalculateTrainerScalingWorldLevel(255, 255, 0, 0) >= 6);
    EXPECT(CalculateTrainerScalingWorldLevel(255, 255, 0, 0) <= MAX_LEVEL);
    // Player at max level cannot exceed MAX_LEVEL.
    EXPECT(CalculateTrainerScalingWorldLevel(TRAINER_RANK_LEGEND, WORLD_PHASE_LEGEND, 100, 6) <= MAX_LEVEL);
}

TEST("Trainer Scaling: stronger party can only increase world level")
{
    u8 rank, phase, base, boosted;
    for (rank = 0; rank < TRAINER_RANK_COUNT; rank++)
    {
        for (phase = 0; phase < WORLD_PHASE_COUNT; phase++)
        {
            base = CalculateTrainerScalingWorldLevel(rank, phase, 0, 0);
            boosted = CalculateTrainerScalingWorldLevel(rank, phase, MAX_LEVEL, 6);
            EXPECT(boosted >= base);
        }
    }
}

TEST("Trainer Scaling: MAX_PARTY_BOOST caps party influence at 15")
{
    // playerAvg far above base -> (avg-base)/3 capped at 15.
    u8 world = CalculateTrainerScalingWorldLevel(TRAINER_RANK_ROOKIE, WORLD_PHASE_BEGINNING, 100, 6);
    // base 5 + min(31,15) = 20.
    EXPECT_EQ(world, 20);
    // Even higher only clamps at 100 in aggregate but boost stays <= 15.
    EXPECT(CalculateTrainerScalingWorldLevel(TRAINER_RANK_LEGEND, WORLD_PHASE_LEGEND, 100, 6) <= 95);
}

TEST("Trainer Scaling: zero valid party mons yields avg 0, count 0")
{
    ZeroPlayerPartyMons();
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 0);
    EXPECT_EQ(s.usableCount, 0);
}

TEST("Trainer Scaling: one valid mon")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 42);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 42);
    EXPECT_EQ(s.usableCount, 1);
}

TEST("Trainer Scaling: two valid mons uses average")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 30);
    AddPartyMon(1, SPECIES_TREECKO, 50);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 40);  // floor((50+30)/2)
    EXPECT_EQ(s.usableCount, 2);
}

TEST("Trainer Scaling: three valid mons")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 10);
    AddPartyMon(1, SPECIES_TREECKO, 20);
    AddPartyMon(2, SPECIES_TREECKO, 30);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 20);  // floor((30+20+10)/3)
    EXPECT_EQ(s.usableCount, 3);
}

TEST("Trainer Scaling: 4-6 valid mons uses strongest three")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 10);
    AddPartyMon(1, SPECIES_TREECKO, 60);
    AddPartyMon(2, SPECIES_TREECKO, 20);
    AddPartyMon(3, SPECIES_TREECKO, 30);
    AddPartyMon(4, SPECIES_TREECKO, 50);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, (60 + 50 + 30) / 3);
    EXPECT_EQ(s.usableCount, 5);
}

TEST("Trainer Scaling: fainted high-level mon still counts")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 5);
    AddPartyMon(1, SPECIES_TREECKO, 100);
    Faint(1);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 52);   // floor((100+5)/2), faint NOT excluded
    EXPECT_EQ(s.usableCount, 2);
}

TEST("Trainer Scaling: eggs excluded")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 60);
    AddPartyMon(1, SPECIES_TREECKO, 99);
    MarkEgg(1);
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 60);
    EXPECT_EQ(s.usableCount, 1);
}

TEST("Trainer Scaling: empty slots excluded")
{
    ZeroPlayerPartyMons();
    AddPartyMon(0, SPECIES_TREECKO, 77);
    // Slots 1..5 remain SPECIES_NONE.
    struct LAPartyStrength s = CalculateTrainerPartyStrength();
    EXPECT_EQ(s.avgLevel, 77);
    EXPECT_EQ(s.usableCount, 1);
}

TEST("Trainer Scaling: ordinary category modifier is 0")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);  // ordinary
    EXPECT_EQ(GetLATrainerCategoryLevelModifier(p), 0);
}

TEST("Trainer Scaling: major category modifier is +3")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(1);  // rival -> major
    EXPECT_EQ(GetLATrainerCategoryLevelModifier(p), 3);
}

TEST("Trainer Scaling: ID modifier defaults to 0")
{
    EXPECT_EQ(GetLATrainerIdLevelModifier(TRAINER_SAWYER_1), 0);
    EXPECT_EQ(GetLATrainerIdLevelModifier(TRAINER_RED), 0);
    EXPECT_EQ(GetLATrainerIdLevelModifier(0xFFFF), 0);
}

TEST("Trainer Scaling: total modifier composes category + id")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(1);  // major
    // category +3, id 0 -> total 3.
    EXPECT_EQ(GetLATrainerTotalLevelModifier(1, p), 3);
    struct LATrainerPolicy o = GetLATrainerPolicy(0);  // ordinary
    EXPECT_EQ(GetLATrainerTotalLevelModifier(0, o), 0);
}

TEST("Trainer Scaling: no downscaling when desired below authored")
{
    // authoredAnchor 60, worldLevel 20 -> desired 20 < 60 -> delta 0.
    EXPECT_EQ(CalculateTrainerLevelDelta(60, 20, 0), 0);
    EXPECT_EQ(CalculateTrainerLevelDelta(60, 20, 3), 0);
}

TEST("Trainer Scaling: positive delta when desired above authored")
{
    // authoredAnchor 10, worldLevel 25, mod 2 -> desired 27 -> delta 17.
    EXPECT_EQ(CalculateTrainerLevelDelta(10, 25, 2), 17);
}

TEST("Trainer Scaling: apply delta preserves internal level gaps")
{
    u8 delta = CalculateTrainerLevelDelta(23, 27, 3);   // desired 30, anchor 23 -> +7
    EXPECT_EQ(delta, 7);
    EXPECT_EQ(ApplyTrainerLevelDelta(18, delta), 25);
    EXPECT_EQ(ApplyTrainerLevelDelta(18, delta), 25);
    EXPECT_EQ(ApplyTrainerLevelDelta(20, delta), 27);
    EXPECT_EQ(ApplyTrainerLevelDelta(21, delta), 28);
    EXPECT_EQ(ApplyTrainerLevelDelta(23, delta), 30);
}

TEST("Trainer Scaling: ace remains highest after delta")
{
    u8 delta = CalculateTrainerLevelDelta(23, 27, 3);
    EXPECT(ApplyTrainerLevelDelta(18, delta) < ApplyTrainerLevelDelta(23, delta));
}

TEST("Trainer Scaling: MAX_LEVEL clamp")
{
    // authoredAnchor 90, world 100 -> desired 100 -> delta 10 -> slot 90+10=100.
    u8 delta = CalculateTrainerLevelDelta(90, 100, 0);
    EXPECT_EQ(ApplyTrainerLevelDelta(90, delta), 100);
    EXPECT_EQ(ApplyTrainerLevelDelta(95, delta), 100);
    EXPECT(ApplyTrainerLevelDelta(95, delta) <= MAX_LEVEL);
}

TEST("Trainer Scaling: deterministic output")
{
    u8 a = CalculateTrainerScalingWorldLevel(TRAINER_RANK_ACE, WORLD_PHASE_RISING, 30, 3);
    u8 b = CalculateTrainerScalingWorldLevel(TRAINER_RANK_ACE, WORLD_PHASE_RISING, 30, 3);
    EXPECT_EQ(a, b);
    EXPECT_EQ(CalculateTrainerLevelDelta(20, 30, 0), CalculateTrainerLevelDelta(20, 30, 0));
}
