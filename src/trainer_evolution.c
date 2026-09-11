#include "global.h"
#include "pokemon.h"
#include "item.h"
#include "trainer_evolution.h"
#include "constants/hold_effects.h"

#include "data/trainer_evolution.h"

static const struct TrainerEvolutionProfile *FindProfile(
    const struct TrainerEvolutionAssignment *assignments, u32 count,
    u16 trainerId, enum DifficultyLevel difficulty, u32 sourceIndex,
    enum Species authoredSpecies)
{
    const struct TrainerEvolutionProfile *result = NULL;
    for (u32 i = 0; i < count; i++)
    {
        const struct TrainerEvolutionAssignment *entry = &assignments[i];
        if (entry->trainerId == trainerId && entry->difficulty == difficulty
         && entry->sourceIndex == sourceIndex && entry->authoredSpecies == authoredSpecies)
        {
            // Conflicting assignments must never select a branch by row order.
            static const struct TrainerEvolutionProfile preserve = {.preserveSpecies = TRUE};
            if (result != NULL)
                return &preserve;
            result = entry->profile;
        }
    }
    return result;
}

const struct TrainerEvolutionProfile *GetTrainerEvolutionProfile(u16 trainerId,
    enum DifficultyLevel difficulty, u32 sourceIndex, enum Species authoredSpecies)
{
    return FindProfile(sTrainerEvolutionAssignments, ARRAY_COUNT(sTrainerEvolutionAssignments),
        trainerId, difficulty, sourceIndex, authoredSpecies);
}

#if TESTING
// Exercise the actual key matcher with test-local metadata, without ROM balance rows.
const struct TrainerEvolutionProfile *TestFindTrainerEvolutionProfile(
    const struct TrainerEvolutionAssignment *entries, u32 count, u16 trainerId,
    enum DifficultyLevel difficulty, u32 sourceIndex, enum Species species)
{
    return FindProfile(entries, count, trainerId, difficulty, sourceIndex, species);
}
#endif

static bool32 ValidSpecies(enum Species species)
{
    // IsSpeciesEnabled indexes directly; do not call it until bounds are checked.
    return species > SPECIES_NONE && species < NUM_SPECIES && IsSpeciesEnabled(species);
}

static bool32 SafeSpecies(enum Species species)
{
    if (!ValidSpecies(species))
        return FALSE;
    const struct SpeciesInfo *info = &gSpeciesInfo[species];
    if (info->isRestrictedLegendary || info->isSubLegendary || info->isMythical
     || info->isUltraBeast || info->isParadox || info->isAlolanForm
     || info->isGalarianForm || info->isHisuianForm || info->isPaldeanForm
     || info->isTotem || info->isMegaEvolution || info->isPrimalReversion
     || info->isUltraBurst || info->isGigantamax || info->isTeraForm)
        return FALSE;
    if (GET_BASE_SPECIES_ID(species) != species)
        return FALSE;
    for (u32 i = 0; i < ARRAY_COUNT(sTrainerEvolutionDeniedSpecies); i++)
        if (sTrainerEvolutionDeniedSpecies[i] == species)
            return FALSE;
    return TRUE;
}

static bool32 HasConditions(const struct Evolution *evo)
{
    return evo->params != NULL && evo->params[0].condition != CONDITIONS_END;
}

// Only structural gender restrictions survive an explicit progression override.
// No runtime conditions are evaluated and no gameplay evolution helpers are called.
static bool32 CompatibleGender(const struct TrainerMon *mon, enum Species target,
    const struct Evolution *edge)
{
    u32 sourceRatio = gSpeciesInfo[mon->species].genderRatio;
    u32 targetRatio = gSpeciesInfo[target].genderRatio;
    if (mon->gender == TRAINER_MON_RANDOM_GENDER)
    {
        if (sourceRatio != targetRatio)
            return FALSE;
    }
    else if (mon->gender == TRAINER_MON_MALE)
    {
        if (sourceRatio >= MON_FEMALE || targetRatio >= MON_FEMALE)
            return FALSE;
    }
    else if (mon->gender == TRAINER_MON_FEMALE)
    {
        if (sourceRatio == MON_MALE || sourceRatio == MON_GENDERLESS
         || targetRatio == MON_MALE || targetRatio == MON_GENDERLESS)
            return FALSE;
    }
    else
        return FALSE;

    for (u32 i = 0; edge->params != NULL && edge->params[i].condition != CONDITIONS_END; i++)
    {
        if (edge->params[i].condition == IF_GENDER)
        {
            if (mon->gender == TRAINER_MON_RANDOM_GENDER)
                return FALSE;
            if ((mon->gender == TRAINER_MON_MALE ? MON_MALE : MON_FEMALE) != edge->params[i].arg1)
                return FALSE;
        }
    }
    return TRUE;
}

#if TESTING
// Virtual ability data for failure-path tests; never writes gSpeciesInfo.
static enum Species sTestAbilitySpecies;
static const enum Ability *sTestAbilities;
#endif

static enum Ability EvolutionAbility(enum Species species, u32 slot)
{
#if TESTING
    if (species == sTestAbilitySpecies && sTestAbilities != NULL)
        return sTestAbilities[slot];
#endif
    return gSpeciesInfo[species].abilities[slot];
}

