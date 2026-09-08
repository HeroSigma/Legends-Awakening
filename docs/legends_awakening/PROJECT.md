# Pokémon Emerald: Legends Awakening

- Project: Pokémon Emerald: Legends Awakening
- Base: pokeemerald-expansion 1.17.0
- Current milestone: LA v0.5.0 - Wild Scaling & Evolution Framework
- Current development branch: feature/wild-scaling
- Current gameplay state: expansion baseline with independent Trainer Rank,
   Field Log, and World State systems, plus isolated Littleroot development testers
- First custom system: Trainer Rank

LA v0.0.1 is a minimal branding and baseline milestone. `LEGENDS AWAK` is
the internal GBA ROM header identifier, not the full visible game title.
That milestone left title-screen graphics and gameplay unchanged.

The base compatibility metadata, including the GF game name and RHHEXP
header layout, is preserved.

## Trainer Rank Foundation

The six ranks are Rookie (0), Rising (1), Ace (2), Elite (3), Master (4),
and Legend (5). Rank is independent of stars, badges, trainer classes,
Frontier records, and story progression.

### Persistence and variable audit

`VAR_TRAINER_RANK` uses existing persistent event variable **0x40F7**
(decimal 16631), stored by the normal `SaveBlock1.vars` system. No save
structures, sector layouts, variable-array bounds, or initialization routines
were changed. Clearing event data initializes the slot to zero: Rookie.

Before allocation, `include/constants/vars.h` explicitly identified this as
`VAR_UNUSED_0x40F7`. `include/constants/vars_frlg.h` defined only the unused
numeric alias `VAR_0x40F7`. A repository-wide text search, including generated
data and configuration headers, found no consumers of either name or the hex
ID. The only additional decimal match was an unrelated generated `#line 16631`
in trainer data. The slot is inside 0x4000-0x40FF and outside temporary,
object-graphics, and special-variable ranges. It is not a story or system var.
The FRLG alias remains unchanged; do not allocate it independently in future.
Re-audit this reservation when upgrading expansion or importing scripts.

### Module and API

- `include/constants/trainer_rank.h`: rank enum; Rookie is explicitly zero.
- `include/trainer_rank.h`: public API and script-adapter declarations.
- `src/trainer_rank.c`: centralized storage access and ROM-resident names.
- `test/trainer_rank.c`: tests using the existing test runner.

Public API:

- `u8 GetTrainerRank(void)`: invalid u16 saved values read as Rookie without
  modifying the stored value.
- `bool32 SetTrainerRank(u8 rank)`: validates and stores a rank; invalid input
  returns FALSE without writing. Direct demotion is allowed for development.
- `bool32 IsTrainerRankAtLeast(u8 rank)`: compares ranks; invalid thresholds
  return FALSE.
- `const u8 *GetTrainerRankName(u8 rank)`: returns a centralized display name,
  falling back to Rookie for invalid input.

The normal Makefile source wildcard discovers the C module automatically.
No custom build mechanism is needed.

### Script interface

Specials are appended to `data/specials.inc` to preserve existing special IDs:

- `specialvar VAR_RESULT, Script_GetTrainerRank` returns the current rank and
  puts its display name in `STR_VAR_1`. Scripts can compare the result against
  rank constants for future gates without accessing the persistent variable.
- `Script_SetTrainerRank` takes `VAR_0x8004` and returns TRUE/FALSE through
  `specialvar`. This is the development setter, not a promotion policy.
- `Script_BufferTrainerRankName` buffers the name of the rank in `VAR_0x8004`
  into `STR_VAR_1`, without changing the current rank.

Script arguments are validated as u16 before narrowing to the public u8 API.
Special variables are transient arguments/results only, never rank storage.

### Temporary development test

Development-only: remove before story implementation. A stationary scientist
at Littleroot Town tile **(10, 14)**, east of Birch's lab, offers View current
rank, Set Rookie, Set Rising, Set Ace, Set Elite, Set Master, Set Legend, and
Cancel. B also cancels. The menu and dialogue use the centralized name API.
The NPC is appended to the object list, preserving existing local IDs, and
does not change any existing story trigger, movement script, or map tile.

The tester is in `data/maps/LittlerootTown/map.json` and the clearly marked
development section of `data/maps/LittlerootTown/scripts.inc`. It uses the
existing dynamic multichoice commands and rank specials only. It never writes
the raw persistent variable. Selecting a rank does not automatically save.

Manual persistence check:

