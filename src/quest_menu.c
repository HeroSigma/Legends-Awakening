// Manual expansion 1.17.0 port of PokemonSanFran's Unbound Quest Menu.
// Upstream c34ebdd80f78f751edbac83ee10d7fc4f0273746; see docs/legends_awakening/UPSTREAM_FIELD_LOG.md.
// Retains the upstream two-background layout, parent/subquest navigation,
// state filters, alphabetical sorting and persistent favorites.
#include "global.h"
#include "quests.h"
#include "bg.h"
#include "decompress.h"
#include "event_data.h"
#include "event_object_movement.h"
#include "field_weather.h"
#include "gpu_regs.h"
#include "graphics.h"
#include "item_icon.h"
#include "list_menu.h"
#include "malloc.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "pokemon_icon.h"
#include "scanline_effect.h"
#include "script.h"
#include "sound.h"
#include "sprite.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/rgb.h"
#include "constants/songs.h"

#define FIELD_LOG_ICON_TAG 0xF100
#define FIELD_LOG_ROWS 4
#define FIELD_LOG_NAME_BYTES 80

enum { WIN_HEADER, WIN_LIST, WIN_DETAILS, WIN_HINTS };
struct QuestMenuResources
{
    MainCallback callback;
    struct ListMenuItem *items;
    u8 (*names)[FIELD_LOG_NAME_BYTES];
    u16 background[1024];
    u16 scroll;
    u16 row;
    u16 count;
    u16 parent;
    u16 focus;
    u8 listTask;
    u8 icon;
    u8 filter; // 0=all; 1..4=Locked/Active/Reward/Complete
    bool8 alphabetical;
    bool8 closing;
};

static EWRAM_DATA struct QuestMenuResources *sMenu = NULL;
static const u32 sQuestMenuTiles[] = INCGFX_U32("graphics/quest_menu/menu.png", ".4bpp.lz");
static const u16 sQuestMenuPalette[] = INCBIN_U16("graphics/quest_menu/menu.gbapal");
static const u16 sQuestMenuTilemap[] = INCBIN_U16("graphics/quest_menu/menu.bin");

// Same background split as the upstream screen, with windows bounded to 160px.
static const struct BgTemplate sQuestMenuBgTemplates[] =
{
    {.bg = 0, .charBaseIndex = 0, .mapBaseIndex = 31, .priority = 1},
    {.bg = 1, .charBaseIndex = 3, .mapBaseIndex = 30, .priority = 2},
};
static const struct WindowTemplate sWindows[] =
{
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 0, .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 1 },
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 2, .width = 30, .height = 8, .paletteNum = 15, .baseBlock = 61 },
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 12, .width = 30, .height = 7, .paletteNum = 15, .baseBlock = 301 },
    { .bg = 0, .tilemapLeft = 0, .tilemapTop = 10, .width = 30, .height = 2, .paletteNum = 15, .baseBlock = 511 },
    DUMMY_WIN_TEMPLATE,
};
static const u8 sDark[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_DARK_GRAY, TEXT_COLOR_TRANSPARENT};
static const u8 sWhite[] = {TEXT_COLOR_TRANSPARENT, TEXT_COLOR_WHITE, TEXT_COLOR_TRANSPARENT};
static const u8 sTitle[] = _("FIELD LOG");
static const u8 sLocked[] = _("??????");
static const u8 sEmpty[] = _("");
static const u8 sFavorite[] = _("F ");
static const u8 sChildren[] = _(" {A_BUTTON}");
static const u8 sClose[] = _("Close");
static const u8 sBack[] = _("Back");
static const u8 sHints[] = _("A:Steps  B:Back  R:Filter  START:Sort  SELECT:Fav");
static const u8 sChildHints[] = _("Child objectives                         B:Back");
static const u8 sAll[] = _("All");
static const u8 sFilterLocked[] = _("Locked");
static const u8 sFilterActive[] = _("Active");
static const u8 sFilterReward[] = _("Reward");
static const u8 sFilterComplete[] = _("Complete");
static const u8 *const sFilters[] = {sAll, sFilterLocked, sFilterActive, sFilterReward, sFilterComplete};
static const u8 sObjectives[] = _("Objectives");
static const u8 sAZ[] = _(" A-Z");
static const u8 sLocation[] = _("Location: {STR_VAR_1}");
static const u8 sFinished[] = _("Done");
static const u8 sPending[] = _("Pending");

