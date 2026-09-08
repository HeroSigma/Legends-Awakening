#ifndef GUARD_QUESTS_H
#define GUARD_QUESTS_H

#include "global.h"
#include "main.h"
#include "constants/quests.h"

// Adapted from PokemonSanFran's Unbound Quest Menu SideQuest/SubQuest model.
struct SubQuest
{
    u16 id; // Globally unique persistent bit, not the index within the parent.
    const u8 *name;
    const u8 *desc;
    const u8 *map;
    u16 sprite;
    u8 spriteType;
};

struct SideQuest
{
    const u8 *name;
    const u8 *desc;
    const u8 *doneDesc;
    const u8 *objective;
    const u8 *map;
    const u8 *rewardText;
    const struct SubQuest *subquests;
    u16 sprite;
    u16 rewardItem;
    u16 rewardCount;
    u8 spriteType;
    u8 numSubquests;
    u8 category;
};

const struct SideQuest *QuestGetDefinition(u16 quest);
u8 QuestGetState(u16 quest);
bool32 QuestSetState(u16 quest, u16 state);
bool32 QuestStart(u16 quest);
bool32 QuestComplete(u16 quest);
bool32 QuestCompleteSubquest(u16 quest, u16 child);
bool32 QuestIsSubquestComplete(u16 quest, u16 child);
bool32 QuestAreAllSubquestsComplete(u16 quest);
bool32 QuestMarkRewardAvailable(u16 quest);
bool32 QuestClaimReward(u16 quest);
bool32 QuestIsFavorite(u16 quest);
bool32 QuestSetFavorite(u16 quest, bool32 favorite);
const u8 *QuestGetObjective(u16 quest);
const u8 *QuestGetStateName(u16 quest);
bool32 QuestSaveIsValid(void);

// Existing special command transport: 8004=quest, 8005=command, 8006=state/child.
u16 Script_QuestCommand(void);
void Script_OpenFieldLog(void);

// Development-only. Remove tester/special before story implementation.
void QuestDebugReset(void);
u16 Script_FieldLogDevAdvance(void);
void Script_FieldLogDevReset(void);

void QuestMenu_Init(u16 focusQuest, MainCallback callback);

#endif // GUARD_QUESTS_H
