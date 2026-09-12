// Shared ROM records use TrainerMon EV storage order, not macro argument order.
static const struct LASetTraining sBulkyPhysical = {{252, 252, 0, 0, 4, 0}, NATURE_ADAMANT};
static const struct LASetTraining sBulkySpecial = {{252, 0, 0, 252, 4, 0}, NATURE_MODEST};
static const struct LASetTraining sPhysicalUtility = {{252, 0, 252, 0, 4, 0}, NATURE_IMPISH};
static const struct LASetTraining sSpecialUtility = {{252, 0, 252, 0, 4, 0}, NATURE_BOLD};

// Slots are intentional: Sturdy=1 in Geodude/Onix, 0 in Aron;
// No Guard/Simple/Solid Rock=1, White Smoke=0 in the configured species data.
static const struct LACompetitiveSet sGeodudeSet =
{
    .finalSpecies = SPECIES_GEODUDE, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_EARTHQUAKE, MOVE_ROCK_SLIDE, MOVE_STEALTH_ROCK, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sGravelerSet =
{
    .finalSpecies = SPECIES_GRAVELER, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_EARTHQUAKE, MOVE_ROCK_SLIDE, MOVE_STEALTH_ROCK, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sGolemSet =
{
    .finalSpecies = SPECIES_GOLEM, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_EARTHQUAKE, MOVE_ROCK_SLIDE, MOVE_STEALTH_ROCK, MOVE_PROTECT},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sMachopSet =
{
    .finalSpecies = SPECIES_MACHOP, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_DYNAMIC_PUNCH, MOVE_KNOCK_OFF, MOVE_ROCK_SLIDE, MOVE_BULK_UP},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sMachokeSet =
{
    .finalSpecies = SPECIES_MACHOKE, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_DYNAMIC_PUNCH, MOVE_KNOCK_OFF, MOVE_ROCK_SLIDE, MOVE_BULK_UP},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sNumelSet =
{
    .finalSpecies = SPECIES_NUMEL, .training = &sBulkySpecial, .abilitySlot = 1,
    .moves = {MOVE_LAVA_PLUME, MOVE_EARTH_POWER, MOVE_AMNESIA, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sCameruptSet =
{
    .finalSpecies = SPECIES_CAMERUPT, .training = &sBulkySpecial, .abilitySlot = 1,
    .moves = {MOVE_LAVA_PLUME, MOVE_EARTH_POWER, MOVE_YAWN, MOVE_PROTECT},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sAronSet =
{
    .finalSpecies = SPECIES_ARON, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_IRON_TAIL, MOVE_ROCK_SLIDE, MOVE_EARTHQUAKE, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sLaironSet =
{
    .finalSpecies = SPECIES_LAIRON, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_IRON_TAIL, MOVE_ROCK_SLIDE, MOVE_EARTHQUAKE, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sAggronSet =
{
    .finalSpecies = SPECIES_AGGRON, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_IRON_TAIL, MOVE_ROCK_SLIDE, MOVE_EARTHQUAKE, MOVE_PROTECT},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sOnixSet =
{
    .finalSpecies = SPECIES_ONIX, .training = &sPhysicalUtility, .abilitySlot = 1,
    .moves = {MOVE_ROCK_TOMB, MOVE_EARTHQUAKE, MOVE_TAUNT, MOVE_PROTECT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sTorkoalSet =
{
    .finalSpecies = SPECIES_TORKOAL, .training = &sSpecialUtility, .abilitySlot = 0,
    .moves = {MOVE_LAVA_PLUME, MOVE_RAPID_SPIN, MOVE_YAWN, MOVE_PROTECT},
    .heldItem = ITEM_LEFTOVERS,
};

static const struct LACompetitiveSet *const sGeodudeVariants[] = {&sGeodudeSet, &sGravelerSet};
static const struct LASetBundle sGeodudeBundle = {sGeodudeVariants, ARRAY_COUNT(sGeodudeVariants)};

static const struct LACompetitiveSet *const sGravelerVariants[] = {&sGravelerSet};
static const struct LASetBundle sGravelerBundle = {sGravelerVariants, ARRAY_COUNT(sGravelerVariants)};

static const struct LACompetitiveSet *const sGolemVariants[] = {&sGolemSet};
static const struct LASetBundle sGolemBundle = {sGolemVariants, ARRAY_COUNT(sGolemVariants)};

static const struct LACompetitiveSet *const sMachopVariants[] = {&sMachopSet, &sMachokeSet};
static const struct LASetBundle sMachopBundle = {sMachopVariants, ARRAY_COUNT(sMachopVariants)};

static const struct LACompetitiveSet *const sMachokeVariants[] = {&sMachokeSet};
static const struct LASetBundle sMachokeBundle = {sMachokeVariants, ARRAY_COUNT(sMachokeVariants)};

static const struct LACompetitiveSet *const sNumelVariants[] = {&sNumelSet, &sCameruptSet};
static const struct LASetBundle sNumelBundle = {sNumelVariants, ARRAY_COUNT(sNumelVariants)};

static const struct LACompetitiveSet *const sCameruptVariants[] = {&sCameruptSet};
static const struct LASetBundle sCameruptBundle = {sCameruptVariants, ARRAY_COUNT(sCameruptVariants)};

static const struct LACompetitiveSet *const sAronVariants[] = {&sAronSet, &sLaironSet, &sAggronSet};
static const struct LASetBundle sAronBundle = {sAronVariants, ARRAY_COUNT(sAronVariants)};

static const struct LACompetitiveSet *const sOnixVariants[] = {&sOnixSet};
static const struct LASetBundle sOnixBundle = {sOnixVariants, ARRAY_COUNT(sOnixVariants)};

static const struct LACompetitiveSet *const sTorkoalVariants[] = {&sTorkoalSet};
static const struct LASetBundle sTorkoalBundle = {sTorkoalVariants, ARRAY_COUNT(sTorkoalVariants)};

static const struct LASetAssignment sLASetAssignments[] =
{
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE, &sGeodudeBundle},
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 1), SPECIES_NUMEL, &sNumelBundle},
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 2), SPECIES_MACHOP, &sMachopBundle},
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 3), SPECIES_ARON, &sAronBundle},
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 4), SPECIES_ONIX, &sOnixBundle},
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 5), SPECIES_TORKOAL, &sTorkoalBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, 0, SPECIES_GEODUDE, &sGeodudeBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, 1, SPECIES_NUMEL, &sNumelBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 2), SPECIES_MACHOP, &sMachopBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 3), SPECIES_ARON, &sAronBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 4), SPECIES_ONIX, &sOnixBundle},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 5), SPECIES_TORKOAL, &sTorkoalBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, 0, SPECIES_MACHOP, &sMachopBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, 1, SPECIES_NUMEL, &sNumelBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, 2, SPECIES_GRAVELER, &sGravelerBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 3), SPECIES_ARON, &sAronBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 4), SPECIES_ONIX, &sOnixBundle},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 5), SPECIES_TORKOAL, &sTorkoalBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, 0, SPECIES_MACHOP, &sMachopBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, 1, SPECIES_NUMEL, &sNumelBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, 2, SPECIES_GRAVELER, &sGravelerBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 3), SPECIES_ARON, &sAronBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 4), SPECIES_ONIX, &sOnixBundle},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 5), SPECIES_TORKOAL, &sTorkoalBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, 0, SPECIES_MACHOKE, &sMachokeBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, 1, SPECIES_CAMERUPT, &sCameruptBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, 2, SPECIES_GOLEM, &sGolemBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 3), SPECIES_ARON, &sAronBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 4), SPECIES_ONIX, &sOnixBundle},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, LA_ROSTER_SUPPLEMENT_KEY(1, 5), SPECIES_TORKOAL, &sTorkoalBundle},
};