static void Task_FieldLog(u8 taskId);
static void BuildList(s32 selected);
static void DestroyIcon(void);

static void Print(u8 window, u8 font, u8 x, u8 y, const u8 *text, const u8 *colors)
{
    AddTextPrinterParameterized3(window, font, x, y, colors, TEXT_SKIP_DRAW, text);
}

static void MainCB(void)
{
    RunTasks();
    AnimateSprites();
    BuildOamBuffer();
    DoScheduledBgTilemapCopiesToVram();
    UpdatePaletteFade();
}

static void VBlankCB(void)
{
    LoadOam();
    ProcessSpriteCopyRequests();
    TransferPlttBuffer();
}

static void DestroyIcon(void)
{
    if (sMenu->icon != MAX_SPRITES)
    {
        struct Sprite *sprite = &gSprites[sMenu->icon];
        u16 tileTag = GetSpriteTileTagByTileStart(sprite->oam.tileNum);
        u16 paletteTag = GetSpritePaletteTagByPaletteNum(sprite->oam.paletteNum);
        DestroySprite(sprite);
        if (tileTag != TAG_NONE)
            FreeSpriteTilesByTag(tileTag);
        if (paletteTag != TAG_NONE)
            FreeSpritePaletteByTag(paletteTag);
        sMenu->icon = MAX_SPRITES;
    }
}

static void CreateIcon(u16 graphic, u8 type)
{
    u8 sprite = MAX_SPRITES;
    // The old screen mixed untagged mon icons and generic DestroySprite.
    // Expansion's tagged icon keeps sheet/palette ownership explicit.
    if (type == QUEST_ICON_POKEMON)
        sprite = CreateTaggedMonIcon(FIELD_LOG_ICON_TAG, FIELD_LOG_ICON_TAG, graphic);
    else if (type == QUEST_ICON_ITEM)
        sprite = AddItemIconSprite(FIELD_LOG_ICON_TAG, FIELD_LOG_ICON_TAG, graphic);
    else if (type == QUEST_ICON_NPC)
        sprite = CreateObjectGraphicsSprite(graphic, SpriteCallbackDummy, 20, 132, 0);
    if (sprite != MAX_SPRITES)
    {
        sMenu->icon = sprite;
        gSprites[sprite].x = 20;
        gSprites[sprite].y = 132;
        gSprites[sprite].oam.priority = 0;
    }
}

static void PrintDetails(s32 id, bool8 onInit, struct ListMenu *list)
{
    const struct SideQuest *quest;
    DestroyIcon();
    FillWindowPixelBuffer(WIN_DETAILS, PIXEL_FILL(0));
    if (!onInit)
        PlaySE(SE_SELECT);
    if (id != LIST_CANCEL)
    {
        if (sMenu->parent != QUEST_NONE)
        {
            quest = QuestGetDefinition(sMenu->parent);
            if (id >= 0 && id < quest->numSubquests)
            {
                const struct SubQuest *sub = &quest->subquests[id];
                StringCopy(gStringVar1, sub->map);
                StringExpandPlaceholders(gStringVar4, sLocation);
                Print(WIN_DETAILS, FONT_SMALL, 3, 0, gStringVar4, sWhite);
                Print(WIN_DETAILS, FONT_SMALL, 40, 15, sub->desc, sWhite);
                Print(WIN_DETAILS, FONT_SMALL, 40, 30,
                    QuestIsSubquestComplete(sMenu->parent, id) ? sFinished : sPending, sWhite);
                CreateIcon(sub->sprite, sub->spriteType);
            }
        }
        else if ((quest = QuestGetDefinition(id)) != NULL && QuestGetState(id) != QUEST_STATE_LOCKED)
        {
            StringCopy(gStringVar1, quest->map);
            StringExpandPlaceholders(gStringVar4, sLocation);
            Print(WIN_DETAILS, FONT_SMALL, 3, 0, gStringVar4, sWhite);
            Print(WIN_DETAILS, FONT_SMALL, 40, 13,
                QuestGetState(id) == QUEST_STATE_COMPLETE ? quest->doneDesc : quest->desc, sWhite);
            Print(WIN_DETAILS, FONT_SMALL, 40, 27, QuestGetObjective(id), sWhite);
            Print(WIN_DETAILS, FONT_SMALL, 40, 41, quest->rewardText, sWhite);
            CreateIcon(quest->sprite, quest->spriteType);
        }
    }
    CopyWindowToVram(WIN_DETAILS, COPYWIN_FULL);
}

