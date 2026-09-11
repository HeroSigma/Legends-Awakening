// Persistent namespace 1: Mt. Chimney Hiker / Sawyer family.
// Candidate IDs: Numel=1, Machop=2, Aron=3, Onix=4, Torkoal=5.
// Priority and stage-specific levels may change without renumbering identities.
#define SAWYER_SUPPLEMENT(id, speciesId, level) \
    { .candidateId = (id), .mon = { .species = (speciesId), .lvl = (level), \
      .gender = TRAINER_MON_RANDOM_GENDER, .ball = POKEBALL_COUNT, .nature = NATURE_HARDY } }

static const struct LARosterAuthoredRef sSawyer1Retained[] =
{
    {0, SPECIES_GEODUDE},
};
static const struct LARosterSupplement sSawyer1Supplements[] =
{
    SAWYER_SUPPLEMENT(1, SPECIES_NUMEL, 21),
    SAWYER_SUPPLEMENT(2, SPECIES_MACHOP, 21),
    SAWYER_SUPPLEMENT(3, SPECIES_ARON, 21),
    SAWYER_SUPPLEMENT(4, SPECIES_ONIX, 21),
    SAWYER_SUPPLEMENT(5, SPECIES_TORKOAL, 21),
};
STATIC_ASSERT(ARRAY_COUNT(sSawyer1Retained) + ARRAY_COUNT(sSawyer1Supplements) == PARTY_SIZE, Sawyer1RosterSize)
static const struct LARosterProfile sSawyer1Profile =
{
    .retained = sSawyer1Retained,
    .supplements = sSawyer1Supplements,
    .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sSawyer1Retained),
    .supplementCount = ARRAY_COUNT(sSawyer1Supplements),
};

static const struct LARosterAuthoredRef sSawyer2Retained[] =
{
    {0, SPECIES_GEODUDE},
    {1, SPECIES_NUMEL},
};
static const struct LARosterSupplement sSawyer2Supplements[] =
{
    SAWYER_SUPPLEMENT(2, SPECIES_MACHOP, 26),
    SAWYER_SUPPLEMENT(3, SPECIES_ARON, 26),
    SAWYER_SUPPLEMENT(4, SPECIES_ONIX, 26),
    SAWYER_SUPPLEMENT(5, SPECIES_TORKOAL, 26),
};
STATIC_ASSERT(ARRAY_COUNT(sSawyer2Retained) + ARRAY_COUNT(sSawyer2Supplements) == PARTY_SIZE, Sawyer2RosterSize)
static const struct LARosterProfile sSawyer2Profile =
{
    .retained = sSawyer2Retained,
    .supplements = sSawyer2Supplements,
    .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sSawyer2Retained),
    .supplementCount = ARRAY_COUNT(sSawyer2Supplements),
};

static const struct LARosterAuthoredRef sSawyer3Retained[] =
{
    {0, SPECIES_MACHOP},
    {1, SPECIES_NUMEL},
    {2, SPECIES_GRAVELER},
};
static const struct LARosterSupplement sSawyer3Supplements[] =
{
    SAWYER_SUPPLEMENT(3, SPECIES_ARON, 28),
    SAWYER_SUPPLEMENT(4, SPECIES_ONIX, 28),
    SAWYER_SUPPLEMENT(5, SPECIES_TORKOAL, 28),
};
STATIC_ASSERT(ARRAY_COUNT(sSawyer3Retained) + ARRAY_COUNT(sSawyer3Supplements) == PARTY_SIZE, Sawyer3RosterSize)
static const struct LARosterProfile sSawyer3Profile =
{
    .retained = sSawyer3Retained,
    .supplements = sSawyer3Supplements,
    .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sSawyer3Retained),
    .supplementCount = ARRAY_COUNT(sSawyer3Supplements),
};

static const struct LARosterAuthoredRef sSawyer4Retained[] =
{
    {0, SPECIES_MACHOP},
    {1, SPECIES_NUMEL},
    {2, SPECIES_GRAVELER},
};
static const struct LARosterSupplement sSawyer4Supplements[] =
{
    SAWYER_SUPPLEMENT(3, SPECIES_ARON, 30),
    SAWYER_SUPPLEMENT(4, SPECIES_ONIX, 30),
    SAWYER_SUPPLEMENT(5, SPECIES_TORKOAL, 30),
};
STATIC_ASSERT(ARRAY_COUNT(sSawyer4Retained) + ARRAY_COUNT(sSawyer4Supplements) == PARTY_SIZE, Sawyer4RosterSize)
static const struct LARosterProfile sSawyer4Profile =
{
    .retained = sSawyer4Retained,
    .supplements = sSawyer4Supplements,
    .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sSawyer4Retained),
    .supplementCount = ARRAY_COUNT(sSawyer4Supplements),
};

static const struct LARosterAuthoredRef sSawyer5Retained[] =
{
    {0, SPECIES_MACHOKE},
    {1, SPECIES_CAMERUPT},
    {2, SPECIES_GOLEM},
};
static const struct LARosterSupplement sSawyer5Supplements[] =
{
    SAWYER_SUPPLEMENT(3, SPECIES_ARON, 33),
    SAWYER_SUPPLEMENT(4, SPECIES_ONIX, 33),
    SAWYER_SUPPLEMENT(5, SPECIES_TORKOAL, 33),
};
STATIC_ASSERT(ARRAY_COUNT(sSawyer5Retained) + ARRAY_COUNT(sSawyer5Supplements) == PARTY_SIZE, Sawyer5RosterSize)
static const struct LARosterProfile sSawyer5Profile =
{
    .retained = sSawyer5Retained,
    .supplements = sSawyer5Supplements,
    .namespaceId = 1,
    .retainedCount = ARRAY_COUNT(sSawyer5Retained),
    .supplementCount = ARRAY_COUNT(sSawyer5Supplements),
};

#undef SAWYER_SUPPLEMENT

static const struct LARosterAssignment sLARosterAssignments[] =
{
    {TRAINER_SAWYER_1, DIFFICULTY_NORMAL, &sSawyer1Profile},
    {TRAINER_SAWYER_2, DIFFICULTY_NORMAL, &sSawyer2Profile},
    {TRAINER_SAWYER_3, DIFFICULTY_NORMAL, &sSawyer3Profile},
    {TRAINER_SAWYER_4, DIFFICULTY_NORMAL, &sSawyer4Profile},
    {TRAINER_SAWYER_5, DIFFICULTY_NORMAL, &sSawyer5Profile},
};
