// DEVELOPMENT ONLY: Route 101 visibility tests; no canonical placements.
// Full encounter-type replacements with intentionally distinct time-of-day test pools.
static const struct WildPokemon sDevelopmentLand1[TIMES_OF_DAY_COUNT][NUM_LAND_MONS_ENCOUNTER_SLOTS] =
{
    [TIME_MORNING] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_SABLEYE}},
    [TIME_DAY] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_TREECKO}},
    [TIME_EVENING] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_MAGIKARP}},
    [TIME_NIGHT] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_WAILMER}},
};
static const struct WildPokemon sDevelopmentLand2[TIMES_OF_DAY_COUNT][NUM_LAND_MONS_ENCOUNTER_SLOTS] =
{
    [TIME_MORNING] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_TREECKO}},
    [TIME_DAY] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_GROVYLE}},
    [TIME_EVENING] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_MAGIKARP}},
    [TIME_NIGHT] = {[0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_WAILMER}},
};
static const struct WildPokemonInfo sDevelopmentInfo1[TIMES_OF_DAY_COUNT] =
{
    [TIME_MORNING] = {20, sDevelopmentLand1[TIME_MORNING]},
    [TIME_DAY] = {20, sDevelopmentLand1[TIME_DAY]},
    [TIME_EVENING] = {20, sDevelopmentLand1[TIME_EVENING]},
    [TIME_NIGHT] = {20, sDevelopmentLand1[TIME_NIGHT]},
};
static const struct WildPokemonInfo sDevelopmentInfo2[TIMES_OF_DAY_COUNT] =
{
    [TIME_MORNING] = {20, sDevelopmentLand2[TIME_MORNING]},
    [TIME_DAY] = {20, sDevelopmentLand2[TIME_DAY]},
    [TIME_EVENING] = {20, sDevelopmentLand2[TIME_EVENING]},
    [TIME_NIGHT] = {20, sDevelopmentLand2[TIME_NIGHT]},
};
static const struct WildEncounterTypes sDevelopmentTypes1[TIMES_OF_DAY_COUNT] =
{
    [TIME_MORNING] = {.landMonsInfo = &sDevelopmentInfo1[TIME_MORNING]},
    [TIME_DAY] = {.landMonsInfo = &sDevelopmentInfo1[TIME_DAY]},
    [TIME_EVENING] = {.landMonsInfo = &sDevelopmentInfo1[TIME_EVENING]},
    [TIME_NIGHT] = {.landMonsInfo = &sDevelopmentInfo1[TIME_NIGHT]},
};
static const struct WildEncounterTypes sDevelopmentTypes2[TIMES_OF_DAY_COUNT] =
{
    [TIME_MORNING] = {.landMonsInfo = &sDevelopmentInfo2[TIME_MORNING]},
    [TIME_DAY] = {.landMonsInfo = &sDevelopmentInfo2[TIME_DAY]},
    [TIME_EVENING] = {.landMonsInfo = &sDevelopmentInfo2[TIME_EVENING]},
    [TIME_NIGHT] = {.landMonsInfo = &sDevelopmentInfo2[TIME_NIGHT]},
};
static const struct WildEncounterTypes *const sProfiles[DYNAMIC_ENCOUNTER_PROFILE_COUNT] =
{
    [DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1] = sDevelopmentTypes1,
    [DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2] = sDevelopmentTypes2,
};
// First matching rule wins. More specific/higher thresholds go first.
static const struct DynamicEncounterRule sRoute101Rules[] =
{
    {WORLD_REGION_HOENN, 2, 0xFFFF, WORLD_PHASE_BEGINNING, DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_2},
    {WORLD_REGION_HOENN, 1, 1, WORLD_PHASE_BEGINNING, DYNAMIC_ENCOUNTER_PROFILE_DEVELOPMENT_1},
};
// Keep maps sorted by (mapGroup, mapNum) for binary search.
static const struct DynamicEncounterMap sDynamicMaps[] =
{
    {MAP_GROUP(MAP_ROUTE101), MAP_NUM(MAP_ROUTE101), sRoute101Rules, ARRAY_COUNT(sRoute101Rules)},
};
