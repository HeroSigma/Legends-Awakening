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

// Southwest Batch 1: explicit sets, reusable only through source assignments.
static const struct LASetTraining sFastPhysical = {{0, 252, 0, 0, 4, 252}, NATURE_JOLLY};
static const struct LASetTraining sFastSpecial = {{0, 0, 0, 252, 4, 252}, NATURE_TIMID};
static const struct LASetTraining sSpecialDefense = {{252, 0, 4, 0, 252, 0}, NATURE_CALM};

static const struct LACompetitiveSet sSwellowSet =
{
    .finalSpecies = SPECIES_SWELLOW, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_BRAVE_BIRD, MOVE_QUICK_ATTACK, MOVE_FACADE, MOVE_STEEL_WING},
    .heldItem = ITEM_SHARP_BEAK,
};
static const struct LACompetitiveSet sLinooneFieldSet =
{
    .finalSpecies = SPECIES_LINOONE, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_BODY_SLAM, MOVE_THIEF, MOVE_DIG, MOVE_THUNDER_WAVE},
    .heldItem = ITEM_SILK_SCARF,
};
static const struct LACompetitiveSet sLinooneWinstonSet =
{
    .finalSpecies = SPECIES_LINOONE, .training = &sFastPhysical, .abilitySlot = 1,
    .moves = {MOVE_BELLY_DRUM, MOVE_DOUBLE_EDGE, MOVE_THIEF, MOVE_ROCK_SMASH},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sLinooneCindySet =
{
    .finalSpecies = SPECIES_LINOONE, .training = &sPhysicalUtility, .abilitySlot = 0,
    .moves = {MOVE_BODY_SLAM, MOVE_BABY_DOLL_EYES, MOVE_THUNDER_WAVE, MOVE_REST},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sMightyenaSet =
{
    .finalSpecies = SPECIES_MIGHTYENA, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_CRUNCH, MOVE_SUCKER_PUNCH, MOVE_ICE_FANG, MOVE_HOWL},
    .heldItem = ITEM_BLACK_GLASSES,
};
static const struct LACompetitiveSet sBreloomSet =
{
    .finalSpecies = SPECIES_BRELOOM, .training = &sBulkyPhysical, .abilitySlot = 2,
    .moves = {MOVE_BULLET_SEED, MOVE_MACH_PUNCH, MOVE_ROCK_TOMB, MOVE_SWORDS_DANCE},
    .heldItem = ITEM_BLACK_BELT,
};
static const struct LACompetitiveSet sManectricSet =
{
    .finalSpecies = SPECIES_MANECTRIC, .training = &sFastSpecial, .abilitySlot = 1,
    .moves = {MOVE_THUNDERBOLT, MOVE_LIGHT_SCREEN, MOVE_FLAMETHROWER, MOVE_THUNDER_WAVE},
    .heldItem = ITEM_MAGNET,
};
static const struct LACompetitiveSet sSwalotSet =
{
    .finalSpecies = SPECIES_SWALOT, .training = &sSpecialUtility, .abilitySlot = 1,
    .moves = {MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_YAWN, MOVE_ENCORE},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sGrumpigSet =
{
    .finalSpecies = SPECIES_GRUMPIG, .training = &sBulkySpecial, .abilitySlot = 0,
    .moves = {MOVE_PSYCHIC, MOVE_SHADOW_BALL, MOVE_THUNDER_WAVE, MOVE_TAUNT},
    .heldItem = ITEM_TWISTED_SPOON,
};
static const struct LACompetitiveSet sKecleonSet =
{
    .finalSpecies = SPECIES_KECLEON, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_BODY_SLAM, MOVE_SHADOW_SNEAK, MOVE_BRICK_BREAK, MOVE_RECOVER},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sMawileSet =
{
    .finalSpecies = SPECIES_MAWILE, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_PLAY_ROUGH, MOVE_IRON_HEAD, MOVE_SUCKER_PUNCH, MOVE_SWORDS_DANCE},
    .heldItem = ITEM_METAL_COAT,
};
static const struct LACompetitiveSet sPelipperSet =
{
    .finalSpecies = SPECIES_PELIPPER, .training = &sSpecialUtility, .abilitySlot = 0,
    .moves = {MOVE_SURF, MOVE_AIR_SLASH, MOVE_ROOST, MOVE_ICY_WIND},
    .heldItem = ITEM_MYSTIC_WATER,
};
static const struct LACompetitiveSet sSkittySet =
{
    .finalSpecies = SPECIES_SKITTY, .training = &sPhysicalUtility, .abilitySlot = 0,
    .moves = {MOVE_FAKE_OUT, MOVE_BODY_SLAM, MOVE_WISH, MOVE_HEAL_BELL},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sRoseliaSet =
{
    .finalSpecies = SPECIES_ROSELIA, .training = &sBulkySpecial, .abilitySlot = 0,
    .moves = {MOVE_GIGA_DRAIN, MOVE_SLUDGE_BOMB, MOVE_SPIKES, MOVE_SYNTHESIS},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sSwabluSet =
{
    .finalSpecies = SPECIES_SWABLU, .training = &sPhysicalUtility, .abilitySlot = 0,
    .moves = {MOVE_BODY_SLAM, MOVE_ROOST, MOVE_SAFEGUARD, MOVE_SING},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sAltariaSet =
{
    .finalSpecies = SPECIES_ALTARIA, .training = &sSpecialUtility, .abilitySlot = 0,
    .moves = {MOVE_DRAGON_PULSE, MOVE_FLAMETHROWER, MOVE_REST, MOVE_SLEEP_TALK},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sBeautiflySet =
{
    .finalSpecies = SPECIES_BEAUTIFLY, .training = &sFastSpecial, .abilitySlot = 0,
    .moves = {MOVE_BUG_BUZZ, MOVE_AIR_CUTTER, MOVE_GIGA_DRAIN, MOVE_QUIVER_DANCE},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sAzumarillSet =
{
    .finalSpecies = SPECIES_AZUMARILL, .training = &sBulkyPhysical, .abilitySlot = 1,
    .moves = {MOVE_WATERFALL, MOVE_PLAY_ROUGH, MOVE_BRICK_BREAK, MOVE_ICE_PUNCH},
    .heldItem = ITEM_MYSTIC_WATER,
};
static const struct LACompetitiveSet sLombreSet =
{
    .finalSpecies = SPECIES_LOMBRE, .training = &sBulkySpecial, .abilitySlot = 2,
    .moves = {MOVE_SURF, MOVE_GIGA_DRAIN, MOVE_ICE_BEAM, MOVE_FAKE_OUT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sIllumiseSet =
{
    .finalSpecies = SPECIES_ILLUMISE, .training = &sSpecialUtility, .abilitySlot = 2,
    .moves = {MOVE_ENCORE, MOVE_THUNDER_WAVE, MOVE_BUG_BUZZ, MOVE_MOONLIGHT},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sMasquerainSet =
{
    .finalSpecies = SPECIES_MASQUERAIN, .training = &sFastSpecial, .abilitySlot = 0,
    .moves = {MOVE_BUG_BUZZ, MOVE_AIR_SLASH, MOVE_STUN_SPORE, MOVE_GIGA_DRAIN},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sNinjaskAceSet =
{
    .finalSpecies = SPECIES_NINJASK, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_X_SCISSOR, MOVE_AERIAL_ACE, MOVE_SWORDS_DANCE, MOVE_PROTECT},
    .heldItem = ITEM_LUM_BERRY,
};
static const struct LACompetitiveSet sNinjaskCoverageSet =
{
    .finalSpecies = SPECIES_NINJASK, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_X_SCISSOR, MOVE_AERIAL_ACE, MOVE_DIG, MOVE_SCREECH},
    .heldItem = ITEM_SHARP_BEAK,
};
static const struct LACompetitiveSet sDustoxSet =
{
    .finalSpecies = SPECIES_DUSTOX, .training = &sSpecialDefense, .abilitySlot = 0,
    .moves = {MOVE_BUG_BUZZ, MOVE_SLUDGE_BOMB, MOVE_MOONLIGHT, MOVE_LIGHT_SCREEN},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sVolbeatSet =
{
    .finalSpecies = SPECIES_VOLBEAT, .training = &sSpecialUtility, .abilitySlot = 2,
    .moves = {MOVE_THUNDER_WAVE, MOVE_ENCORE, MOVE_BUG_BUZZ, MOVE_MOONLIGHT},
    .heldItem = ITEM_SITRUS_BERRY,
};

static const struct LACompetitiveSet *const sSwellowVariants[] = {&sSwellowSet};
static const struct LASetBundle sSwellowBundle = {sSwellowVariants, ARRAY_COUNT(sSwellowVariants)};

static const struct LACompetitiveSet *const sLinooneFieldVariants[] = {&sLinooneFieldSet};
static const struct LASetBundle sLinooneFieldBundle = {sLinooneFieldVariants, ARRAY_COUNT(sLinooneFieldVariants)};

static const struct LACompetitiveSet *const sLinooneWinstonVariants[] = {&sLinooneWinstonSet};
static const struct LASetBundle sLinooneWinstonBundle = {sLinooneWinstonVariants, ARRAY_COUNT(sLinooneWinstonVariants)};

static const struct LACompetitiveSet *const sLinooneCindyVariants[] = {&sLinooneCindySet};
static const struct LASetBundle sLinooneCindyBundle = {sLinooneCindyVariants, ARRAY_COUNT(sLinooneCindyVariants)};

static const struct LACompetitiveSet *const sMightyenaVariants[] = {&sMightyenaSet};
static const struct LASetBundle sMightyenaBundle = {sMightyenaVariants, ARRAY_COUNT(sMightyenaVariants)};

static const struct LACompetitiveSet *const sBreloomVariants[] = {&sBreloomSet};
static const struct LASetBundle sBreloomBundle = {sBreloomVariants, ARRAY_COUNT(sBreloomVariants)};

static const struct LACompetitiveSet *const sManectricVariants[] = {&sManectricSet};
static const struct LASetBundle sManectricBundle = {sManectricVariants, ARRAY_COUNT(sManectricVariants)};

static const struct LACompetitiveSet *const sSwalotVariants[] = {&sSwalotSet};
static const struct LASetBundle sSwalotBundle = {sSwalotVariants, ARRAY_COUNT(sSwalotVariants)};

static const struct LACompetitiveSet *const sGrumpigVariants[] = {&sGrumpigSet};
static const struct LASetBundle sGrumpigBundle = {sGrumpigVariants, ARRAY_COUNT(sGrumpigVariants)};

static const struct LACompetitiveSet *const sKecleonVariants[] = {&sKecleonSet};
static const struct LASetBundle sKecleonBundle = {sKecleonVariants, ARRAY_COUNT(sKecleonVariants)};

static const struct LACompetitiveSet *const sMawileVariants[] = {&sMawileSet};
static const struct LASetBundle sMawileBundle = {sMawileVariants, ARRAY_COUNT(sMawileVariants)};

static const struct LACompetitiveSet *const sPelipperVariants[] = {&sPelipperSet};
static const struct LASetBundle sPelipperBundle = {sPelipperVariants, ARRAY_COUNT(sPelipperVariants)};

static const struct LACompetitiveSet *const sSkittyVariants[] = {&sSkittySet};
static const struct LASetBundle sSkittyBundle = {sSkittyVariants, ARRAY_COUNT(sSkittyVariants)};

static const struct LACompetitiveSet *const sRoseliaVariants[] = {&sRoseliaSet};
static const struct LASetBundle sRoseliaBundle = {sRoseliaVariants, ARRAY_COUNT(sRoseliaVariants)};

static const struct LACompetitiveSet *const sBeautiflyVariants[] = {&sBeautiflySet};
static const struct LASetBundle sBeautiflyBundle = {sBeautiflyVariants, ARRAY_COUNT(sBeautiflyVariants)};

static const struct LACompetitiveSet *const sAzumarillVariants[] = {&sAzumarillSet};
static const struct LASetBundle sAzumarillBundle = {sAzumarillVariants, ARRAY_COUNT(sAzumarillVariants)};

static const struct LACompetitiveSet *const sLombreVariants[] = {&sLombreSet};
static const struct LASetBundle sLombreBundle = {sLombreVariants, ARRAY_COUNT(sLombreVariants)};

static const struct LACompetitiveSet *const sIllumiseVariants[] = {&sIllumiseSet};
static const struct LASetBundle sIllumiseBundle = {sIllumiseVariants, ARRAY_COUNT(sIllumiseVariants)};

static const struct LACompetitiveSet *const sMasquerainVariants[] = {&sMasquerainSet};
static const struct LASetBundle sMasquerainBundle = {sMasquerainVariants, ARRAY_COUNT(sMasquerainVariants)};

static const struct LACompetitiveSet *const sNinjaskAceVariants[] = {&sNinjaskAceSet};
static const struct LASetBundle sNinjaskAceBundle = {sNinjaskAceVariants, ARRAY_COUNT(sNinjaskAceVariants)};

static const struct LACompetitiveSet *const sNinjaskCoverageVariants[] = {&sNinjaskCoverageSet};
static const struct LASetBundle sNinjaskCoverageBundle = {sNinjaskCoverageVariants, ARRAY_COUNT(sNinjaskCoverageVariants)};

static const struct LACompetitiveSet *const sDustoxVariants[] = {&sDustoxSet};
static const struct LASetBundle sDustoxBundle = {sDustoxVariants, ARRAY_COUNT(sDustoxVariants)};

static const struct LACompetitiveSet *const sVolbeatVariants[] = {&sVolbeatSet};
static const struct LASetBundle sVolbeatBundle = {sVolbeatVariants, ARRAY_COUNT(sVolbeatVariants)};

static const struct LACompetitiveSet *const sCindyBirdVariants[] = {&sSwabluSet, &sAltariaSet};
static const struct LASetBundle sCindyBirdBundle = {sCindyBirdVariants, ARRAY_COUNT(sCindyBirdVariants)};

// Batch 2: reviewed final-species sets; source identities remain in assignments.
static const struct LACompetitiveSet sGyaradosSet =
{
    .finalSpecies = SPECIES_GYARADOS, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_WATERFALL, MOVE_EARTHQUAKE, MOVE_ICE_FANG, MOVE_DRAGON_DANCE},
    .heldItem = ITEM_LUM_BERRY,
};
static const struct LACompetitiveSet sCarvanhaSet =
{
    .finalSpecies = SPECIES_CARVANHA, .training = &sFastPhysical, .abilitySlot = 2,
    .moves = {MOVE_LIQUIDATION, MOVE_CRUNCH, MOVE_ICE_FANG, MOVE_PROTECT},
    .heldItem = ITEM_MYSTIC_WATER,
};
static const struct LACompetitiveSet sSharpedoSet =
{
    .finalSpecies = SPECIES_SHARPEDO, .training = &sFastPhysical, .abilitySlot = 2,
    .moves = {MOVE_LIQUIDATION, MOVE_CRUNCH, MOVE_ICE_FANG, MOVE_PROTECT},
    .heldItem = ITEM_MYSTIC_WATER,
};
static const struct LACompetitiveSet sTentacoolSet =
{
    .finalSpecies = SPECIES_TENTACOOL, .training = &sSpecialUtility, .abilitySlot = 1,
    .moves = {MOVE_SURF, MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_HAZE},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sTentacruelSet =
{
    .finalSpecies = SPECIES_TENTACRUEL, .training = &sSpecialUtility, .abilitySlot = 1,
    .moves = {MOVE_SURF, MOVE_SLUDGE_BOMB, MOVE_GIGA_DRAIN, MOVE_TOXIC},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sLoudredSet =
{
    .finalSpecies = SPECIES_LOUDRED, .training = &sBulkySpecial, .abilitySlot = 2,
    .moves = {MOVE_HYPER_VOICE, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_SHADOW_BALL},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sExploudSet =
{
    .finalSpecies = SPECIES_EXPLOUD, .training = &sBulkySpecial, .abilitySlot = 2,
    .moves = {MOVE_BOOMBURST, MOVE_FLAMETHROWER, MOVE_ICE_BEAM, MOVE_SURF},
    .heldItem = ITEM_SILK_SCARF,
};
static const struct LACompetitiveSet sKirliaSet =
{
    .finalSpecies = SPECIES_KIRLIA, .training = &sBulkySpecial, .abilitySlot = 1,
    .moves = {MOVE_PSYCHIC, MOVE_DRAINING_KISS, MOVE_CALM_MIND, MOVE_THUNDERBOLT},
    .heldItem = ITEM_EVIOLITE,
};
static const struct LACompetitiveSet sBanetteSet =
{
    .finalSpecies = SPECIES_BANETTE, .training = &sBulkyPhysical, .abilitySlot = 0,
    .moves = {MOVE_KNOCK_OFF, MOVE_SHADOW_SNEAK, MOVE_WILL_O_WISP, MOVE_SUCKER_PUNCH},
    .heldItem = ITEM_SITRUS_BERRY,
};
static const struct LACompetitiveSet sMedichamSet =
{
    .finalSpecies = SPECIES_MEDICHAM, .training = &sFastPhysical, .abilitySlot = 0,
    .moves = {MOVE_HIGH_JUMP_KICK, MOVE_ZEN_HEADBUTT, MOVE_ICE_PUNCH, MOVE_THUNDER_PUNCH},
    .heldItem = ITEM_BLACK_BELT,
};
static const struct LACompetitiveSet sChimechoSet =
{
    .finalSpecies = SPECIES_CHIMECHO, .training = &sSpecialUtility, .abilitySlot = 0,
    .moves = {MOVE_PSYCHIC, MOVE_RECOVER, MOVE_HEAL_BELL, MOVE_THUNDER_WAVE},
    .heldItem = ITEM_LEFTOVERS,
};
static const struct LACompetitiveSet sSableyeSet =
{
    .finalSpecies = SPECIES_SABLEYE, .training = &sPhysicalUtility, .abilitySlot = 2,
    .moves = {MOVE_KNOCK_OFF, MOVE_RECOVER, MOVE_TAUNT, MOVE_THUNDER_WAVE},
    .heldItem = ITEM_LEFTOVERS,
};

static const struct LACompetitiveSet *const sCarvanhaLineVariants[] = {&sCarvanhaSet, &sSharpedoSet};
static const struct LASetBundle sCarvanhaLineBundle = {sCarvanhaLineVariants, ARRAY_COUNT(sCarvanhaLineVariants)};
static const struct LACompetitiveSet *const sTentacoolLineVariants[] = {&sTentacoolSet, &sTentacruelSet};
static const struct LASetBundle sTentacoolLineBundle = {sTentacoolLineVariants, ARRAY_COUNT(sTentacoolLineVariants)};
static const struct LACompetitiveSet *const sLoudredLineVariants[] = {&sLoudredSet, &sExploudSet};
static const struct LASetBundle sLoudredLineBundle = {sLoudredLineVariants, ARRAY_COUNT(sLoudredLineVariants)};
static const struct LACompetitiveSet *const sGyaradosVariants[] = {&sGyaradosSet};
static const struct LASetBundle sGyaradosBundle = {sGyaradosVariants, ARRAY_COUNT(sGyaradosVariants)};
static const struct LACompetitiveSet *const sSharpedoVariants[] = {&sSharpedoSet};
static const struct LASetBundle sSharpedoBundle = {sSharpedoVariants, ARRAY_COUNT(sSharpedoVariants)};
static const struct LACompetitiveSet *const sTentacruelVariants[] = {&sTentacruelSet};
static const struct LASetBundle sTentacruelBundle = {sTentacruelVariants, ARRAY_COUNT(sTentacruelVariants)};
static const struct LACompetitiveSet *const sKirliaVariants[] = {&sKirliaSet};
static const struct LASetBundle sKirliaBundle = {sKirliaVariants, ARRAY_COUNT(sKirliaVariants)};
static const struct LACompetitiveSet *const sBanetteVariants[] = {&sBanetteSet};
static const struct LASetBundle sBanetteBundle = {sBanetteVariants, ARRAY_COUNT(sBanetteVariants)};
static const struct LACompetitiveSet *const sMedichamVariants[] = {&sMedichamSet};
static const struct LASetBundle sMedichamBundle = {sMedichamVariants, ARRAY_COUNT(sMedichamVariants)};
static const struct LACompetitiveSet *const sChimechoVariants[] = {&sChimechoSet};
static const struct LASetBundle sChimechoBundle = {sChimechoVariants, ARRAY_COUNT(sChimechoVariants)};
static const struct LACompetitiveSet *const sSableyeVariants[] = {&sSableyeSet};
static const struct LASetBundle sSableyeBundle = {sSableyeVariants, ARRAY_COUNT(sSableyeVariants)};

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
    {TRAINER_CALVIN_4, DIFFICULTY_NORMAL, 0, SPECIES_SWELLOW, &sSwellowBundle},
    {TRAINER_CALVIN_4, DIFFICULTY_NORMAL, 1, SPECIES_LINOONE, &sLinooneFieldBundle},
    {TRAINER_CALVIN_4, DIFFICULTY_NORMAL, 2, SPECIES_MIGHTYENA, &sMightyenaBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0, SPECIES_SWELLOW, &sSwellowBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 1, SPECIES_LINOONE, &sLinooneFieldBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 2, SPECIES_MIGHTYENA, &sMightyenaBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0x80020001u, SPECIES_SHROOMISH, &sBreloomBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0x80020002u, SPECIES_ELECTRIKE, &sManectricBundle},
    {TRAINER_CALVIN_5, DIFFICULTY_NORMAL, 0x80020003u, SPECIES_GULPIN, &sSwalotBundle},
    {TRAINER_WINSTON_4, DIFFICULTY_NORMAL, 0, SPECIES_LINOONE, &sLinooneWinstonBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0, SPECIES_LINOONE, &sLinooneWinstonBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030001u, SPECIES_ELECTRIKE, &sManectricBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030002u, SPECIES_SPOINK, &sGrumpigBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030003u, SPECIES_KECLEON, &sKecleonBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030004u, SPECIES_MAWILE, &sMawileBundle},
    {TRAINER_WINSTON_5, DIFFICULTY_NORMAL, 0x80030005u, SPECIES_WINGULL, &sPelipperBundle},
    {TRAINER_CINDY_5, DIFFICULTY_NORMAL, 0, SPECIES_LINOONE, &sLinooneCindyBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0, SPECIES_LINOONE, &sLinooneCindyBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040001u, SPECIES_SKITTY, &sSkittyBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040002u, SPECIES_ROSELIA, &sRoseliaBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040003u, SPECIES_SWABLU, &sCindyBirdBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040004u, SPECIES_BEAUTIFLY, &sBeautiflyBundle},
    {TRAINER_CINDY_6, DIFFICULTY_NORMAL, 0x80040005u, SPECIES_MARILL, &sAzumarillBundle},
    {TRAINER_HALEY_4, DIFFICULTY_NORMAL, 0, SPECIES_LOMBRE, &sLombreBundle},
    {TRAINER_HALEY_4, DIFFICULTY_NORMAL, 1, SPECIES_BRELOOM, &sBreloomBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0, SPECIES_SWELLOW, &sSwellowBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 1, SPECIES_LOMBRE, &sLombreBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 2, SPECIES_BRELOOM, &sBreloomBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0x80050001u, SPECIES_ROSELIA, &sRoseliaBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0x80050002u, SPECIES_WINGULL, &sPelipperBundle},
    {TRAINER_HALEY_5, DIFFICULTY_NORMAL, 0x80050003u, SPECIES_ILLUMISE, &sIllumiseBundle},
    {TRAINER_JAMES_4, DIFFICULTY_NORMAL, 0, SPECIES_SURSKIT, &sMasquerainBundle},
    {TRAINER_JAMES_4, DIFFICULTY_NORMAL, 1, SPECIES_DUSTOX, &sDustoxBundle},
    {TRAINER_JAMES_4, DIFFICULTY_NORMAL, 2, SPECIES_NINJASK, &sNinjaskAceBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 0, SPECIES_SURSKIT, &sMasquerainBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 1, SPECIES_NINJASK, &sNinjaskCoverageBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 2, SPECIES_DUSTOX, &sDustoxBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 3, SPECIES_NINJASK, &sNinjaskAceBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 0x80060001u, SPECIES_BEAUTIFLY, &sBeautiflyBundle},
    {TRAINER_JAMES_5, DIFFICULTY_NORMAL, 0x80060002u, SPECIES_VOLBEAT, &sVolbeatBundle},
    // Batch 2: 42 Normal-only assignments; guards are pre-evolution species.
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 0, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 1, SPECIES_CARVANHA, &sCarvanhaLineBundle},
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 2, SPECIES_TENTACOOL, &sTentacoolLineBundle},
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 3, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 0x80070001u, SPECIES_TENTACRUEL, &sTentacruelBundle},
    {TRAINER_ELLIOT_3, DIFFICULTY_NORMAL, 0x80070002u, SPECIES_SHARPEDO, &sSharpedoBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 0, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 1, SPECIES_CARVANHA, &sCarvanhaLineBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 2, SPECIES_TENTACRUEL, &sTentacruelBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 3, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 0x80070001u, SPECIES_TENTACRUEL, &sTentacruelBundle},
    {TRAINER_ELLIOT_4, DIFFICULTY_NORMAL, 0x80070002u, SPECIES_SHARPEDO, &sSharpedoBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 0, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 1, SPECIES_SHARPEDO, &sSharpedoBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 2, SPECIES_GYARADOS, &sGyaradosBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 3, SPECIES_TENTACRUEL, &sTentacruelBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 0x80070001u, SPECIES_TENTACRUEL, &sTentacruelBundle},
    {TRAINER_ELLIOT_5, DIFFICULTY_NORMAL, 0x80070002u, SPECIES_SHARPEDO, &sSharpedoBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0, SPECIES_BRELOOM, &sBreloomBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 1, SPECIES_LOUDRED, &sLoudredLineBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080001u, SPECIES_BEAUTIFLY, &sBeautiflyBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080002u, SPECIES_SURSKIT, &sMasquerainBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080003u, SPECIES_ROSELIA, &sRoseliaBundle},
    {TRAINER_KAREN_4, DIFFICULTY_NORMAL, 0x80080004u, SPECIES_NINJASK, &sNinjaskAceBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 0, SPECIES_BRELOOM, &sBreloomBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 1, SPECIES_EXPLOUD, &sLoudredLineBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 0x80080001u, SPECIES_BEAUTIFLY, &sBeautiflyBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 0x80080002u, SPECIES_SURSKIT, &sMasquerainBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 0x80080003u, SPECIES_ROSELIA, &sRoseliaBundle},
    {TRAINER_KAREN_5, DIFFICULTY_NORMAL, 0x80080004u, SPECIES_NINJASK, &sNinjaskAceBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0, SPECIES_KIRLIA, &sKirliaBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 1, SPECIES_MEDICHAM, &sMedichamBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0x80090001u, SPECIES_GRUMPIG, &sGrumpigBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0x80090002u, SPECIES_CHIMECHO, &sChimechoBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0x80090003u, SPECIES_SABLEYE, &sSableyeBundle},
    {TRAINER_JERRY_4, DIFFICULTY_NORMAL, 0x80090004u, SPECIES_BANETTE, &sBanetteBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 0, SPECIES_KIRLIA, &sKirliaBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 1, SPECIES_BANETTE, &sBanetteBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 2, SPECIES_MEDICHAM, &sMedichamBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 0x80090001u, SPECIES_GRUMPIG, &sGrumpigBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 0x80090002u, SPECIES_CHIMECHO, &sChimechoBundle},
    {TRAINER_JERRY_5, DIFFICULTY_NORMAL, 0x80090003u, SPECIES_SABLEYE, &sSableyeBundle},
};
