#include "global.h"
#include "event_data.h"
#include "string_util.h"
#include "trainer_rank.h"

static const u8 sRankNameRookie[] = _("Rookie");
static const u8 sRankNameRising[] = _("Rising");
static const u8 sRankNameAce[] = _("Ace");
static const u8 sRankNameElite[] = _("Elite");
static const u8 sRankNameMaster[] = _("Master");
static const u8 sRankNameLegend[] = _("Legend");

static const u8 *const sTrainerRankNames[TRAINER_RANK_COUNT] =
{
    [TRAINER_RANK_ROOKIE] = sRankNameRookie,
    [TRAINER_RANK_RISING] = sRankNameRising,
    [TRAINER_RANK_ACE] = sRankNameAce,
    [TRAINER_RANK_ELITE] = sRankNameElite,
    [TRAINER_RANK_MASTER] = sRankNameMaster,
    [TRAINER_RANK_LEGEND] = sRankNameLegend,
};

u8 GetTrainerRank(void)
{
    u16 rank = VarGet(VAR_TRAINER_RANK);

    // Validate the full saved value before narrowing; reads never repair saves.
    if (rank >= TRAINER_RANK_COUNT)
        return TRAINER_RANK_ROOKIE;
    return rank;
}

bool32 SetTrainerRank(u8 rank)
{
    if (rank >= TRAINER_RANK_COUNT)
        return FALSE;

    VarSet(VAR_TRAINER_RANK, rank);
    return TRUE;
}

bool32 IsTrainerRankAtLeast(u8 rank)
{
    return rank < TRAINER_RANK_COUNT && GetTrainerRank() >= rank;
}

const u8 *GetTrainerRankName(u8 rank)
{
    if (rank >= TRAINER_RANK_COUNT)
        rank = TRAINER_RANK_ROOKIE;
    return sTrainerRankNames[rank];
}

// Returns the current rank to specialvar and buffers its display name.
u16 Script_GetTrainerRank(void)
{
    u8 rank = GetTrainerRank();

    StringCopy(gStringVar1, GetTrainerRankName(rank));
    return rank;
}

// Development-only direct setter, including demotion. Do not use for story
// promotions; remove the temporary tester before story implementation.
u16 Script_SetTrainerRank(void)
{
    // Script arguments are u16: reject invalid values before u8 conversion.
    if (gSpecialVar_0x8004 >= TRAINER_RANK_COUNT)
        return FALSE;
    return SetTrainerRank(gSpecialVar_0x8004);
}

void Script_BufferTrainerRankName(void)
{
    u16 rank = gSpecialVar_0x8004;

    if (rank >= TRAINER_RANK_COUNT)
        rank = TRAINER_RANK_ROOKIE;
    StringCopy(gStringVar1, GetTrainerRankName(rank));
}
