#include "global.h"
#include "trainer_roster.h"
#include "pokemon.h"

#include "data/trainer_rosters.h"

const struct LARosterProfile *FindLARosterProfile(const struct LARosterAssignment *assignments,
    u32 count, u16 trainerId, enum DifficultyLevel difficulty)
{
    const struct LARosterProfile *exact = NULL, *any = NULL;
    u32 exactCount = 0, anyCount = 0;
    if ((count != 0 && assignments == NULL) || difficulty >= DIFFICULTY_COUNT
     || trainerId >= TRAINERS_COUNT || IsSpecialTrainer(trainerId) || IsPartnerTrainerId(trainerId))
        return NULL;
    for (u32 i = 0; i < count; i++)
    {
        const struct LARosterAssignment *entry = &assignments[i];
        if (entry->trainerId != trainerId)
            continue;
        if (entry->difficulty != LA_ROSTER_DIFFICULTY_ANY && entry->difficulty >= DIFFICULTY_COUNT)
            return NULL;
        if (entry->difficulty == difficulty)
        {
            exact = entry->profile;
            exactCount++;
        }
        else if (entry->difficulty == LA_ROSTER_DIFFICULTY_ANY)
        {
            any = entry->profile;
            anyCount++;
        }
    }
    if (exactCount != 0)
        return exactCount == 1 ? exact : NULL;
    return anyCount == 1 ? any : NULL;
}

const struct LARosterProfile *GetLARosterProfile(u16 trainerId, enum DifficultyLevel difficulty)
{
    return FindLARosterProfile(sLARosterAssignments, ARRAY_COUNT(sLARosterAssignments), trainerId, difficulty);
}

bool32 CanApplyLARoster(u16 trainerId, struct LATrainerPolicy policy,
    bool32 runtimeEligible, u32 battleTypeFlags, bool32 aiVsAi)
{
    // Allow only the ordinary controller flags. New/special formats fail closed.
    const u32 supportedFlags = BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE | BATTLE_TYPE_IS_MASTER;
    return trainerId < TRAINERS_COUNT && !IsSpecialTrainer(trainerId) && !IsPartnerTrainerId(trainerId)
        && runtimeEligible && policy.category == LA_TRAINER_ORDINARY
        && LATrainerPolicyHas(policy, LA_TRAINER_POLICY_FULL_PARTY)
        && !LATrainerPolicyHas(policy, LA_TRAINER_POLICY_HANDCRAFTED)
        && (battleTypeFlags & BATTLE_TYPE_TRAINER)
        && !(battleTypeFlags & ~supportedFlags) && !aiVsAi;
}

static bool32 ValidRosterSpecies(enum Species species)
{
    return species != SPECIES_NONE && species != SPECIES_EGG
        && species < NUM_SPECIES && IsSpeciesEnabled(species);
}

struct LARosterSelection SelectLARoster(const struct Trainer *trainer,
    const struct LARosterProfile *profile, u32 targetCount)
{
    struct LARosterSelection failed = {0};
    struct LARosterSelection result = {0};
    if (trainer == NULL || profile == NULL || targetCount == 0 || targetCount > PARTY_SIZE
     || profile->namespaceId == 0 || profile->namespaceId > 32767
     || (profile->retainedCount != 0 && (profile->retained == NULL || trainer->party == NULL))
     || (profile->supplementCount != 0 && profile->supplements == NULL))
        return failed;

    u32 sourceCount = trainer->poolSize != 0 ? trainer->poolSize : trainer->partySize;
    // Validate all metadata, including entries beyond the selected prefix.
    for (u32 i = 0; i < profile->retainedCount; i++)
    {
        const struct LARosterAuthoredRef *ref = &profile->retained[i];
        if (ref->sourceIndex >= sourceCount || !ValidRosterSpecies(ref->expectedSpecies)
         || trainer->party[ref->sourceIndex].species != ref->expectedSpecies)
            return failed;
        for (u32 j = 0; j < i; j++)
            if (profile->retained[j].sourceIndex == ref->sourceIndex)
                return failed;
    }
    for (u32 i = 0; i < profile->supplementCount; i++)
    {
        const struct LARosterSupplement *candidate = &profile->supplements[i];
        if (candidate->candidateId == 0 || !ValidRosterSpecies(candidate->mon.species)
         || candidate->mon.lvl == 0 || candidate->mon.lvl > MAX_LEVEL)
            return failed;
        for (u32 j = 0; j < i; j++)
            if (profile->supplements[j].candidateId == candidate->candidateId)
                return failed;
    }
    if ((u32)profile->retainedCount + profile->supplementCount < targetCount)
    {
        failed.status = LA_ROSTER_SELECTION_INCOMPLETE;
        return failed;
    }
    for (u32 i = 0; i < profile->retainedCount && result.count < targetCount; i++)
    {
        u32 index = profile->retained[i].sourceIndex;
        result.members[result.count++] = (struct LARosterSelectedMon){&trainer->party[index], index};
    }
    for (u32 i = 0; i < profile->supplementCount && result.count < targetCount; i++)
    {
        const struct LARosterSupplement *candidate = &profile->supplements[i];
        result.members[result.count++] = (struct LARosterSelectedMon){
            &candidate->mon, LA_ROSTER_SUPPLEMENT_KEY(profile->namespaceId, candidate->candidateId)};
    }
    result.status = LA_ROSTER_SELECTION_COMPLETE;
    return result;
}
