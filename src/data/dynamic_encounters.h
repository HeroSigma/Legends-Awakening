// DEVELOPMENT ONLY: Route 101 visibility tests; no canonical placements.
// Full encounter-type replacements, with shared tables across times of day.
static const struct WildPokemon sDevelopmentLand1[NUM_LAND_MONS_ENCOUNTER_SLOTS] =
{
    [0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_MAGIKARP},
};
static const struct WildPokemon sDevelopmentLand2[NUM_LAND_MONS_ENCOUNTER_SLOTS] =
{
    [0 ... NUM_LAND_MONS_ENCOUNTER_SLOTS - 1] = {2, 3, SPECIES_WAILMER},
};
static const struct WildPokemonInfo sDevelopmentInfo1 = {20, sDevelopmentLand1};
static const struct WildPokemonInfo sDevelopmentInfo2 = {20, sDevelopmentLand2};
static const struct WildEncounterTypes sDevelopmentTypes1[TIMES_OF_DAY_COUNT] =
{
    [0 ... TIMES_OF_DAY_COUNT - 1] = {.landMonsInfo = &sDevelopmentInfo1},
};
static const struct WildEncounterTypes sDevelopmentTypes2[TIMES_OF_DAY_COUNT] =
{
    [0 ... TIMES_OF_DAY_COUNT - 1] = {.landMonsInfo = &sDevelopmentInfo2},
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