static bool32 MapAbility(const struct TrainerMon *mon, enum Species target, enum Ability *ability)
{
    *ability = mon->ability;
    if (mon->ability == ABILITY_NONE)
        return TRUE;

    u32 slot = NUM_ABILITY_SLOTS;
    for (u32 i = 0; i < NUM_ABILITY_SLOTS; i++)
    {
        if (EvolutionAbility(mon->species, i) == mon->ability)
        {
            if (slot != NUM_ABILITY_SLOTS)
                return FALSE; // ID cannot express an unambiguous source slot.
            slot = i;
        }
    }
    if (slot == NUM_ABILITY_SLOTS)
        return FALSE;
    *ability = EvolutionAbility(target, slot);
    if (*ability == ABILITY_NONE)
        return FALSE;
    for (u32 i = 0; i < NUM_ABILITY_SLOTS; i++)
        if (i != slot && EvolutionAbility(target, i) == *ability)
            return FALSE;
    return TRUE;
}

static enum Species Traverse(enum Species species, u8 level,
    const struct TrainerEvolutionProfile *profile, struct TrainerMon *mon)
{
    u8 visited[(NUM_SPECIES + 7) / 8] = {0};
    if (level < MIN_LEVEL || level > MAX_LEVEL || !SafeSpecies(species)
     || (profile != NULL && (profile->preserveSpecies || (profile->stepCount && profile->steps == NULL))))
        return species;

    while (TRUE)
    {
        visited[species / 8] |= 1 << (species % 8);
        const struct Evolution *evos = GetSpeciesEvolutions(species);
        if (evos == NULL)
            return species;

        const struct TrainerEvolutionStep *step = NULL;
        for (u32 i = 0; profile != NULL && i < profile->stepCount; i++)
        {
            if (profile->steps[i].from == species)
            {
                if (step != NULL)
                    return species;
                step = &profile->steps[i];
            }
        }

        enum Species firstTarget = SPECIES_NONE;
        bool32 ambiguous = FALSE;
        for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            if (evos[i].method == EVO_NONE)
                continue;
            if (evos[i].method == EVO_SPLIT_FROM_EVO || !ValidSpecies(evos[i].targetSpecies))
                return species;
            if (firstTarget == SPECIES_NONE)
                firstTarget = evos[i].targetSpecies;
            else if (firstTarget != evos[i].targetSpecies)
                ambiguous = TRUE;
        }
        if (firstTarget == SPECIES_NONE || (ambiguous && step == NULL))
            return species;
        enum Species target = step != NULL ? step->to : firstTarget;
        if (!SafeSpecies(target) || (visited[target / 8] & (1 << (target % 8))))
            return species;

        u32 threshold = 0;
        bool32 foundEdge = FALSE;
        for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
        {
            const struct Evolution *edge = &evos[i];
            if (edge->method == EVO_NONE || edge->targetSpecies != target)
                continue;
            foundEdge = TRUE;
            // All methods to this target must preserve structural gender intent.
            if (mon != NULL && !CompatibleGender(mon, target, edge))
                return species;
            if (edge->method == EVO_LEVEL && edge->param > 0 && edge->param <= MAX_LEVEL
             && (step != NULL || !HasConditions(edge)))
            {
                if (threshold != 0 && threshold != edge->param)
                    return species;
                threshold = edge->param;
            }
        }
        if (!foundEdge)
            return species; // Profiles cannot invent relationships.
        if (step != NULL)
        {
            if (step->thresholdSource == TRAINER_EVO_EXPLICIT_LEVEL)
                threshold = step->level;
            else if (step->thresholdSource != TRAINER_EVO_CANONICAL_LEVEL)
                return species;
        }
        if (threshold == 0 || threshold > MAX_LEVEL || level < threshold)
            return species;

        if (mon != NULL)
        {
            enum Ability ability;
            if (!MapAbility(mon, target, &ability))
                return species;
            mon->ability = ability;
            mon->species = target;
        }
        species = target;
    }
}

enum Species ResolveTrainerScaledSpecies(enum Species authoredSpecies, u8 finalLevel,
    const struct TrainerEvolutionProfile *profile)
{
    return Traverse(authoredSpecies, finalLevel, profile, NULL);
}

bool32 ApplyTrainerEvolution(struct TrainerMon *workingMon, const struct TrainerEvolutionProfile *profile)
{
    enum Species original = workingMon->species;
    enum HoldEffect effect = GetItemHoldEffect(workingMon->heldItem);
    if (workingMon->gigantamaxFactor || effect == HOLD_EFFECT_PREVENT_EVOLVE || effect == HOLD_EFFECT_EVIOLITE)
        return FALSE;
    return Traverse(original, workingMon->lvl, profile, workingMon) != original;
}

#if TESTING
bool32 TestApplyTrainerEvolutionWithAbilities(struct TrainerMon *mon, enum Species species,
    const enum Ability *abilities)
{
    sTestAbilitySpecies = species;
    sTestAbilities = abilities;
    bool32 changed = ApplyTrainerEvolution(mon, NULL);
    sTestAbilities = NULL;
    sTestAbilitySpecies = SPECIES_NONE;
    return changed;
}
#endif
