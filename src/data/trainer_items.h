// Provisional LA v0.6.0 finite consumables, in deterministic fill priority.
static const struct LATrainerItemSet sLATrainerItemTiers[] =
{
    {{ITEM_POTION, ITEM_POTION}},
    {{ITEM_SUPER_POTION, ITEM_FULL_HEAL}},
    {{ITEM_HYPER_POTION, ITEM_FULL_HEAL}},
    {{ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_FULL_HEAL}},
    {{ITEM_HYPER_POTION, ITEM_HYPER_POTION, ITEM_FULL_HEAL, ITEM_FULL_HEAL}},
};
