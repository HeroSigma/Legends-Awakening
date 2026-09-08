@ Development-only Field Log tester. Remove before story implementation.
@ One interaction activates, each subsequent interaction advances one child.
@ Reward-ready and reward-claimed deliberately require separate interactions.

FieldLogDev_EventScript_Scientist::
    lock
    faceplayer
    msgbox FieldLogDev_Text_Intro, MSGBOX_DEFAULT
    setvar VAR_0x8004, QUEST_FRAMEWORK_TEST
    setvar VAR_0x8005, QUEST_CMD_GET_STATE
    setvar VAR_0x8006, 0
    specialvar VAR_RESULT, Script_QuestCommand
    switch VAR_RESULT
    case QUEST_STATE_LOCKED, FieldLogDev_Start
    case QUEST_STATE_ACTIVE, FieldLogDev_Advance
    case QUEST_STATE_REWARD, FieldLogDev_Claim
    goto FieldLogDev_Tools

FieldLogDev_Start:
    setvar VAR_0x8005, QUEST_CMD_START
    specialvar VAR_RESULT, Script_QuestCommand
    goto_if_eq VAR_RESULT, FALSE, FieldLogDev_Unavailable
    msgbox FieldLogDev_Text_Started, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_Advance:
    specialvar VAR_RESULT, Script_FieldLogDevAdvance
    setvar VAR_0x8005, QUEST_CMD_REWARD_READY
    specialvar VAR_RESULT, Script_QuestCommand
    goto_if_eq VAR_RESULT, TRUE, FieldLogDev_Ready
    msgbox FieldLogDev_Text_Advanced, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_Ready:
    msgbox FieldLogDev_Text_Ready, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_Claim:
    setvar VAR_0x8005, QUEST_CMD_CLAIM_REWARD
    specialvar VAR_RESULT, Script_QuestCommand
    goto_if_eq VAR_RESULT, FALSE, FieldLogDev_BagFull
    msgbox FieldLogDev_Text_Reward, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_BagFull:
    msgbox FieldLogDev_Text_BagFull, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_Unavailable:
    msgbox FieldLogDev_Text_Unavailable, MSGBOX_DEFAULT
    goto FieldLogDev_ToolMenu

FieldLogDev_Tools:
    msgbox FieldLogDev_Text_Complete, MSGBOX_DEFAULT
FieldLogDev_ToolMenu:
    closemessage
    dynmultichoice 1, 1, FALSE, 4, 0, DYN_MULTICHOICE_CB_NONE, FieldLogDev_Text_Open, FieldLogDev_Text_Icon, FieldLogDev_Text_Reset, FieldLogDev_Text_Cancel
    switch VAR_RESULT
    case 0, FieldLogDev_Open
    case 1, FieldLogDev_Icon
    case 2, FieldLogDev_Reset
    goto FieldLogDev_End

FieldLogDev_Open:
    setvar VAR_0x8004, QUEST_FRAMEWORK_TEST
    special Script_OpenFieldLog
    goto FieldLogDev_End

FieldLogDev_Icon:
    setvar VAR_0x8004, QUEST_ICON_TEST
    setvar VAR_0x8005, QUEST_CMD_START
    specialvar VAR_RESULT, Script_QuestCommand
    msgbox FieldLogDev_Text_IconStarted, MSGBOX_DEFAULT
    goto FieldLogDev_End

FieldLogDev_Reset:
    msgbox FieldLogDev_Text_ResetConfirm, MSGBOX_YESNO
    goto_if_eq VAR_RESULT, NO, FieldLogDev_End
    special Script_FieldLogDevReset
    msgbox FieldLogDev_Text_ResetDone, MSGBOX_DEFAULT

FieldLogDev_End:
    release
    end

FieldLogDev_Text_Intro:
    .string "Development-only FIELD LOG tester.$"
FieldLogDev_Text_Started:
    .string "Framework Test started.\n"
    .string "Open FIELD LOG, then speak again.$"
FieldLogDev_Text_Advanced:
    .string "One test objective completed.\n"
    .string "Check FIELD LOG, then speak again.$"
FieldLogDev_Text_Ready:
    .string "All objectives complete! Reward ready.\n"
    .string "Check FIELD LOG or save before claiming.$"
FieldLogDev_Text_Reward:
    .string "Received one POTION. Test complete!$"
FieldLogDev_Text_BagFull:
    .string "Make room in your Bag, then return.\n"
    .string "Your reward is still available.$"
FieldLogDev_Text_Unavailable:
    .string "Quest data could not be updated.\n"
    .string "You can reset this development record.$"
FieldLogDev_Text_Complete:
    .string "Framework Test is complete.\n"
    .string "No further reward can be claimed.$"
FieldLogDev_Text_Open:
    .string "Open FIELD LOG$"
FieldLogDev_Text_Icon:
    .string "Activate Icon Test$"
FieldLogDev_Text_Reset:
    .string "Reset test quests$"
FieldLogDev_Text_Cancel:
    .string "Cancel$"
FieldLogDev_Text_IconStarted:
    .string "Inspect Icon Test in FIELD LOG.$"
FieldLogDev_Text_ResetConfirm:
    .string "Reset all development quest data?\n"
    .string "Your Trainer Rank will not change.$"
FieldLogDev_Text_ResetDone:
    .string "Development quests reset.$"