static void PrintRowState(u8 window, u32 id, u8 y)
{
    const u8 *text;
    if (id == (u32)LIST_CANCEL)
        return;
    if (sMenu->parent != QUEST_NONE)
        text = QuestIsSubquestComplete(sMenu->parent, id) ? sFinished : sEmpty;
    else
        text = sFilters[QuestGetState(id) + 1];
    Print(window, FONT_SMALL, 192, y + 1, text, sDark);
}

static bool32 SortBefore(u16 a, u16 b)
{
    bool32 favA = QuestIsFavorite(a), favB = QuestIsFavorite(b);
    if (favA != favB)
        return favA;
    if (sMenu->alphabetical)
        return StringCompare(QuestGetDefinition(a)->name, QuestGetDefinition(b)->name) < 0;
    return a < b;
}

static void AddRow(u16 id, const u8 *name, bool32 favorite, bool32 children)
{
    u8 *dst = sMenu->names[sMenu->count];
    u8 *end = StringCopy(dst, favorite ? sFavorite : sEmpty);
    StringCopyN(end, name, 60);
    end[60] = EOS;
    if (children)
        StringAppend(dst, sChildren);
    sMenu->items[sMenu->count].name = dst;
    sMenu->items[sMenu->count++].id = id;
}

static void BuildList(s32 selected)
{
    struct ListMenuTemplate template = {0};
    u16 i, j, count = 0, order[QUEST_COUNT], cursor = 0;
    if (sMenu->listTask != TASK_NONE)
        DestroyListMenuTask(sMenu->listTask, NULL, NULL);
    sMenu->listTask = TASK_NONE;
    sMenu->count = 0;
    FillWindowPixelBuffer(WIN_LIST, PIXEL_FILL(0));
    if (sMenu->parent != QUEST_NONE)
    {
        const struct SideQuest *quest = QuestGetDefinition(sMenu->parent);
        for (i = 0; i < quest->numSubquests; i++)
            AddRow(i, quest->subquests[i].name, FALSE, FALSE);
    }
    else
    {
        for (i = 0; i < QUEST_COUNT; i++)
            if (sMenu->filter == 0 || QuestGetState(i) == sMenu->filter - 1)
                order[count++] = i;
        // Upstream alphabetical/favorite ordering, limited to the filtered set.
        for (i = 1; i < count; i++)
        {
            u16 value = order[i];
            for (j = i; j > 0 && SortBefore(value, order[j - 1]); j--)
                order[j] = order[j - 1];
            order[j] = value;
        }
        for (i = 0; i < count; i++)
        {
            const struct SideQuest *quest = QuestGetDefinition(order[i]);
            bool32 unlocked = QuestGetState(order[i]) != QUEST_STATE_LOCKED;
            AddRow(order[i], unlocked ? quest->name : sLocked, QuestIsFavorite(order[i]),
                unlocked && quest->numSubquests != 0);
        }
    }
    sMenu->items[sMenu->count].name = sMenu->parent == QUEST_NONE ? sClose : sBack;
    sMenu->items[sMenu->count++].id = LIST_CANCEL;
    for (i = 0; i < sMenu->count; i++)
        if (sMenu->items[i].id == selected)
            cursor = i;

    template.items = sMenu->items;
    template.moveCursorFunc = PrintDetails;
    template.itemPrintFunc = PrintRowState;
    template.totalItems = sMenu->count;
    template.maxShowed = min(sMenu->count, FIELD_LOG_ROWS);
    template.windowId = WIN_LIST;
    template.item_X = 12;
    template.cursor_X = 2;
    template.upText_Y = 1;
    template.cursorPal = TEXT_COLOR_DARK_GRAY;
    template.fillValue = TEXT_COLOR_TRANSPARENT;
    template.fontId = FONT_SMALL;
    template.itemVerticalPadding = 2;
    sMenu->row = min(cursor, FIELD_LOG_ROWS - 1);
    sMenu->scroll = cursor - sMenu->row;
    sMenu->listTask = ListMenuInit(&template, sMenu->scroll, sMenu->row);

    FillWindowPixelBuffer(WIN_HEADER, PIXEL_FILL(0));
    Print(WIN_HEADER, FONT_SMALL, 3, 1, sTitle, sDark);
    StringCopy(gStringVar4, sMenu->parent != QUEST_NONE ? sObjectives : sFilters[sMenu->filter]);
    if (sMenu->parent == QUEST_NONE && sMenu->alphabetical)
        StringAppend(gStringVar4, sAZ);
    Print(WIN_HEADER, FONT_SMALL, 94, 1, gStringVar4, sDark);
    ConvertIntToDecimalStringN(gStringVar4, sMenu->count - 1, STR_CONV_MODE_LEFT_ALIGN, 3);
    Print(WIN_HEADER, FONT_SMALL, 218, 1, gStringVar4, sDark);
    FillWindowPixelBuffer(WIN_HINTS, PIXEL_FILL(0));
    Print(WIN_HINTS, FONT_SMALL, 2, 0, sMenu->parent == QUEST_NONE ? sHints : sChildHints, sDark);
    CopyWindowToVram(WIN_HEADER, COPYWIN_FULL);
    CopyWindowToVram(WIN_HINTS, COPYWIN_FULL);
    CopyWindowToVram(WIN_LIST, COPYWIN_FULL);
}

