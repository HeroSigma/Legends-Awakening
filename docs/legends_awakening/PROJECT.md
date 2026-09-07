# Pokémon Emerald: Legends Awakening

- Project: Pokémon Emerald: Legends Awakening
- Base: pokeemerald-expansion 1.17.0
- Current milestone: LA v0.1.0 - Trainer Rank Foundation
- Current development branch: legends-awakening-dev
- Current gameplay state: expansion baseline with an independent Trainer Rank API
  and an isolated, temporary Littleroot development tester
- First custom system: Trainer Rank

LA v0.0.1 is a minimal branding and baseline milestone. `LEGENDS AWAK` is
the internal GBA ROM header identifier, not the full visible game title.
Title-screen graphics and gameplay remain unchanged.

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

### Scope and next milestone

No promotion conditions, automatic promotions, rewards, Field Log, quests,
world-state system, story progression, or Trainer Card redesign are implemented.
Save layouts, Trainer Card/link formats, battles, Pokémon, encounters, Gym/badge
logic, Start Menu, title graphics, and expansion compatibility metadata remain
unchanged. Older baseline saves retain their layout; an invalid rank value is
read safely without silently rewriting the save.

Next planned milestone: **LA v0.2.0 - Field Log / Quest Framework**.
The planned Field Log will use the ported Unbound Quest Menu as its foundation;
no menu port or quest implementation is part of LA v0.1.0.
