#include "global.h"
#include "data.h"
#include "constants/trainers.h"
#include "constants/battle_ai.h"

#include "test/test.h"
#include "la_trainer.h"
#include "battle.h"
#include "battle_setup.h"
#include "battle_ai_main.h"
#include "battle_ai_switch.h"
#include "debug.h"

extern u64 TestGetLATrainerAIFlags(u16 trainerId, enum BattlerId battler);
extern AiScoreFunc sDynamicAiFunc;

// The TESTING build uses a tiny trainer table from test/battle/trainer_control.h
// (see test/test_runner_battle.c:57-60). Relevant populated rows:
//   [0]      TRAINER_CLASS_PKMN_TRAINER_1   (ORDINARY baseline)
//   [1]      TRAINER_CLASS_RIVAL           (MAJOR via class)
//   [2]      TRAINER_CLASS_RIVAL
//   [3]      TRAINER_CLASS_PKMN_TRAINER_1
//   [5]      TRAINER_CLASS_PKMN_TRAINER_1   (difficulty variants)
// Valid-but-unclassified ordinary ids in the test table: 0, 3, 6..14.

TEST("LA Trainer: valid ordinary trainer resolves to ORDINARY competitive preset")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);
    ASSUME(p.category == LA_TRAINER_ORDINARY);
    EXPECT(LATrainerPolicyHasAll(p, LA_POLICY_ORDINARY));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_LEVEL));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_EVOLUTION));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_SMART_AI));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_FULL_PARTY));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_COMPETITIVE_ITEMS));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_EXEMPT));
}

TEST("LA Trainer: Rival class resolves to MAJOR with HANDCRAFTED and no SCALE_EVOLUTION")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(1);  // RIVAL class
    ASSUME(p.category == LA_TRAINER_MAJOR);
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_EVOLUTION));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_LEVEL));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_SMART_AI));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_FULL_PARTY));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_COMPETITIVE_ITEMS));
}

TEST("LA Trainer: Leader class mapping resolves to MAJOR")
{
    // No leader-class trainer id exists in the TESTING roster, so test the
    // class->category mapping directly.
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_LEADER) == LA_TRAINER_MAJOR);
}

TEST("LA Trainer: Elite Four class mapping resolves to MAJOR")
{
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_ELITE_FOUR) == LA_TRAINER_MAJOR);
}

TEST("LA Trainer: Champion class mapping resolves to MAJOR")
{
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_CHAMPION) == LA_TRAINER_MAJOR);
}

TEST("LA Trainer: Rival class trainer resolves to MAJOR")
{
    // id 1 (or 2) in the TESTING table has RIVAL class -> MAJOR via class.
    struct LATrainerPolicy p = GetLATrainerPolicy(1);
    EXPECT(p.category == LA_TRAINER_MAJOR);
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_EVOLUTION));
}

TEST("LA Trainer: frontier boss classes resolve to EXEMPT")
{
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_SALON_MAIDEN) == LA_TRAINER_EXEMPT);
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_DOME_ACE) == LA_TRAINER_EXEMPT);
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_PIKE_QUEEN) == LA_TRAINER_EXEMPT);
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_PYRAMID_KING) == LA_TRAINER_EXEMPT);
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_FACTORY_HEAD) == LA_TRAINER_EXEMPT);
}

TEST("LA Trainer: Wally explicit ID override marks a non-class major -> MAJOR")
{
    // Wally's class is TRAINER_CLASS_PKMN_TRAINER_1 (ordinary in the test table:
    // id 0/3 are PKMN_TRAINER_1 and resolve ORDINARY). The override changes Wally.
    EXPECT(GetLATrainerCategoryForClass(TRAINER_CLASS_PKMN_TRAINER_1) == LA_TRAINER_ORDINARY);

    struct LATrainerPolicy p = GetLATrainerPolicy(TRAINER_WALLY_VR_1);
    EXPECT(p.category == LA_TRAINER_MAJOR);
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR));
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_EVOLUTION));
}