1. Back up existing saves and start a new game; finish the normal home intro.
2. Talk to the scientist east of Birch's lab and select View current rank.
   Confirm Rookie before using any Set option.
3. Talk again, select Set Rising, and confirm the dialogue says Rising.
4. Save through the normal in-game Save menu and wait for completion.
5. Close the emulator completely; reopen the same ROM and save file.
6. Choose Continue (do not restore an emulator save state).
7. Talk to the scientist and select View current rank; confirm Rising.
8. Optionally check every other Set option, demotion to Rookie, and Cancel/B.

### LA v0.1.0 scope (historical)

At LA v0.1.0, no promotion conditions, automatic promotions, rewards, Field Log, quests,
world-state system, story progression, or Trainer Card redesign were implemented.
At that milestone, save layouts, Trainer Card/link formats, battles, Pokémon, encounters, Gym/badge
logic, Start Menu, title graphics, and expansion compatibility metadata remain
unchanged. Older baseline saves retain their layout; an invalid rank value is
read safely without silently rewriting the save.

## LA v0.2.0 - Field Log / Quest Framework

This milestone ports and adapts the Unbound Quest Menu from PokemonSanFran's
`unbound-quest-menu` branch at commit `c34ebdd80f78f751edbac83ee10d7fc4f0273746`.
The upstream implementation was manually compared with expansion 1.17.0;
vanilla files were not merged directly. The upstream `SaveBlock2` quest and
subquest bit arrays were replaced with an isolated, versioned 104-byte record
appended to the existing `SaveBlock3`. A magic, version, and Fletcher checksum
allow old saves to read as locked quests. Reads never initialize storage. An
explicit write initializes a missing magic; recognized records with an unknown
version or invalid checksum are preserved and reject writes. The record is included in the normal SaveBlock3 sector copy,
so Trainer Rank's `VAR_TRAINER_RANK` remains independent and unchanged.

Quest states are Locked (0), Active (1), Reward Available (2), and Complete
(3). Quest definitions are ROM-resident and support title, descriptions,
objective text, location, NPC/item/Pokemon graphic, reward item, category,
favorite state, and child subquests. Child objectives use globally unique bits
in the isolated record. The public API in `include/quests.h` is the only
storage interface map scripts need: it covers state, activation, completion,
subquest progress, rewards, favorites, definitions, and save validation.

The script adapter `Script_QuestCommand` supports starting, querying, setting,
and completing quests and subquests, marking rewards available, and claiming
rewards. `Script_OpenFieldLog` opens the menu from scripts. The normal overworld
Start Menu includes **FIELD LOG** only; link, Union Room, Safari Zone, Battle
Pike, Battle Pyramid, and multi-partner restricted menus retain their existing
entries. The menu preserves the upstream parent/subquest view, state filters,
favorite ordering, alphabetical sorting, location/objective/reward details,
and icon support, using expansion 1.17.0's tagged Pokemon/item sprite APIs and
current decompression/text/window APIs.

The **southern scientist at (10, 16), east of Birch's lab**, is the Field Log
tester. The original Trainer Rank scientist at (10, 14) is unchanged. These NPCs
and the two definitions in `src/data/quests.h` are development-only, not canon.
The tester advances one step per interaction, with reward-ready and claiming
on separate visits. After completion it offers development tools.

Manual emulator checklist:

1. Start a new game or load the existing save. If loading while already in
   Littleroot, enter and exit a building to refresh map objects. Open FIELD LOG
   before talking to the southern scientist: both quests should be Locked.
2. Talk to the southern scientist once. This activates **Framework Test** only.
   Open FIELD LOG, highlight it and press A to view its three pending children.
3. Talk to the scientist three more times. Each interaction completes one child.
   Inspect the child states and NPC/Pokemon/item icons between interactions.
4. The third advance marks the reward available without claiming it. Open
   FIELD LOG and verify Reward Available; save and restart here if desired.
5. Talk again to claim one Potion. Verify Complete and a single added Potion.
   A full bag must leave the reward available and permit retry after making
   space. Later interactions offer tools instead of granting another reward.
6. Use Select to favorite a parent, R to cycle state filters, Start for A-Z,
   and B to return from children/close. Empty filters retain Close.
7. Save through the normal Save menu, close the emulator completely, reopen
   the same ROM/save, and choose Continue (not an emulator save state). Verify
   completion, all child states and favorite status persist. Also repeat while
   Active and Reward Available; activation itself does not advance a child.
