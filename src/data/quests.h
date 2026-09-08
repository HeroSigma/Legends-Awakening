// Development-only definitions. Not Legends Awakening story canon.
// Remove/replace before story implementation, preserving any shipped save IDs.
static const u8 sDevLocation[] = _("Littleroot");
static const u8 sNoObjective[] = _("No current objective.");
static const u8 sReturnForReward[] = _("Return to the test scientist.");
static const u8 sDoneObjective[] = _("Test complete.");
static const u8 sDevName[] = _("Framework Test");
static const u8 sDevDesc[] = _("Verify Field Log functionality.");
static const u8 sDevDone[] = _("All framework checks complete.");
static const u8 sDevReward[] = _("Reward: 1 POTION");
static const u8 sSubName0[] = _("Speak to the scientist");
static const u8 sSubName1[] = _("Advance an objective");
static const u8 sSubName2[] = _("Speak to the scientist again");
static const u8 sSubDesc0[] = _("Speak to the scientist once.");
static const u8 sSubDesc1[] = _("Speak to the scientist again.");
static const u8 sSubDesc2[] = _("Speak to the scientist once more.");
static const struct SubQuest sFrameworkSubquests[] =
{
    {0, sSubName0, sSubDesc0, sDevLocation, OBJ_EVENT_GFX_SCIENTIST_1, QUEST_ICON_NPC},
    {1, sSubName1, sSubDesc1, sDevLocation, SPECIES_TREECKO, QUEST_ICON_POKEMON},
    {2, sSubName2, sSubDesc2, sDevLocation, ITEM_POTION, QUEST_ICON_ITEM},
};
static const u8 sIconName[] = _("Icon Test");
static const u8 sIconDesc[] = _("Development-only item icon check.");
static const u8 sIconDone[] = _("Item icon check complete.");
static const u8 sIconObjective[] = _("Inspect the POTION icon.");
static const u8 sIconReward[] = _("Reward: none (development test)");

static const struct SideQuest sSideQuests[QUEST_COUNT] =
{
    [QUEST_FRAMEWORK_TEST] =
    {
        .name = sDevName, .desc = sDevDesc, .doneDesc = sDevDone,
        .objective = sSubName0, .map = sDevLocation, .rewardText = sDevReward,
        .subquests = sFrameworkSubquests, .numSubquests = ARRAY_COUNT(sFrameworkSubquests),
        .sprite = OBJ_EVENT_GFX_SCIENTIST_1, .spriteType = QUEST_ICON_NPC,
        .rewardItem = ITEM_POTION, .rewardCount = 1, .category = QUEST_CATEGORY_DEV,
    },
    [QUEST_ICON_TEST] =
    {
        .name = sIconName, .desc = sIconDesc, .doneDesc = sIconDone,
        .objective = sIconObjective, .map = sDevLocation, .rewardText = sIconReward,
        .sprite = ITEM_POTION, .spriteType = QUEST_ICON_ITEM,
        .category = QUEST_CATEGORY_DEV,
    },
};