TEST("LA Trainer: all Wally story instances are MAJOR overrides")
{
    struct LATrainerPolicy p1 = GetLATrainerPolicy(TRAINER_WALLY_MAUVILLE);
    struct LATrainerPolicy p2 = GetLATrainerPolicy(TRAINER_WALLY_VR_2);
    struct LATrainerPolicy p3 = GetLATrainerPolicy(TRAINER_WALLY_VR_3);
    struct LATrainerPolicy p4 = GetLATrainerPolicy(TRAINER_WALLY_VR_4);
    struct LATrainerPolicy p5 = GetLATrainerPolicy(TRAINER_WALLY_VR_5);

    EXPECT(p1.category == LA_TRAINER_MAJOR);
    EXPECT(p2.category == LA_TRAINER_MAJOR);
    EXPECT(p3.category == LA_TRAINER_MAJOR);
    EXPECT(p4.category == LA_TRAINER_MAJOR);
    EXPECT(p5.category == LA_TRAINER_MAJOR);

    EXPECT(LATrainerPolicyHas(p1, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(LATrainerPolicyHas(p2, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(!LATrainerPolicyHas(p1, LA_TRAINER_POLICY_SCALE_EVOLUTION));
}

TEST("LA Trainer: sentinel trainer IDs are EXEMPT")
{
    struct LATrainerPolicy sb = GetLATrainerPolicy(TRAINER_SECRET_BASE);
    EXPECT(sb.category == LA_TRAINER_EXEMPT);
    EXPECT(sb.flags == LA_TRAINER_POLICY_EXEMPT);

    struct LATrainerPolicy link = GetLATrainerPolicy(TRAINER_LINK_OPPONENT);
    EXPECT(link.category == LA_TRAINER_EXEMPT);

    struct LATrainerPolicy unionRoom = GetLATrainerPolicy(TRAINER_UNION_ROOM);
    EXPECT(unionRoom.category == LA_TRAINER_EXEMPT);
}

TEST("LA Trainer: TRAINERS_COUNT boundary is EXEMPT")
{
    EXPECT(GetLATrainerPolicy(TRAINERS_COUNT).category == LA_TRAINER_EXEMPT);
}

TEST("LA Trainer: reserved trainer-ID band (855-863) is EXEMPT")
{
    u16 id;
    for (id = 855; id < 864; id++)
    {
        struct LATrainerPolicy p = GetLATrainerPolicy(id);
        EXPECT(p.category == LA_TRAINER_EXEMPT);
        EXPECT(!(p.flags & LA_TRAINER_POLICY_SCALE_LEVEL));
        EXPECT(!(p.flags & LA_TRAINER_POLICY_MAJOR));
    }
}

TEST("LA Trainer: 0xFFFF is EXEMPT")
{
    EXPECT(GetLATrainerPolicy(0xFFFF).category == LA_TRAINER_EXEMPT);
}

TEST("LA Trainer: invalid IDs never fold to trainer 0 / ORDINARY")
{
    // Sentinel path must catch 0xFF00 before the TRAINERS_COUNT range check,
    // and the range check must catch >= TRAINERS_COUNT without SanitizeTrainerId
    // folding to 0.
    EXPECT(GetLATrainerPolicy(TRAINER_SECRET_BASE).category == LA_TRAINER_EXEMPT);
    struct LATrainerPolicy big = GetLATrainerPolicy(0x8000);
    EXPECT(big.category == LA_TRAINER_EXEMPT);
    EXPECT(big.category != LA_TRAINER_ORDINARY);
}

TEST("LA Trainer: valid unclassified ID safely defaults ORDINARY")
{
    // id 6 is valid, ordinary (zero-class row in the test table).
    struct LATrainerPolicy p = GetLATrainerPolicy(6);
    EXPECT(p.category == LA_TRAINER_ORDINARY || p.category == LA_TRAINER_EXEMPT);
    if (p.category == LA_TRAINER_ORDINARY)
        EXPECT(LATrainerPolicyHasAll(p, LA_POLICY_ORDINARY));
}

TEST("LA Trainer: EXEMPT invariant holds")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(TRAINERS_COUNT);
    EXPECT(p.category == LA_TRAINER_EXEMPT);
    EXPECT(p.flags == LA_TRAINER_POLICY_EXEMPT);
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_LEVEL | LA_TRAINER_POLICY_MAJOR
        | LA_TRAINER_POLICY_SCALE_EVOLUTION | LA_TRAINER_POLICY_SMART_AI
        | LA_TRAINER_POLICY_FULL_PARTY | LA_TRAINER_POLICY_COMPETITIVE_ITEMS | LA_TRAINER_POLICY_HANDCRAFTED));
}

TEST("LA Trainer: MAJOR invariant holds")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(1);  // Rival -> MAJOR
    EXPECT(p.category == LA_TRAINER_MAJOR);
    EXPECT(LATrainerPolicyHas(p, LA_TRAINER_POLICY_HANDCRAFTED));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_EVOLUTION));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_EXEMPT));
}

