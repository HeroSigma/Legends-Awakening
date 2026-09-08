# World State Framework

World State represents the condition and progression of the game world. It is
not the player's Trainer Rank and it is not a quest log. A future story system
may use quest results to change World State, and a later promotion system may
use both World State and accomplishments, but those relationships are not
automatic in this framework.

## Design

World State uses persistent event variables rather than adding fields to a save
block. The API validates all global phase and region IDs centrally. Regions
store generic unsigned progression values so story-specific names can be
defined later without making the framework know the story in advance.

The current regions are Hoenn, Johto, Kanto, Sevii, and Sinnoh. The enum is
count-based and can be extended. The current generic phases are Beginning,
Rookie, Rising, Ace, Elite, Master, and Legend. World phases are independent
values, not Trainer Rank aliases.

## Reuse Maps Intelligently

The world should change as the player progresses without requiring a duplicate
map for every story state. Future map work should prefer combinations of:

- flags
- persistent variables
- NPC states
- dialogue states
- map objects
- encounter states
- quest results

The World State API is intended to make conditions such as `HoennState >= 3`,
`WorldPhase >= Ace`, or `JohtoState >= 5` readable from shared map and script
logic. This milestone adds no such story conditions.

## Deliberate Boundaries

World State does not modify Trainer Rank storage, Field Log storage, save block
layouts, Start Menu behavior, quest UI, encounters, Pokémon data, badges, gyms,
link structures, region travel, Reversal, or story content. Future systems
should call the public API and keep their own rules separate from the storage
implementation.

## Storage audit and extension rules

The variable IDs and their decimal forms are recorded in PROJECT.md. The audit
searched tracked and new repository text for the old unused names, FRLG numeric
aliases, new symbols, and exact hexadecimal/decimal IDs. Matches were confined
to the variable headers and this milestone's implementation, tests and docs.
No existing gameplay or configuration consumer was found. The HEAD version of
vars.h marks all six slots unused. `VarGet`/`VarSet` use SaveBlock1.vars for
these IDs; normal new-game event initialization clears them. Map temporary
variable clearing does not reach these slots.

Keep existing region IDs stable and append future regions before COUNT. Add
an independently audited persistent slot and a mapping/name table entry for
each new region. Do not derive storage IDs from region arithmetic. Region
states accept the entire u16 range (0..65535); decreases are allowed. Global
phases accept only 0..WORLD_PHASE_COUNT-1. Invalid region names return Unknown.
Existing baseline saves normally contain zero in these unused slots; this
framework does not overwrite values in saves from other hacks that used them.

## Script contract

Use `specialvar VAR_RESULT, Script_GetWorldPhase` for the phase. Set/compare
phase specials take VAR_0x8004. Region get/name specials take the region ID in
VAR_0x8004; region set/compare additionally take the state in VAR_0x8005.
Getters return the value and setters/comparisons return TRUE/FALSE through
`specialvar`. Name specials use `special` and write STR_VAR_1: phase naming
uses the current saved phase; region naming uses the supplied region ID.
All region script arguments are checked before narrowing from u16 to u8.

For future conditional dialogue, set VAR_0x8004 to WORLD_REGION_HOENN and
VAR_0x8005 to a threshold, call Script_IsRegionWorldStateAtLeast with
specialvar, then branch on VAR_RESULT. No raw persistent-variable manipulation
is needed. This is an interface example, not an installed story condition.

## Manual emulator checklist

1. Start a new game and reach Littleroot. Speak to the scientist at (12, 16).
2. Use View all states: confirm Beginning and zero for all five regions.
3. Set global phase to Rising. Increment Hoenn twice, reopening the tester
   as needed. View again: Rising, Hoenn 2, and other regions still zero.
4. Choose Hear Hoenn proof dialogue: expect "The situation is getting worse."
5. Save normally, close the emulator completely, reopen the ROM and Continue.
6. Confirm Rising and Hoenn 2 persist and repeat the proof dialogue.
7. Reset Hoenn and verify "The world is quiet." Increment once and verify
   "Something has changed in Hoenn." Check Cancel/B and global phase decreases.
8. Verify the original Trainer Rank and Field Log testers still work and retain
   their prior values/progress. If loading while already in Littleroot, enter
   and leave a building to refresh map objects.

Manual emulator persistence and visual checks have not been performed by this
implementation pass. The increment control uses u16 arithmetic and wraps to
zero after 65535; this is a development tool, not a progression policy.

## Files in this milestone

Added: include/constants/world_state.h (enums), include/world_state.h (API),
src/world_state.c (central storage and script adapters), test/world_state.c
(focused tests), data/scripts/world_state_dev.inc (tester and proof dialogue),
and this document.

Modified: include/constants/vars.h (six reservations), data/specials.inc
(append-only specials), data/event_scripts.s (constants/script includes),
data/maps/LittlerootTown/map.json (one appended development scientist), and
PROJECT.md (milestone and next milestone).

The pre-existing src/data/pokemon/all_learnables.json change is unrelated and
was not edited as part of this milestone.

## Validation results

Normal `make -j8` ROM build passed. The existing mGBA runner passed all
8 World State, 5 Trainer Rank, 10 Field Log and 3 SaveBlock tests using
test-name prefixes. `git diff --check` passed. World State checks include
new-game event initialization, map temporary clearing, all phases/regions,
u16 boundaries, invalid full-width script arguments, decreases, comparisons,
and byte-for-byte preservation of populated quest storage and Trainer Rank.
Protected save, rank, quest and menu files are unchanged from HEAD.
No warnings were reported in the final build/test logs.
