#include "global.h"
#include "trainer_sets.h"
#include "trainer_roster.h"
#include "pokemon.h"
#include "move.h"
#include "item.h"

#include "data/trainer_sets.h"

static bool32 ValidSetSpecies(enum Species species)
{
    return species != SPECIES_NONE && species != SPECIES_EGG
        && species < NUM_SPECIES && IsSpeciesEnabled(species);
}

bool32 CanApplyLACompetitiveSets(u16 trainerId, struct LATrainerPolicy policy,
    bool32 runtimeEligible, u32 battleTypeFlags, bool32 aiVsAi)
{
    const u32 supported = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_IS_MASTER;
    return trainerId < TRAINERS_COUNT && !IsPartnerTrainerId(trainerId) && !IsSpecialTrainer(trainerId)
        && runtimeEligible && policy.category == LA_TRAINER_ORDINARY
        && LATrainerPolicyHas(policy, LA_TRAINER_POLICY_COMPETITIVE_SETS)
        && !LATrainerPolicyHas(policy, LA_TRAINER_POLICY_HANDCRAFTED)
        && (battleTypeFlags & BATTLE_TYPE_TRAINER) && !(battleTypeFlags & ~supported) && !aiVsAi;
}

const struct LASetBundle *FindLASetBundle(const struct LASetAssignment *assignments,
    u32 count, u16 trainerId, enum DifficultyLevel difficulty, u32 sourceKey, enum Species authoredSpecies)
{
    const struct LASetAssignment *exact = NULL, *any = NULL;
    u32 exactCount = 0, anyCount = 0;
    if ((count != 0 && assignments == NULL) || difficulty >= DIFFICULTY_COUNT
     || trainerId >= TRAINERS_COUNT || IsPartnerTrainerId(trainerId) || IsSpecialTrainer(trainerId))
        return NULL;
    for (u32 i = 0; i < count; i++)
    {
        const struct LASetAssignment *row = &assignments[i];
        if (row->trainerId != trainerId || row->sourceKey != sourceKey)
            continue;
        if (row->difficulty != LA_SET_DIFFICULTY_ANY && row->difficulty >= DIFFICULTY_COUNT)
            return NULL;
        if (row->difficulty == difficulty)
        {
            exact = row;
            exactCount++;
        }
        else if (row->difficulty == LA_SET_DIFFICULTY_ANY)
        {
            any = row;
            anyCount++;
        }
    }
    // Resolve authority BEFORE checking the authored-species guard. A stale
    // exact row must not fall through to an ANY assignment.
    const struct LASetAssignment *selected = NULL;
    if (exactCount != 0)
    {
        if (exactCount != 1)
            return NULL;
        selected = exact;
    }
    else if (anyCount == 1)
        selected = any;
    if (selected == NULL || !ValidSetSpecies(authoredSpecies) || selected->authoredSpecies != authoredSpecies)
        return NULL;
    return selected->bundle;
}

const struct LASetBundle *GetLASetBundle(u16 trainerId, enum DifficultyLevel difficulty,
    u32 sourceKey, enum Species authoredSpecies)
{
    return FindLASetBundle(sLASetAssignments, ARRAY_COUNT(sLASetAssignments),
        trainerId, difficulty, sourceKey, authoredSpecies);
}

bool32 LASetSpeciesCanLearnMove(enum Species species, enum Move move)
{
    if (!ValidSetSpecies(species) || move == MOVE_NONE || move >= MOVES_COUNT
     || gMovesInfo[move].name == NULL || gMovesInfo[move].pp == 0
     || move == MOVE_RETURN || move == MOVE_FRUSTRATION)
        return FALSE;
    const struct LevelUpMove *levels = GetSpeciesLevelUpLearnset(species);
    if (levels != NULL)
        for (u32 i = 0; levels[i].move != LEVEL_UP_MOVE_END; i++)
            if (levels[i].move == move)
                return TRUE; // Species-based legality; intentionally no level test.
    const u16 *lists[] = {GetSpeciesTeachableLearnset(species), GetSpeciesEggMoves(species)};
    for (u32 list = 0; list < ARRAY_COUNT(lists); list++)
        if (lists[list] != NULL)
            for (u32 i = 0; lists[list][i] != MOVE_UNAVAILABLE; i++)
                if (lists[list][i] == move)
                    return TRUE;
    return FALSE;
}