TEST("LA Trainer: ORDINARY invariant holds")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);
    EXPECT(p.category == LA_TRAINER_ORDINARY);
    EXPECT(LATrainerPolicyHasAll(p, LA_POLICY_ORDINARY));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR | LA_TRAINER_POLICY_HANDCRAFTED | LA_TRAINER_POLICY_EXEMPT));
}

TEST("LA Trainer: partner ID resolves to SPECIAL, never gTrainers indexing")
{
    // TRAINER_PARTNER(1) = MAX_TRAINERS_COUNT+1 (Steven partner slot).
    u16 partnerId = TRAINER_PARTNER(1);
    struct LATrainerPolicy p = GetLATrainerPolicy(partnerId);
    EXPECT(p.category == LA_TRAINER_SPECIAL);
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_SCALE_LEVEL));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_MAJOR));
    EXPECT(!LATrainerPolicyHas(p, LA_TRAINER_POLICY_EXEMPT));
}

TEST("LA Trainer: runtime eligibility ordinary normal is true")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);
    EXPECT(LATrainerRuntimeEligibility(p, 0, 0, FALSE));
}

TEST("LA Trainer: runtime eligibility debug is false")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);
    EXPECT(!LATrainerRuntimeEligibility(p, 0, 0, TRUE));
}

TEST("LA Trainer: runtime eligibility exempt is false")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(TRAINERS_COUNT);
    EXPECT(!LATrainerRuntimeEligibility(p, TRAINERS_COUNT, 0, FALSE));
}

TEST("LA Trainer: runtime eligibility partner (SPECIAL) is false")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(TRAINER_PARTNER(1));
    EXPECT(!LATrainerRuntimeEligibility(p, TRAINER_PARTNER(1), 0, FALSE));
}

TEST("LA Trainer: protected AI helper recognizes all four special flags")
{
    EXPECT(LATrainerAIIsProtected(AI_FLAG_DYNAMIC_FUNC));
    EXPECT(LATrainerAIIsProtected(AI_FLAG_ROAMING));
    EXPECT(LATrainerAIIsProtected(AI_FLAG_SAFARI));
    EXPECT(LATrainerAIIsProtected(AI_FLAG_FIRST_BATTLE));
    EXPECT(LATrainerAIIsProtected(AI_FLAG_DYNAMIC_FUNC | AI_FLAG_ROAMING));
    EXPECT(!LATrainerAIIsProtected(AI_FLAG_SMART_TRAINER));
    EXPECT(!LATrainerAIIsProtected(0));
}

static u64 SmartFlags(u64 authored)
{
    return GetLATrainerAIFlags(authored, GetLATrainerPolicy(0), TRUE, TRUE);
}

TEST("LA Trainer: Smart AI ordinary and Major receive approved mask")
{
    EXPECT_EQ(SmartFlags(0), LA_SMART_AI_MASK);
    EXPECT_EQ(GetLATrainerAIFlags(0, GetLATrainerPolicy(1), TRUE, TRUE), LA_SMART_AI_MASK);
}

