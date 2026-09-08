#include "global.h"
#include "event_data.h"
#include "string_util.h"
#include "world_state.h"

static const u8 sWorldPhaseBeginning[] = _("Beginning");
static const u8 sWorldPhaseRookie[] = _("Rookie");
static const u8 sWorldPhaseRising[] = _("Rising");
static const u8 sWorldPhaseAce[] = _("Ace");
static const u8 sWorldPhaseElite[] = _("Elite");
static const u8 sWorldPhaseMaster[] = _("Master");
static const u8 sWorldPhaseLegend[] = _("Legend");

static const u8 *const sWorldPhaseNames[WORLD_PHASE_COUNT] =
{
    [WORLD_PHASE_BEGINNING] = sWorldPhaseBeginning,
    [WORLD_PHASE_ROOKIE] = sWorldPhaseRookie,
    [WORLD_PHASE_RISING] = sWorldPhaseRising,
    [WORLD_PHASE_ACE] = sWorldPhaseAce,
    [WORLD_PHASE_ELITE] = sWorldPhaseElite,
    [WORLD_PHASE_MASTER] = sWorldPhaseMaster,
    [WORLD_PHASE_LEGEND] = sWorldPhaseLegend,
};

static const u8 sWorldRegionHoenn[] = _("Hoenn");
static const u8 sWorldRegionJohto[] = _("Johto");
static const u8 sWorldRegionKanto[] = _("Kanto");
static const u8 sWorldRegionSevii[] = _("Sevii");
static const u8 sWorldRegionSinnoh[] = _("Sinnoh");
static const u8 sWorldRegionUnknown[] = _("Unknown");

static const u8 *const sWorldRegionNames[WORLD_REGION_COUNT] =
{
    [WORLD_REGION_HOENN] = sWorldRegionHoenn,
    [WORLD_REGION_JOHTO] = sWorldRegionJohto,
    [WORLD_REGION_KANTO] = sWorldRegionKanto,
    [WORLD_REGION_SEVII] = sWorldRegionSevii,
    [WORLD_REGION_SINNOH] = sWorldRegionSinnoh,
};

static const u16 sWorldStateVars[WORLD_REGION_COUNT] =
{
    [WORLD_REGION_HOENN] = VAR_WORLD_STATE_HOENN,
    [WORLD_REGION_JOHTO] = VAR_WORLD_STATE_JOHTO,
    [WORLD_REGION_KANTO] = VAR_WORLD_STATE_KANTO,
    [WORLD_REGION_SEVII] = VAR_WORLD_STATE_SEVII,
    [WORLD_REGION_SINNOH] = VAR_WORLD_STATE_SINNOH,
};

static bool32 IsValidRegion(u8 region)
{
    return region < WORLD_REGION_COUNT;
}

u16 GetWorldPhase(void)
{
    u16 phase = VarGet(VAR_WORLD_PHASE);

    if (phase >= WORLD_PHASE_COUNT)
        return WORLD_PHASE_BEGINNING;
    return phase;
}

bool32 SetWorldPhase(u16 phase)
{
    if (phase >= WORLD_PHASE_COUNT)
        return FALSE;

    VarSet(VAR_WORLD_PHASE, phase);
    return TRUE;
}

bool32 IsWorldPhaseAtLeast(u16 phase)
{
    return phase < WORLD_PHASE_COUNT && GetWorldPhase() >= phase;
}

u16 GetRegionWorldState(u8 region)
{
    if (!IsValidRegion(region))
        return 0;
    return VarGet(sWorldStateVars[region]);
}

bool32 SetRegionWorldState(u8 region, u16 state)
{
    if (!IsValidRegion(region))
        return FALSE;

    VarSet(sWorldStateVars[region], state);
    return TRUE;
}

bool32 IsRegionWorldStateAtLeast(u8 region, u16 state)
{
    return IsValidRegion(region) && GetRegionWorldState(region) >= state;
}

const u8 *GetWorldPhaseName(u16 phase)
{
    if (phase >= WORLD_PHASE_COUNT)
        phase = WORLD_PHASE_BEGINNING;
    return sWorldPhaseNames[phase];
}

const u8 *GetWorldRegionName(u8 region)
{
    if (!IsValidRegion(region))
        return sWorldRegionUnknown;
    return sWorldRegionNames[region];
}

u16 Script_GetWorldPhase(void)
{
    return GetWorldPhase();
}

u16 Script_SetWorldPhase(void)
{
    return SetWorldPhase(gSpecialVar_0x8004);
}

u16 Script_GetRegionWorldState(void)
{
    if (gSpecialVar_0x8004 >= WORLD_REGION_COUNT)
        return 0;
    return GetRegionWorldState(gSpecialVar_0x8004);
}

u16 Script_SetRegionWorldState(void)
{
    if (gSpecialVar_0x8004 >= WORLD_REGION_COUNT)
        return FALSE;
    return SetRegionWorldState(gSpecialVar_0x8004, gSpecialVar_0x8005);
}

u16 Script_IsWorldPhaseAtLeast(void)
{
    return IsWorldPhaseAtLeast(gSpecialVar_0x8004);
}

u16 Script_IsRegionWorldStateAtLeast(void)
{
    if (gSpecialVar_0x8004 >= WORLD_REGION_COUNT)
        return FALSE;
    return IsRegionWorldStateAtLeast(gSpecialVar_0x8004, gSpecialVar_0x8005);
}

void Script_BufferWorldPhaseName(void)
{
    StringCopy(gStringVar1, GetWorldPhaseName(GetWorldPhase()));
}

void Script_BufferWorldRegionName(void)
{
    if (gSpecialVar_0x8004 >= WORLD_REGION_COUNT)
        StringCopy(gStringVar1, sWorldRegionUnknown);
    else
        StringCopy(gStringVar1, GetWorldRegionName(gSpecialVar_0x8004));
}
