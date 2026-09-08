#include "global.h"
#include "constants/maps.h"
#include "pokemon.h"
#include "random.h"
#include "event_data.h"
#include "trainer_rank.h"
#include "world_state.h"
#include "constants/item.h"
#include "constants/vars.h"
#include "constants/dynamic_encounters.h"
#include "test/test.h"
#include "wild_scaling.h"

TEST("Wild Scaling: progression bands and top-three party average")
{
    static const u8 rookieParty[] = {5, 6, 7};
    static const u8 outlierParty[] = {50, 60, 70, 100};

    EXPECT_EQ(CalculateWildScalingWorldLevel(0, 0, NULL, 0), 1);
    EXPECT_EQ(CalculateWildScalingWorldLevel(1, 0, NULL, 0), 12);
    EXPECT_EQ(CalculateWildScalingWorldLevel(2, 0, NULL, 0), 24);
    EXPECT_EQ(CalculateWildScalingWorldLevel(3, 0, NULL, 0), 38);
    EXPECT_EQ(CalculateWildScalingWorldLevel(4, 0, NULL, 0), 52);
    EXPECT_EQ(CalculateWildScalingWorldLevel(5, 0, NULL, 0), 70);
    EXPECT_EQ(CalculateWildScalingWorldLevel(0, 0, rookieParty, ARRAY_COUNT(rookieParty)), 4);
    EXPECT_EQ(CalculateWildScalingWorldLevel(5, 0, outlierParty, ARRAY_COUNT(outlierParty)), 74);
}

TEST("Wild Scaling: rank and phase independently establish the tier")
{
    EXPECT_EQ(CalculateWildScalingWorldLevel(5, 0, NULL, 0), 70);
    EXPECT_EQ(CalculateWildScalingWorldLevel(0, 6, NULL, 0), 70);
}

TEST("Wild Scaling: eggs are excluded from the live party average")
{
    bool8 isEgg = TRUE;

    ZeroPlayerPartyMons();
    CreateMon(&gParties[B_TRAINER_PLAYER][0], SPECIES_TREECKO, 50, 0, OTID_STRUCT_PLAYER_ID);
    CreateMon(&gParties[B_TRAINER_PLAYER][1], SPECIES_TREECKO, 10, 0, OTID_STRUCT_PLAYER_ID);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][0]);
    CalculateMonStats(&gParties[B_TRAINER_PLAYER][1]);
    SetMonData(&gParties[B_TRAINER_PLAYER][1], MON_DATA_IS_EGG, &isEgg);
    EXPECT_EQ(GetWildScalingPartyAverage(), 50);
    ZeroPlayerPartyMons();
}

TEST("Wild Scaling: map modifiers and unconfigured maps")
{
    EXPECT_EQ(GetWildScalingMapModifier(MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101)), -2);
    EXPECT_EQ(GetWildScalingMapModifier(MAP_GROUP(MAP_ROUTE110), MAP_NUM(MAP_ROUTE110)), 0);
    EXPECT_EQ(GetWildScalingMapModifier(0xFFFF, 0xFFFF), 0);
}

TEST("Wild Scaling: level spread preserves slot identity and clamps")
{
    EXPECT_EQ(TestApplyWildLevelScaling(10, 10, 50, 0, 0), 38);
    EXPECT_EQ(TestApplyWildLevelScaling(30, 50, 50, 0, 0), 46);
    EXPECT_EQ(TestApplyWildLevelScaling(1, 1, 1, -100, 0), 1);
    EXPECT_EQ(TestApplyWildLevelScaling(100, 100, 100, 100, 4), 100);
    EXPECT_EQ(TestApplyWildLevelScaling(1, 1, 50, 0, 0), 36);
}

TEST("Wild Scaling: explicit evolution metadata retains safe families")
{
    u16 species;
    u8 i;
    bool32 sawBase = FALSE;
    bool32 sawMiddle = FALSE;
    bool32 sawFinal = FALSE;

    EXPECT_EQ(ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 10, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), TIME_DAY, 1), SPECIES_TREECKO);
    EXPECT_EQ(ResolveScaledWildSpecies(SPECIES_NONE, 100, 0, 0), SPECIES_NONE);
    EXPECT_EQ(ResolveScaledWildSpecies(SPECIES_EEVEE, 100, 0, 0), SPECIES_EEVEE);
    EXPECT_EQ(ResolveScaledWildSpecies(SPECIES_TAUROS, 100, 0, 0), SPECIES_TAUROS);
    EXPECT_EQ(ResolveScaledWildSpecies(SPECIES_WAILMER, 10, 0, 0), SPECIES_WAILMER);
    SeedRng(1);
    for (i = 0; i < 128; i++)
    {
        species = ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 25, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), TIME_DAY, 1);
        EXPECT(species == SPECIES_TREECKO || species == SPECIES_GROVYLE);
        sawBase |= species == SPECIES_TREECKO;
        sawMiddle |= species == SPECIES_GROVYLE;
    }
    EXPECT(sawBase);
    EXPECT(sawMiddle);
    sawBase = FALSE;
    sawMiddle = FALSE;
    SeedRng(2);
    for (i = 0; i < 128; i++)
    {
        species = ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 100, MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), TIME_DAY, 1);
        EXPECT(species == SPECIES_TREECKO || species == SPECIES_GROVYLE || species == SPECIES_SCEPTILE);
        sawBase |= species == SPECIES_TREECKO;
        sawMiddle |= species == SPECIES_GROVYLE;
        sawFinal |= species == SPECIES_SCEPTILE;
    }
    EXPECT(sawBase);
    EXPECT(sawMiddle);
    EXPECT(sawFinal);
}