TEST("LA Trainer: Smart AI requires policy runtime and controller permission")
{
    struct LATrainerPolicy p = GetLATrainerPolicy(0);
    p.flags &= ~LA_TRAINER_POLICY_SMART_AI;
    EXPECT_EQ(GetLATrainerAIFlags(AI_FLAG_HP_AWARE, p, TRUE, TRUE), AI_FLAG_HP_AWARE);
    EXPECT_EQ(GetLATrainerAIFlags(AI_FLAG_HP_AWARE, GetLATrainerPolicy(0), FALSE, TRUE), AI_FLAG_HP_AWARE);
    EXPECT_EQ(GetLATrainerAIFlags(AI_FLAG_HP_AWARE, GetLATrainerPolicy(0), TRUE, FALSE), AI_FLAG_HP_AWARE);
}

TEST("LA Trainer: Smart AI SPECIAL and EXEMPT unchanged")
{
    EXPECT_EQ(GetLATrainerAIFlags(AI_FLAG_HP_AWARE, GetLATrainerPolicy(TRAINER_PARTNER(1)), FALSE, TRUE), AI_FLAG_HP_AWARE);
    EXPECT_EQ(GetLATrainerAIFlags(AI_FLAG_HP_AWARE, GetLATrainerPolicy(0xFFFF), FALSE, TRUE), AI_FLAG_HP_AWARE);
}

TEST("LA Trainer: Smart AI preserves every authored bit")
{
    for (u32 bit = 0; bit < 64; bit++)
    {
        u64 authored = 1ULL << bit;
        EXPECT_EQ(SmartFlags(authored) & authored, authored);
    }
}

TEST("LA Trainer: Smart AI retains authored Tera without granting Tera prediction or partner attacks")
{
    EXPECT(SmartFlags(AI_FLAG_SMART_TERA) & AI_FLAG_SMART_TERA);
    EXPECT_EQ(SmartFlags(0) & (AI_FLAG_SMART_TERA | AI_FLAG_PREDICTION | AI_FLAG_ATTACKS_PARTNER), 0);
    EXPECT_EQ(SmartFlags(0) & AI_FLAG_OMNISCIENT, AI_FLAG_OMNISCIENT);
}

TEST("LA Trainer: Smart AI protected strategies unchanged")
{
    const u64 protected[] = {AI_FLAG_DYNAMIC_FUNC, AI_FLAG_ROAMING, AI_FLAG_SAFARI, AI_FLAG_FIRST_BATTLE};
    for (u32 i = 0; i < ARRAY_COUNT(protected); i++)
        EXPECT_EQ(SmartFlags(protected[i] | AI_FLAG_HP_AWARE), protected[i] | AI_FLAG_HP_AWARE);
}

TEST("LA Trainer: Smart AI deterministic idempotent and does not mutate trainer data")
{
    struct Trainer before = gTrainers[DIFFICULTY_NORMAL][0];
    u64 flags = SmartFlags(before.aiFlags);
    EXPECT_EQ(SmartFlags(before.aiFlags), flags);
    EXPECT_EQ(SmartFlags(flags), flags);
    EXPECT_EQ(memcmp(&before, &gTrainers[DIFFICULTY_NORMAL][0], sizeof(before)), 0);
}

static void SetupSmartAIContext(u32 battleFlags)
{
    gIsDebugBattle = FALSE;
    gBattleTypeFlags = battleFlags;
    TRAINER_BATTLE_PARAM.opponentA = 0;
    SetCurrentDifficultyLevel(DIFFICULTY_NORMAL);
    ResetDynamicAiFunctions();
}

static bool32 CustomSwitch(struct SwitchAiContext *ctx)
{
    return FALSE;
}

static s32 CustomScore(u32 battlerAtk, u32 battlerDef, u32 move, s32 score)
{
    return score;
}

