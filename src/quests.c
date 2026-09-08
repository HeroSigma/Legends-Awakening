// LA compatibility adaptation of PokemonSanFran's Unbound Quest Menu.
// Basis: c34ebdd80f78f751edbac83ee10d7fc4f0273746 (unbound-quest-menu).
#include "global.h"
#include "quests.h"
#include "event_data.h"
#include "item.h"
#include "string_util.h"
#include "constants/event_objects.h"
#include "constants/items.h"
#include "constants/species.h"

#include "data/quests.h"

#define QUEST_FAVORITE 0x80
#define QUEST_STATE_MASK 0x03

STATIC_ASSERT(sizeof(struct QuestSaveData) == 104, QuestSaveV1Size);
STATIC_ASSERT(QUEST_COUNT <= QUEST_SAVE_CAPACITY, QuestDefinitionCapacity);

static const u8 sStateLocked[] = _("Locked");
static const u8 sStateActive[] = _("Active");
static const u8 sStateReward[] = _("Reward Available");
static const u8 sStateComplete[] = _("Complete");
static const u8 *const sStateNames[] =
{
    sStateLocked, sStateActive, sStateReward, sStateComplete,
};

// Fletcher checksum covers the fixed record except the checksum itself.
static u16 QuestChecksum(const struct QuestSaveData *save)
{
    const u8 *bytes = (const u8 *)save;
    u32 i, a = 0, b = 0;
    for (i = 0; i < sizeof(*save); i++)
    {
        if (i == offsetof(struct QuestSaveData, checksum)
         || i == offsetof(struct QuestSaveData, checksum) + 1)
            continue;
        a = (a + bytes[i]) % 255;
        b = (b + a) % 255;
    }
    return (b << 8) | a;
}

bool32 QuestSaveIsValid(void)
{
    const struct QuestSaveData *save = &gSaveBlock3Ptr->quests;
    return save->magic == QUEST_SAVE_MAGIC && save->version == QUEST_SAVE_VERSION
        && save->checksum == QuestChecksum(save);
}

static void CommitQuestData(void)
{
    gSaveBlock3Ptr->quests.checksum = QuestChecksum(&gSaveBlock3Ptr->quests);
}

static bool32 PrepareQuestWrite(void)
{
    struct QuestSaveData *save = &gSaveBlock3Ptr->quests;
    if (save->magic != QUEST_SAVE_MAGIC)
    {
        // Baseline saves have no record. Initialize only on an explicit write.
        memset(save, 0, sizeof(*save));
        save->magic = QUEST_SAVE_MAGIC;
        save->version = QUEST_SAVE_VERSION;
        CommitQuestData();
    }
    // Preserve unknown future versions and damaged recognized records.
    return QuestSaveIsValid();
}

const struct SideQuest *QuestGetDefinition(u16 quest)
{
    if (quest >= QUEST_COUNT)
        return NULL;
    return &sSideQuests[quest];
}

u8 QuestGetState(u16 quest)
{
    if (QuestGetDefinition(quest) == NULL || !QuestSaveIsValid())
        return QUEST_STATE_LOCKED;
    if (gSaveBlock3Ptr->quests.quests[quest] & ~(QUEST_STATE_MASK | QUEST_FAVORITE))
        return QUEST_STATE_LOCKED;
    return gSaveBlock3Ptr->quests.quests[quest] & QUEST_STATE_MASK;
}

bool32 QuestSetState(u16 quest, u16 state)
{
    u8 oldState;
    if (QuestGetDefinition(quest) == NULL || state >= QUEST_STATE_COUNT)
        return FALSE;
    oldState = QuestGetState(quest);
    // Normal script state changes cannot reopen a claimed reward.
    if (state < oldState || !PrepareQuestWrite())
        return FALSE;
    gSaveBlock3Ptr->quests.quests[quest] =
        (gSaveBlock3Ptr->quests.quests[quest] & QUEST_FAVORITE) | state;
    CommitQuestData();
    return TRUE;
}

bool32 QuestStart(u16 quest)
{
    return QuestSetState(quest, QUEST_STATE_ACTIVE);
}

bool32 QuestComplete(u16 quest)
{
    // Explicit script completion, without granting an item.
    return QuestSetState(quest, QUEST_STATE_COMPLETE);
}

static const struct SubQuest *GetSubquest(u16 quest, u16 child)
{
    const struct SideQuest *parent = QuestGetDefinition(quest);
    if (parent == NULL || child >= parent->numSubquests || parent->subquests == NULL
     || parent->subquests[child].id >= SUBQUEST_SAVE_CAPACITY)
        return NULL;
    return &parent->subquests[child];
}

bool32 QuestIsSubquestComplete(u16 quest, u16 child)
{
    const struct SubQuest *sub = GetSubquest(quest, child);
    if (sub == NULL || !QuestSaveIsValid() || QuestGetState(quest) == QUEST_STATE_LOCKED)
        return FALSE;
    return (gSaveBlock3Ptr->quests.subquests[sub->id / 8] & (1 << (sub->id % 8))) != 0;
}