8. After completion, talk again for **Open FIELD LOG**, **Activate Icon Test**,
   **Reset test quests**, and Cancel. Opening here also tests script focus and
   returning control to the field. Icon Test has no children and remains Active
   until completed through the shared API. Reset requires confirmation and
   clears only development quest data, not Trainer Rank or previously given items.
9. Verify the northern scientist still reads/sets Trainer Rank. Test B/Cancel
   and reopening the Field Log repeatedly. Reset is development-only and permits
   repeating test rewards; it must be removed before story implementation.

The normal Start menu now has capacity for ten entries, including DexNav and
FIELD LOG. Menus longer than eight entries use smaller text and 14px spacing
inside an 18-tile-high window. The shared Start window is one tile wider to fit
FIELD LOG; restricted menu entries are unchanged. Check the fully unlocked
normal menu, debug configuration, Safari, link and Frontier menus in an emulator.

### Storage and API details

The pre-port save-size tests measured SaveBlock1=15568, SaveBlock2=3884 and
SaveBlock3=4 bytes. SaveBlock3 is now 108/1624 bytes: its old four-byte prefix
is intact and the 104-byte quest payload starts at offset 4. No sector layout,
SaveBlock1/2 field, or existing expansion feature was removed. Capacities are
fixed at 64 parent quests and 256 global child bits independently of the current
two definitions. Keep IDs and v1 offsets stable. Enabling configuration options
that add earlier SaveBlock3 fields requires a separate save migration audit.
Older binaries do not preserve the new record reliably: back up before downgrade.

`QuestStart`, `QuestSetState`, `QuestComplete`, `QuestCompleteSubquest`,
`QuestIsSubquestComplete`, `QuestAreAllSubquestsComplete`,
`QuestMarkRewardAvailable`, `QuestClaimReward`, `QuestGetState`,
`QuestGetDefinition`, `QuestGetObjective`, `QuestGetStateName`,
`QuestIsFavorite`, `QuestSetFavorite` and `QuestSaveIsValid` form the C API.
Normal state transitions are monotonic. Explicit `QuestComplete` marks complete
without giving items; `QuestClaimReward` grants the defined item only from the
reward state. No real promotion or story rules are attached.

For `specialvar VAR_RESULT, Script_QuestCommand`, pass the quest ID in
VAR_0x8004, a QUEST_CMD_* operation in VAR_0x8005, and state/child index in
VAR_0x8006. IDs are validated before narrowing. The result is the state for GET
or TRUE/FALSE for mutations. `special Script_OpenFieldLog` takes an optional
focus quest in VAR_0x8004 (QUEST_NONE for default), waits implicitly and resumes
the script on close. Append-only specials avoid changing expansion opcodes.

Upstream-style subquests are a parent plus a list of child objectives, not an
arbitrarily recursive quest tree. Stable global child IDs allow multiple parent
quests; deeper future groupings require an explicit extension. Category metadata
supports Main, Regional, Character, Exploration, Faction, Gym and Development;
the current filters are by state, not category. No real quests are included.

Focused tests in `test/quests.c` cover locked and invalid IDs, activation,
save-record round trips, favorite persistence, child completion, reward-ready,
reward claim, complete state, and invalid child/state requests. Additional tests cover full-bag reward retry and menu opening, filters,
parent/child icon navigation, repeated closing, and resource cleanup.

Validation completed with the existing mGBA runner: all 10 Field Log tests,
all 5 Trainer Rank tests, and all 3 SaveBlock size checks passed. The final
normal ROM build (`make -j8`) and `git diff --check` passed. Automated record
round trips and menu smoke tests do not replace the manual visual and in-game
save/restart checklist above; those checks remain for emulator playtesting.

## LA v0.3.0 - World State Framework

World State is a centralized persistent-variable API for the condition and
progression of the game world. It is intentionally separate from Trainer Rank,
which represents player recognition and authority, and Field Log, which
represents quests and objectives. No system automatically changes another.

The region enum currently contains Hoenn, Johto, Kanto, Sevii, and Sinnoh, but
the API uses a count-based enum so future regions can be appended without
changing callers. Region values are generic unsigned progression values; no
unfinished story chapter names are encoded. World phases are similarly generic:
Beginning, Rookie, Rising, Ace, Elite, Master, and Legend. These names are
World Phase names only and are not aliases for Trainer Rank.

### Persistent variable audit