// Exercise the real chooser -> scaling -> creation path. Hidden encounters select
// slot zero without a slot-selection RNG draw, allowing exact seeded replay.
TEST("Wild Scaling: shared generator preserves lure and level ability choices")
{
    static const struct WildPokemon slots[NUM_LAND_MONS_ENCOUNTER_SLOTS] =
    {
        [0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {10, 50, SPECIES_TAUROS},
    };
    static const struct WildPokemonInfo info = {20, slots};
    static const struct { u16 species; u8 abilityNum; u16 ability; } leads[] =
    {
        {SPECIES_DUSCLOPS, 0, ABILITY_PRESSURE},
        {SPECIES_DELIBIRD, 0, ABILITY_VITAL_SPIRIT},
        {SPECIES_DELIBIRD, 1, ABILITY_HUSTLE},
    };
    u32 lead, seed, lure;
    u16 original, expected, actual;
    u16 sawMax, sawBelowMax;
    u16 savedRepel = VarGet(VAR_REPEL_STEP_COUNT);
    u8 savedRank = GetTrainerRank();
    u16 savedPhase = GetWorldPhase();
    u16 savedLayout = gMapHeader.mapLayoutId;

    EXPECT(SetTrainerRank(TRAINER_RANK_MASTER));
    EXPECT(SetWorldPhase(WORLD_PHASE_BEGINNING));
    gMapHeader.mapLayoutId = 0;
    for (lead = 0; lead < ARRAY_COUNT(leads); lead++)
    {
        ZeroPlayerPartyMons();
        CreateMon(&gParties[B_TRAINER_PLAYER][0], leads[lead].species, 60, 0, OTID_STRUCT_PLAYER_ID);
        CalculateMonStats(&gParties[B_TRAINER_PLAYER][0]);
        SetMonData(&gParties[B_TRAINER_PLAYER][0], MON_DATA_ABILITY_NUM, &leads[lead].abilityNum);
        EXPECT_EQ(GetMonAbility(&gParties[B_TRAINER_PLAYER][0]), leads[lead].ability);
        for (lure = 0; lure < 2; lure++)
        {
            VarSet(VAR_REPEL_STEP_COUNT, lure ? REPEL_LURE_MASK | 100 : 0);
            sawMax = sawBelowMax = 0;
            for (seed = 1; seed <= 32; seed++)
            {
                SeedRng(seed);
                original = ChooseWildMonLevel(slots, 0, WILD_AREA_HIDDEN);
                if (lure)
                    EXPECT_EQ(original, 51);
                else
                {
                    sawMax += original == 50;
                    sawBelowMax += original < 50;
                }
                expected = ApplyWildLevelScalingWithBaseLevel(SPECIES_TAUROS, original, 10, 50,
                    gSaveBlock1Ptr->location.mapGroup, gSaveBlock1Ptr->location.mapNum);
                SeedRng(seed);
                EXPECT(TryGenerateWildMon(&info, WILD_AREA_HIDDEN, 0));
                actual = GetMonData(&gParties[B_TRAINER_OPPONENT_A][0], MON_DATA_LEVEL);
                EXPECT_EQ(actual, expected);
            }
            if (!lure)
            {
                EXPECT(sawMax > 0);
                EXPECT(sawBelowMax > 0);
            }
        }
    }
    VarSet(VAR_REPEL_STEP_COUNT, savedRepel);
    EXPECT(SetTrainerRank(savedRank));
    EXPECT(SetWorldPhase(savedPhase));
    gMapHeader.mapLayoutId = savedLayout;
    ZeroPlayerPartyMons();
}

TEST("Wild Scaling: development contexts retain all stages in both profiles")
{
    static const struct { u16 profile; enum TimeOfDay time; u16 species; } contexts[] =
    {
        {DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1, TIME_DAY, SPECIES_TREECKO},
        {DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2, TIME_MORNING, SPECIES_TREECKO},
        {DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2, TIME_DAY, SPECIES_GROVYLE},
    };
    u32 context, i;
    u16 species;
    bool32 sawBase, sawMiddle, sawFinal;
    for (context = 0; context < ARRAY_COUNT(contexts); context++)
    {
        sawBase = sawMiddle = sawFinal = FALSE;
        SeedRng(3);
        for (i = 0; i < 128; i++)
        {
            species = ResolveScaledWildSpeciesForContext(contexts[context].species, 100,
                MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), contexts[context].time, contexts[context].profile);
            EXPECT(species == SPECIES_TREECKO || species == SPECIES_GROVYLE || species == SPECIES_SCEPTILE);
            sawBase |= species == SPECIES_TREECKO;
            sawMiddle |= species == SPECIES_GROVYLE;
            sawFinal |= species == SPECIES_SCEPTILE;
        }
        EXPECT(sawBase);
        EXPECT(sawMiddle);
        EXPECT(sawFinal);
    }
}

TEST("Wild Scaling: map time and profile mismatches leave the species unchanged")
{
    EXPECT_EQ(ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 100,
        MAP_GROUP(MAP_ROUTE110), MAP_NUM(MAP_ROUTE110), TIME_DAY, DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1), SPECIES_TREECKO);
    EXPECT_EQ(ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 100,
        MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), TIME_NIGHT, DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1), SPECIES_TREECKO);
    EXPECT_EQ(ResolveScaledWildSpeciesForContext(SPECIES_TREECKO, 100,
        MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), TIME_DAY, DYNAMIC_ENCOUNTER_PROFILE_DEFAULT), SPECIES_TREECKO);
}