TEST("LA Trainer: Smart AI actual zero-authored doubles normalization")
{
    SetupSmartAIContext(BATTLE_TYPE_TRAINER | BATTLE_TYPE_DOUBLE);
    ASSUME(gTrainers[DIFFICULTY_NORMAL][0].aiFlags == 0);
    u64 flags = TestGetLATrainerAIFlags(0, B_BATTLER_1);
    EXPECT_EQ(flags & LA_SMART_AI_MASK, LA_SMART_AI_MASK);
    EXPECT(flags & AI_FLAG_DOUBLE_BATTLE);
    EXPECT_EQ(flags & (AI_FLAG_SMART_TERA | AI_FLAG_PREDICTION | AI_FLAG_ATTACKS_PARTNER), 0);
}

TEST("LA Trainer: Smart AI active switching callback bypasses augmentation")
{
    SetupSmartAIContext(BATTLE_TYPE_TRAINER);
    gDynamicAiSwitchFunc = CustomSwitch;
    EXPECT_EQ(TestGetLATrainerAIFlags(0, B_BATTLER_1), 0);
    ResetDynamicAiFunctions();
    EXPECT_EQ(TestGetLATrainerAIFlags(0, B_BATTLER_1) & LA_SMART_AI_MASK, LA_SMART_AI_MASK);
}

TEST("LA Trainer: Smart AI active scoring callback keeps dynamic controller")
{
    SetupSmartAIContext(BATTLE_TYPE_TRAINER);
    sDynamicAiFunc = CustomScore;
    EXPECT_EQ(TestGetLATrainerAIFlags(0, B_BATTLER_1), AI_FLAG_DYNAMIC_FUNC);
    ResetDynamicAiFunctions();
}

TEST("LA Trainer: Smart AI actual special controller exclusions")
{
    const u32 excluded[] = {BATTLE_TYPE_LINK, BATTLE_TYPE_SAFARI, BATTLE_TYPE_ROAMER,
        BATTLE_TYPE_FIRST_BATTLE, BATTLE_TYPE_FRONTIER, BATTLE_TYPE_EREADER_TRAINER,
        BATTLE_TYPE_TRAINER_HILL, BATTLE_TYPE_SECRET_BASE, BATTLE_TYPE_BATTLE_TOWER};
    for (u32 i = 0; i < ARRAY_COUNT(excluded); i++)
    {
        SetupSmartAIContext(BATTLE_TYPE_TRAINER | excluded[i]);
        EXPECT_EQ(TestGetLATrainerAIFlags(0, B_BATTLER_1) & AI_FLAG_OMNISCIENT, 0);
    }
    SetupSmartAIContext(BATTLE_TYPE_TRAINER);
    gIsDebugBattle = TRUE;
    u64 debugFlags = GetTrainerAIFlagsFromId(0);
    if (debugFlags & AI_FLAG_SMART_SWITCHING)
        debugFlags |= AI_FLAG_SMART_MON_CHOICES;
    if (debugFlags & AI_FLAG_PREDICT_INCOMING_MON)
        debugFlags |= AI_FLAG_PREDICT_SWITCH;
    EXPECT(TestGetLATrainerAIFlags(0, B_BATTLER_1) == debugFlags);
    gIsDebugBattle = FALSE;
}

TEST("LA Trainer: Smart AI player partner keeps authored flags")
{
    SetupSmartAIContext(BATTLE_TYPE_TRAINER | BATTLE_TYPE_INGAME_PARTNER);
    u16 id = TRAINER_PARTNER(1);
    u64 authored = GetTrainerAIFlagsFromId(id);
    u64 flags = TestGetLATrainerAIFlags(id, B_BATTLER_2);
    if (IsDoubleBattle() && authored != 0)
        authored |= AI_FLAG_DOUBLE_BATTLE;
    // The upstream smart-switch normalization still applies to authored flags.
    if (authored & AI_FLAG_SMART_SWITCHING)
        authored |= AI_FLAG_SMART_MON_CHOICES;
    if (authored & AI_FLAG_PREDICT_INCOMING_MON)
        authored |= AI_FLAG_PREDICT_SWITCH;
    EXPECT_EQ(flags, authored);
}
