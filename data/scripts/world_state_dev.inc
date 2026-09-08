@ Development-only World State tester. Remove before story implementation.

WorldStateDev_EventScript_Scientist::
	lock
	faceplayer
	msgbox WorldStateDev_Text_Intro, MSGBOX_DEFAULT
WorldStateDev_Menu:
	dynmultichoice 1, 1, FALSE, 5, 0, DYN_MULTICHOICE_CB_NONE, WorldStateDev_Text_View, WorldStateDev_Text_SetPhase, WorldStateDev_Text_Hoenn, WorldStateDev_Text_Proof, WorldStateDev_Text_Cancel
	switch VAR_RESULT
	case 0, WorldStateDev_View
	case 1, WorldStateDev_SetPhase
	case 2, WorldStateDev_Hoenn
	case 3, WorldStateDev_Proof
	goto WorldStateDev_End

WorldStateDev_View:
	specialvar VAR_RESULT, Script_GetWorldPhase
	special Script_BufferWorldPhaseName
	msgbox WorldStateDev_Text_Phase, MSGBOX_DEFAULT
	setvar VAR_0x8004, WORLD_REGION_HOENN
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	special Script_BufferWorldRegionName
	buffernumberstring STR_VAR_2, VAR_0x8006
	msgbox WorldStateDev_Text_Region, MSGBOX_DEFAULT
	setvar VAR_0x8004, WORLD_REGION_JOHTO
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	special Script_BufferWorldRegionName
	buffernumberstring STR_VAR_2, VAR_0x8006
	msgbox WorldStateDev_Text_Region, MSGBOX_DEFAULT
	setvar VAR_0x8004, WORLD_REGION_KANTO
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	special Script_BufferWorldRegionName
	buffernumberstring STR_VAR_2, VAR_0x8006
	msgbox WorldStateDev_Text_Region, MSGBOX_DEFAULT
	setvar VAR_0x8004, WORLD_REGION_SEVII
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	special Script_BufferWorldRegionName
	buffernumberstring STR_VAR_2, VAR_0x8006
	msgbox WorldStateDev_Text_Region, MSGBOX_DEFAULT
	setvar VAR_0x8004, WORLD_REGION_SINNOH
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	special Script_BufferWorldRegionName
	buffernumberstring STR_VAR_2, VAR_0x8006
	msgbox WorldStateDev_Text_Region, MSGBOX_DEFAULT
	goto WorldStateDev_End

WorldStateDev_SetPhase:
	dynmultichoice 1, 1, FALSE, 8, 0, DYN_MULTICHOICE_CB_NONE, WorldStateDev_Text_Beginning, WorldStateDev_Text_Rookie, WorldStateDev_Text_Rising, WorldStateDev_Text_Ace, WorldStateDev_Text_Elite, WorldStateDev_Text_Master, WorldStateDev_Text_Legend, WorldStateDev_Text_Cancel
	compare VAR_RESULT, WORLD_PHASE_COUNT
	goto_if_ge VAR_RESULT, WORLD_PHASE_COUNT, WorldStateDev_End
	copyvar VAR_0x8004, VAR_RESULT
	specialvar VAR_RESULT, Script_SetWorldPhase
	goto WorldStateDev_End

WorldStateDev_Hoenn:
	setvar VAR_0x8004, WORLD_REGION_HOENN
	specialvar VAR_RESULT, Script_GetRegionWorldState
	copyvar VAR_0x8006, VAR_RESULT
	dynmultichoice 1, 1, FALSE, 3, 0, DYN_MULTICHOICE_CB_NONE, WorldStateDev_Text_Increment, WorldStateDev_Text_Reset, WorldStateDev_Text_Cancel
	compare VAR_RESULT, 2
	goto_if_ge VAR_RESULT, 2, WorldStateDev_End
	compare VAR_RESULT, 1
	goto_if_eq VAR_RESULT, 1, WorldStateDev_HoennReset
	compare VAR_RESULT, 0
	goto_if_ne VAR_RESULT, 0, WorldStateDev_End
	addvar VAR_0x8006, 1
	copyvar VAR_0x8005, VAR_0x8006
	specialvar VAR_RESULT, Script_SetRegionWorldState
	goto WorldStateDev_End

WorldStateDev_HoennReset:
	setvar VAR_0x8005, 0
	specialvar VAR_RESULT, Script_SetRegionWorldState
	goto WorldStateDev_End

WorldStateDev_Proof:
	setvar VAR_0x8004, WORLD_REGION_HOENN
	specialvar VAR_RESULT, Script_GetRegionWorldState
	compare VAR_RESULT, 2
	goto_if_ge VAR_RESULT, 2, WorldStateDev_ProofHigh
	compare VAR_RESULT, 1
	goto_if_ge VAR_RESULT, 1, WorldStateDev_ProofChanged
	msgbox WorldStateDev_Text_ProofQuiet, MSGBOX_DEFAULT
	goto WorldStateDev_End

WorldStateDev_ProofChanged:
	msgbox WorldStateDev_Text_ProofChanged, MSGBOX_DEFAULT
	goto WorldStateDev_End

WorldStateDev_ProofHigh:
	msgbox WorldStateDev_Text_ProofHigh, MSGBOX_DEFAULT

WorldStateDev_End:
	release
	end

WorldStateDev_Text_Intro:
	.string "Development-only WORLD STATE tester.$"
WorldStateDev_Text_View:
	.string "View all states$"
WorldStateDev_Text_SetPhase:
	.string "Set global phase$"
WorldStateDev_Text_Hoenn:
	.string "Test Hoenn state$"
WorldStateDev_Text_Proof:
	.string "Hear Hoenn proof dialogue$"
WorldStateDev_Text_Cancel:
	.string "Cancel$"
WorldStateDev_Text_Phase:
	.string "Global World Phase: {STR_VAR_1}.$"
WorldStateDev_Text_Region:
	.string "{STR_VAR_1} state: {STR_VAR_2}.$"
WorldStateDev_Text_Beginning:
	.string "Beginning$"
WorldStateDev_Text_Rookie:
	.string "Rookie$"
WorldStateDev_Text_Rising:
	.string "Rising$"
WorldStateDev_Text_Ace:
	.string "Ace$"
WorldStateDev_Text_Elite:
	.string "Elite$"
WorldStateDev_Text_Master:
	.string "Master$"
WorldStateDev_Text_Legend:
	.string "Legend$"
WorldStateDev_Text_Increment:
	.string "Increment Hoenn state$"
WorldStateDev_Text_Reset:
	.string "Reset Hoenn state$"
WorldStateDev_Text_ProofQuiet:
	.string "The world is quiet.$"
WorldStateDev_Text_ProofChanged:
	.string "Something has changed in Hoenn.$"
WorldStateDev_Text_ProofHigh:
	.string "The situation is getting worse.$"