bool32 QuestCompleteSubquest(u16 quest, u16 child)
{
    const struct SubQuest *sub = GetSubquest(quest, child);
    if (sub == NULL || QuestGetState(quest) != QUEST_STATE_ACTIVE || !PrepareQuestWrite())
        return FALSE;
    gSaveBlock3Ptr->quests.subquests[sub->id / 8] |= 1 << (sub->id % 8);
    CommitQuestData();
    return TRUE;
}

bool32 QuestAreAllSubquestsComplete(u16 quest)
{
    const struct SideQuest *parent = QuestGetDefinition(quest);
    u32 i;
    if (parent == NULL || QuestGetState(quest) == QUEST_STATE_LOCKED)
        return FALSE;
    for (i = 0; i < parent->numSubquests; i++)
        if (!QuestIsSubquestComplete(quest, i))
            return FALSE;
    return TRUE;
}

bool32 QuestMarkRewardAvailable(u16 quest)
{
    if (QuestGetState(quest) != QUEST_STATE_ACTIVE || !QuestAreAllSubquestsComplete(quest))
        return FALSE;
    return QuestSetState(quest, QUEST_STATE_REWARD);
}

bool32 QuestClaimReward(u16 quest)
{
    const struct SideQuest *def = QuestGetDefinition(quest);
    if (def == NULL || QuestGetState(quest) != QUEST_STATE_REWARD || !PrepareQuestWrite())
        return FALSE;
    // AddBagItem is atomic on failure. A full bag leaves the reward available.
    if (def->rewardCount != 0 && !AddBagItem(def->rewardItem, def->rewardCount))
        return FALSE;
    return QuestSetState(quest, QUEST_STATE_COMPLETE);
}

bool32 QuestIsFavorite(u16 quest)
{
    return QuestGetState(quest) != QUEST_STATE_LOCKED
        && (gSaveBlock3Ptr->quests.quests[quest] & QUEST_FAVORITE) != 0;
}

bool32 QuestSetFavorite(u16 quest, bool32 favorite)
{
    if (QuestGetState(quest) == QUEST_STATE_LOCKED || !PrepareQuestWrite())
        return FALSE;
    if (favorite)
        gSaveBlock3Ptr->quests.quests[quest] |= QUEST_FAVORITE;
    else
        gSaveBlock3Ptr->quests.quests[quest] &= ~QUEST_FAVORITE;
    CommitQuestData();
    return TRUE;
}

const u8 *QuestGetObjective(u16 quest)
{
    const struct SideQuest *def = QuestGetDefinition(quest);
    u32 i;
    switch (QuestGetState(quest))
    {
    case QUEST_STATE_ACTIVE:
        for (i = 0; i < def->numSubquests; i++)
            if (!QuestIsSubquestComplete(quest, i))
                return def->subquests[i].name;
        return def->numSubquests != 0 ? sReturnForReward : def->objective;
    case QUEST_STATE_REWARD:
        return sReturnForReward;
    case QUEST_STATE_COMPLETE:
        return sDoneObjective;
    default:
        return sNoObjective;
    }
}

const u8 *QuestGetStateName(u16 quest)
{
    return sStateNames[QuestGetState(quest)];
}

u16 Script_QuestCommand(void)
{
    u16 quest = gSpecialVar_0x8004;
    u16 arg = gSpecialVar_0x8006;
    switch (gSpecialVar_0x8005)
    {
    case QUEST_CMD_START: return QuestStart(quest);
    case QUEST_CMD_GET_STATE: return QuestGetState(quest);
    case QUEST_CMD_SET_STATE: return QuestSetState(quest, arg);
    case QUEST_CMD_COMPLETE_SUB: return QuestCompleteSubquest(quest, arg);
    case QUEST_CMD_CHECK_SUB: return QuestIsSubquestComplete(quest, arg);
    case QUEST_CMD_REWARD_READY: return QuestMarkRewardAvailable(quest);
    case QUEST_CMD_CLAIM_REWARD: return QuestClaimReward(quest);
    case QUEST_CMD_COMPLETE: return QuestComplete(quest);
    default: return FALSE;
    }
}

// Development-only reset and advancement; remove before story implementation.
void QuestDebugReset(void)
{
    memset(&gSaveBlock3Ptr->quests, 0, sizeof(gSaveBlock3Ptr->quests));
}

void Script_FieldLogDevReset(void)
{
    QuestDebugReset();
}

u16 Script_FieldLogDevAdvance(void)
{
    u16 i;
    const struct SideQuest *def = QuestGetDefinition(QUEST_FRAMEWORK_TEST);
    if (QuestGetState(QUEST_FRAMEWORK_TEST) != QUEST_STATE_ACTIVE)
        return FALSE;
    for (i = 0; i < def->numSubquests; i++)
        if (!QuestIsSubquestComplete(QUEST_FRAMEWORK_TEST, i))
            return QuestCompleteSubquest(QUEST_FRAMEWORK_TEST, i);
    return FALSE;
}