World State uses the existing persistent `SaveBlock1.vars` system and does not
modify any SaveBlock layout. The six reserved IDs are:

| Symbol | Hex | Decimal | Meaning |
| --- | --- | ---: | --- |
| `VAR_WORLD_PHASE` | `0x40F8` | 16632 | Global World Phase |
| `VAR_WORLD_STATE_HOENN` | `0x40F9` | 16633 | Hoenn state |
| `VAR_WORLD_STATE_JOHTO` | `0x40FA` | 16634 | Johto state |
| `VAR_WORLD_STATE_KANTO` | `0x40FB` | 16635 | Kanto state |
| `VAR_WORLD_STATE_SEVII` | `0x40FC` | 16636 | Sevii state |
| `VAR_WORLD_STATE_SINNOH` | `0x40FD` | 16637 | Sinnoh state |

Before reservation, `vars.h` marked all six slots unused. `vars_frlg.h`
contains only the old numeric `VAR_0x40F8` through `VAR_0x40FD` aliases; a
repository-wide symbolic and numeric search found no gameplay consumers,
configuration aliases, or script references. The slots are persistent IDs in
the `0x4000-0x40FF` range, outside temporary variables, object-graphics
variables, and special variables. The existing Trainer Rank reservation at
`0x40F7` is unchanged. The old FRLG aliases remain untouched for compatibility
documentation and must not be allocated independently later.

### Public API and scripts

The centralized API is in `include/world_state.h` and `src/world_state.c`:

- `GetWorldPhase`, `SetWorldPhase`, `IsWorldPhaseAtLeast`
- `GetRegionWorldState`, `SetRegionWorldState`, `IsRegionWorldStateAtLeast`
- `GetWorldPhaseName`, `GetWorldRegionName`

Invalid phase reads return Beginning without rewriting storage. Invalid phase
setters and comparisons return `FALSE`. Invalid region reads return zero,
invalid region setters and comparisons return `FALSE`, and no invalid request
can modify another region. Script adapters use `VAR_0x8004` for a phase or
region argument and `VAR_0x8005` for a region state/comparison threshold:

- `Script_GetWorldPhase`, `Script_SetWorldPhase`
- `Script_GetRegionWorldState`, `Script_SetRegionWorldState`
- `Script_IsWorldPhaseAtLeast`, `Script_IsRegionWorldStateAtLeast`
- `Script_BufferWorldPhaseName`, `Script_BufferWorldRegionName`

All script arguments are validated as full `u16` values before reaching the
typed C API. Map scripts should use these specials rather than raw variable
access.

### Development tester

The separate World State scientist at Littleroot `(12, 16)` is development-only
and does not alter the existing Trainer Rank or Field Log scientists. It can
view the current global phase and all five region values, set all seven generic
global phases, increment or reset Hoenn state, and display proof dialogue:

- Hoenn state 0: “The world is quiet.”
- Hoenn state 1: “Something has changed in Hoenn.”
- Hoenn state 2 or higher: “The situation is getting worse.”

These strings and interactions are a framework test, not story canon. Remove
the tester before story implementation.

World State is not connected to quests, Trainer Rank promotion, encounters,
region travel, Reversal, NPC cast changes, badges, gyms, or story progression.

## LA v0.5.0 - Wild Scaling & Evolution Framework

Wild encounters now use a centralized runtime scaling framework after the
existing time-of-day and Dynamic Encounter profile selection. It derives a
World Level from independent Trainer Rank and World Phase values plus the
strongest three usable party levels, applies ROM-resident map modifiers and
bounded variation, and resolves only explicitly configured evolutionary
families. Scaling itself adds no save fields and leaves static, scripted, trainer, gift,
egg, legendary, mythical, boss, roamer, and facility-specific encounters
outside normal scaling.

The framework, focused tests, development inspector contract, exclusions,
manual checklist, and future habitat extension point are documented in
[WILD_SCALING.md](WILD_SCALING.md). The fake-RTC compatibility audit, append-only
SaveBlock3 layout, and legacy conversion are documented in
[SAVE_COMPATIBILITY.md](SAVE_COMPATIBILITY.md).

Next planned milestone: **LA v0.6.0 - Rookie Rank / Foreign Footprints
Prototype**.


World State implementation details, full script calling conventions, extension
rules, file manifest and the manual save/restart checklist are in
[WORLD_STATE.md](WORLD_STATE.md). The tester is development-only; proof dialogue
is selected through its menu and is not a story event.
