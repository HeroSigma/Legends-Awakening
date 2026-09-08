#ifndef GUARD_QUEST_SAVE_H
#define GUARD_QUEST_SAVE_H

#include "constants/quests.h"

// Serialized little-endian GBA layout. Keep offsets/capacity stable within v1.
// Stored in SaveBlock3; its native footer chunks lack a checksum of their own.
struct QuestSaveData
{
    u32 magic;
    u16 version;
    u16 checksum;
    u8 quests[QUEST_SAVE_CAPACITY]; // low two bits: state; bit 7: favorite
    u8 subquests[SUBQUEST_SAVE_CAPACITY / 8];
};

#endif // GUARD_QUEST_SAVE_H