static void CloseMenu(void)
{
    MainCallback callback = sMenu->callback;
    DestroyIcon();
    if (sMenu->listTask != TASK_NONE)
        DestroyListMenuTask(sMenu->listTask, NULL, NULL);
    FreeAllWindowBuffers();
    UnsetBgTilemapBuffer(1);
    Free(sMenu->items);
    Free(sMenu->names);
    Free(sMenu);
    sMenu = NULL;
    SetVBlankCallback(NULL);
    SetMainCallback2(callback);
}

static void Task_FieldLog(u8 taskId)
{
    s32 input, selected;
    if (gPaletteFade.active)
        return;
    if (sMenu->closing)
    {
        CloseMenu();
        DestroyTask(taskId);
        return;
    }
    input = ListMenu_ProcessInput(sMenu->listTask);
    ListMenuGetScrollAndRow(sMenu->listTask, &sMenu->scroll, &sMenu->row);
    selected = sMenu->items[sMenu->scroll + sMenu->row].id;
    if (input == LIST_CANCEL)
    {
        if (sMenu->parent != QUEST_NONE)
        {
            u16 parent = sMenu->parent;
            sMenu->parent = QUEST_NONE;
            BuildList(parent);
        }
        else
        {
            sMenu->closing = TRUE;
            BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
        }
    }
    else if (input >= 0 && sMenu->parent == QUEST_NONE)
    {
        const struct SideQuest *quest = QuestGetDefinition(input);
        if (quest != NULL && quest->numSubquests != 0 && QuestGetState(input) != QUEST_STATE_LOCKED)
        {
            sMenu->parent = input;
            BuildList(0);
        }
    }
    else if (sMenu->parent == QUEST_NONE)
    {
        if (JOY_NEW(R_BUTTON))
        {
            sMenu->filter = (sMenu->filter + 1) % ARRAY_COUNT(sFilters);
            BuildList(QUEST_NONE);
        }
        else if (JOY_NEW(START_BUTTON))
        {
            sMenu->alphabetical ^= TRUE;
            BuildList(selected);
        }
        else if (JOY_NEW(SELECT_BUTTON) && selected >= 0)
        {
            QuestSetFavorite(selected, !QuestIsFavorite(selected));
            BuildList(selected);
        }
    }
}