static bool32 ValidSetItem(enum Species species, enum Item item)
{
    if (item == ITEM_NONE || item >= ITEMS_COUNT || gItemsInfo[item].name == NULL
     || gItemsInfo[item].holdEffect == 0)
        return FALSE;
    // Explicit ordinary-content allowlist. New items require reviewed content;
    // this never admits Mega Stones, Z-Crystals, primal orbs or Everstone.
    switch (item)
    {
    case ITEM_LEFTOVERS:
    case ITEM_SITRUS_BERRY:
    case ITEM_LUM_BERRY:
    case ITEM_SILK_SCARF:
    case ITEM_BLACK_BELT:
    case ITEM_SHARP_BEAK:
    case ITEM_MYSTIC_WATER:
    case ITEM_MAGNET:
    case ITEM_BLACK_GLASSES:
    case ITEM_TWISTED_SPOON:
    case ITEM_METAL_COAT:
        return TRUE;
    default:
        break;
    }
    if (item == ITEM_EVIOLITE)
    {
        const struct Evolution *evos = GetSpeciesEvolutions(species);
        if (evos != NULL)
            for (u32 i = 0; evos[i].method != EVOLUTIONS_END; i++)
                if (evos[i].method != EVO_NONE && ValidSetSpecies(evos[i].targetSpecies)
                 && evos[i].targetSpecies != species)
                    return TRUE;
    }
    return FALSE;
}

bool32 ApplyLACompetitiveSet(struct TrainerMon *workingMon, const struct LASetBundle *bundle)
{
    if (workingMon == NULL || bundle == NULL || bundle->variants == NULL || bundle->count == 0
     || !ValidSetSpecies(workingMon->species))
        return FALSE;
    const struct LACompetitiveSet *set = NULL;
    for (u32 i = 0; i < bundle->count; i++)
    {
        const struct LACompetitiveSet *candidate = bundle->variants[i];
        if (candidate == NULL || !ValidSetSpecies(candidate->finalSpecies))
            return FALSE;
        for (u32 j = 0; j < i; j++)
            if (bundle->variants[j]->finalSpecies == candidate->finalSpecies)
                return FALSE;
        if (candidate->finalSpecies == workingMon->species)
            set = candidate;
    }
    if (set == NULL || set->training == NULL || set->training->nature >= NUM_NATURES
     || set->abilitySlot >= NUM_ABILITY_SLOTS)
        return FALSE;
    u32 total = 0;
    for (u32 i = 0; i < ARRAY_COUNT(set->training->evs); i++)
    {
        if (set->training->evs[i] > 252)
            return FALSE;
        total += set->training->evs[i];
    }
    if (total > 510)
        return FALSE;
    enum Ability ability = gSpeciesInfo[workingMon->species].abilities[set->abilitySlot];
    if (ability == ABILITY_NONE || ability >= ABILITIES_COUNT || gAbilitiesInfo[ability].description == NULL)
        return FALSE;
    for (u32 i = 0; i < MAX_MON_MOVES; i++)
    {
        if (!LASetSpeciesCanLearnMove(workingMon->species, set->moves[i]))
            return FALSE;
        for (u32 j = 0; j < i; j++)
            if (set->moves[j] == set->moves[i])
                return FALSE;
    }
    if (!ValidSetItem(workingMon->species, set->heldItem))
        return FALSE;

    struct TrainerMon result = *workingMon;
    result.iv = LA_SET_PERFECT_IVS;
    result.ev = set->training->evs;
    result.nature = set->training->nature;
    result.ability = ability;
    memcpy(result.moves, set->moves, sizeof(result.moves));
    result.heldItem = set->heldItem;
    *workingMon = result;
    return TRUE;
}
