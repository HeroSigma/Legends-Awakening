#ifndef GUARD_CONSTANTS_QUESTS_H
#define GUARD_CONSTANTS_QUESTS_H

// Stable IDs: append definitions; never renumber shipped quests or subquests.
#define QUEST_FRAMEWORK_TEST 0
#define QUEST_ICON_TEST      1
#define QUEST_COUNT          2
#define QUEST_NONE           0xFFFF

#define QUEST_STATE_LOCKED   0
#define QUEST_STATE_ACTIVE   1
#define QUEST_STATE_REWARD   2
#define QUEST_STATE_COMPLETE 3
#define QUEST_STATE_COUNT    4

#define QUEST_CATEGORY_MAIN       0
#define QUEST_CATEGORY_REGIONAL   1
#define QUEST_CATEGORY_CHARACTER  2
#define QUEST_CATEGORY_EXPLORATION 3
#define QUEST_CATEGORY_FACTION    4
#define QUEST_CATEGORY_GYM        5
#define QUEST_CATEGORY_DEV        6

#define QUEST_ICON_NPC     0
#define QUEST_ICON_ITEM    1
#define QUEST_ICON_POKEMON 2

#define QUEST_CMD_START          0
#define QUEST_CMD_GET_STATE      1
#define QUEST_CMD_SET_STATE      2
#define QUEST_CMD_COMPLETE_SUB   3
#define QUEST_CMD_CHECK_SUB      4
#define QUEST_CMD_REWARD_READY   5
#define QUEST_CMD_CLAIM_REWARD   6
#define QUEST_CMD_COMPLETE      7

// Format capacities are independent of the number of ROM definitions.
#define QUEST_SAVE_CAPACITY     64
#define SUBQUEST_SAVE_CAPACITY  256
#define QUEST_SAVE_VERSION       1

#endif // GUARD_CONSTANTS_QUESTS_H