static void SetupCB(void)
{
    u32 i;
    SetVBlankCallback(NULL);
    SetHBlankCallback(NULL);
    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    ScanlineEffect_Stop();
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();
    ResetTasks();
    ClearScheduledBgCopiesToVram();
    ResetBgsAndClearDma3BusyFlags(0);
    InitBgsFromTemplates(0, sQuestMenuBgTemplates, ARRAY_COUNT(sQuestMenuBgTemplates));
    for (i = 0; i < 4; i++)
    {
        ChangeBgX(i, 0, BG_COORD_SET);
        ChangeBgY(i, 0, BG_COORD_SET);
    }
    SetBgTilemapBuffer(1, sMenu->background);
    memcpy(sMenu->background, sQuestMenuTilemap, sizeof(sMenu->background));
    DecompressDataWithHeaderVram(sQuestMenuTiles, (void *)BG_CHAR_ADDR(3));
    LoadPalette(sQuestMenuPalette, 0, sizeof(sQuestMenuPalette));
    LoadPalette(gStandardMenuPalette, BG_PLTT_ID(15), PLTT_SIZE_4BPP);
    if (!InitWindows(sWindows))
    {
        CloseMenu();
        return;
    }
    DeactivateAllTextPrinters();
    for (i = 0; i < ARRAY_COUNT(sWindows) - 1; i++)
        PutWindowTilemap(i);
    BuildList(sMenu->focus);
    CopyBgTilemapBufferToVram(1);
    ShowBg(0);
    ShowBg(1);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_OBJ_ON | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON | DISPCNT_BG1_ON);
    BlendPalettes(PALETTES_ALL, 16, RGB_BLACK);
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    CreateTask(Task_FieldLog, 0);
    SetVBlankCallback(VBlankCB);
    SetMainCallback2(MainCB);
}

void QuestMenu_Init(u16 focusQuest, MainCallback callback)
{
    u32 i, rows = QUEST_COUNT + 1;
    sMenu = AllocZeroed(sizeof(*sMenu));
    if (sMenu == NULL)
    {
        SetMainCallback2(callback);
        return;
    }
    sMenu->callback = callback;
    sMenu->parent = QUEST_NONE;
    sMenu->focus = focusQuest;
    sMenu->listTask = TASK_NONE;
    sMenu->icon = MAX_SPRITES;
    for (i = 0; i < QUEST_COUNT; i++)
        rows = max(rows, QuestGetDefinition(i)->numSubquests + 1);
    sMenu->items = AllocZeroed(rows * sizeof(*sMenu->items));
    sMenu->names = AllocZeroed(rows * sizeof(*sMenu->names));
    if (sMenu->items == NULL || sMenu->names == NULL)
    {
        Free(sMenu->items);
        Free(sMenu->names);
        Free(sMenu);
        sMenu = NULL;
        SetMainCallback2(callback);
        return;
    }
    SetMainCallback2(SetupCB);
}

static void Task_OpenFieldLogFromScript(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        u16 focus = gTasks[taskId].data[0];
        CleanupOverworldWindowsAndTilemaps();
        DestroyTask(taskId);
        QuestMenu_Init(focus, CB2_ReturnToFieldContinueScriptPlayMapMusic);
    }
}

void Script_OpenFieldLog(void)
{
    u8 task = CreateTask(Task_OpenFieldLogFromScript, 0);
    gTasks[task].data[0] = gSpecialVar_0x8004;
    FadeScreen(FADE_TO_BLACK, 0);
}
