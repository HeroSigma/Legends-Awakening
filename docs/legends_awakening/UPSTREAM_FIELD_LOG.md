# Field Log upstream audit

## Source and attribution

Manual compatibility port based on [PokemonSanFran/pokeemerald,
unbound-quest-menu](https://github.com/PokemonSanFran/pokeemerald/tree/unbound-quest-menu),
commit c34ebdd80f78f751edbac83ee10d7fc4f0273746.
The original [Unbound Quest Menu documentation](https://github.com/PokemonSanFran/pokeemerald/wiki/Unbound-Quest-Menu)
was inspected before implementation. The branch was cloned separately for
reference; it was not merged into Legends Awakening.

The upstream screen recreates Skeli's Unbound menu using ghoulslash's original
quest menu as its base. Upstream also credits HN, Karathan, Mcboy, Greenphx,
BSBob, MM and Tustin2121. The three graphics/quest_menu assets are retained
from that branch. The screen logic is manually adapted, not a byte-for-byte
copy of its 2,810-line implementation.

## Original feature diff

Compared with its vanilla parent 83df84e40, upstream adds:

- src/quests.c, include/quests.h and include/constants/quests.h.
- graphics/quest_menu/menu.png, menu.pal and menu.bin.
- A root menu.bin scratch asset, not imported here.

Upstream modifies:

- asm/macros/event.inc, data/event_scripts.s and data/script_cmd_table.inc:
  custom quest/subquest opcodes and convenience macros.
- include/global.h and src/new_game.c: SaveBlock2 bit arrays and initialization.
- include/constants/flags.h: Start Menu unlock flag.
- src/start_menu.c: launch action and callback.
- src/list_menu.c: global wrapping behavior.
- src/menu_helpers.c and include/menu_helpers.h: background-reset helper.
- src/strings.c and include/strings.h: example quest strings.
- ld_script.ld, sym_bss.txt and sym_ewram.txt: vanilla linker placement.

## Data and behavior inspected

SideQuest definitions contain title, active/completed descriptions, location,
NPC/item/Pokemon graphic and a child array. Each SubQuest has a globally unique
ID, text, location, graphic and completion label. The example data has **30
parents and 30 children total** (10 for one parent and 20 for another).

Upstream packs five bits per parent (unlocked, active, reward, completed,
favorite) and one completion bit per child into SaveBlock2. Its declarations
reserve 24 bytes for the supplied counts. The wiki's approximate byte estimate
differs; the source declarations were used for the audit.

The menu has parent/child navigation, state filters, alphabetical sort and
favorites. Its reward state tells the player to return for a reward; it does
not supply a generic item-awarding transaction or item reward metadata.
Its script integration adds opcodes in scrcmd.c, not command specials.

## Expansion 1.17.0 adaptations

- SaveBlock2 insertion rejected. Pre-port tests measured SaveBlock1=15568,
  SaveBlock2=3884 and SaveBlock3=4 bytes. An isolated 104-byte record is appended
  to SaveBlock3, whose existing capacity is 1624 bytes. No existing feature
  was disabled. SaveBlock1/2, Trainer Rank and sector definitions are untouched.
- Fixed 64-parent/256-child capacity, magic, version and checksum replace
  count-dependent bit packing. Native SaveBlock3 chunks are outside the normal
  sector payload checksum, so the Field Log record validates its own bytes.
- Four mutually exclusive states replace overlapping flags. Unlock/start maps
  to Active; child completion and favorites persist independently.
- Validated u16 IDs and monotonic state changes reject invalid requests and
  prevent reopening claimed rewards through ordinary setters.
- Item reward metadata and QuestClaimReward are added. Inventory failure leaves
  the quest reward-ready; repeat claims do not award again.
- Quest definitions and strings stay in src/data/quests.h, not the shared
  strings files. Only development definitions are supplied.
- Current CreateTaggedMonIcon/AddItemIconSprite and object-graphics APIs handle
  icons. Old seven-argument CreateMonIcon calls and mixed icon cleanup are not
  copied. Sheet/palette tags are released when the displayed graphic changes.
- Current decompression, font constants, windows, callbacks and tasks are used.
  Heap allocations are sized for the largest actual parent/child list.
  The upstream background assets and two-background layout are retained;
  windows are bounded to the visible screen.
- Global list wrapping is not imported. Filtering, A-Z and favorite ordering
  are local to Field Log. There is always a Close/Back row, including empty
  filters. Locked quests hide titles and details.
- Ten normal Start actions are possible with DexNav. The action buffer is
  expanded from nine to ten; long menus use compact spacing. The Start window
  is one tile wider. Restricted menus get no Field Log action.
- Append-only specials use expansion's existing script transport and implicit
  wait-state mechanism. No script opcodes or vanilla linker/symbol files are
  replaced. Normal C-source discovery builds the new modules.
- Existing Trainer Rank code and its test scientist are unchanged. A second,
  explicitly development-only scientist operates the Field Log tests.

## Limits and compatibility

No story quests, promotions, World State or dynamic encounters are implemented.
Children are objective lists, matching upstream; arbitrary recursive nesting
is not implemented. Categories are metadata; filters currently select states.

A missing record is read as Locked and initialized only on an explicit write.
Recognized corrupt/future-version records reject writes until deliberately
reset or migrated. SaveBlock3 configuration changes can move the appended
record; they require a separate migration audit. Downgrading to an older ROM
can discard quest progress. Preserve IDs, record offsets and capacities in v1.

Automated byte round trips test the serialized representation, not a full
emulator close/reopen cycle. See PROJECT.md for manual persistence and UI checks.

## Milestone file manifest

Added:

- `include/constants/quests.h`: state, ID, capacity and script-operation constants.
- `include/quest_save.h`: fixed, versioned save-record layout.
- `include/quests.h`: definitions and shared C/script API declarations.
- `src/data/quests.h`: two development quest definitions and child metadata.
- `src/quests.c`: state, persistence, rewards and script adapters.
- `src/quest_menu.c`: expansion-compatible Field Log screen and resource lifecycle.
- `data/scripts/field_log_dev.inc`: scientist activation, progress, claim and tools flow.
- `graphics/quest_menu/menu.png`, `menu.pal`, `menu.bin`: upstream screen assets.
- `graphics/quest_menu/menu.gbapal`: derived binary palette present in the worktree.
- `test/quests.c`: ten backend, compatibility and menu smoke tests.
- `docs/legends_awakening/UPSTREAM_FIELD_LOG.md`: this upstream and compatibility audit.

Modified:

- `include/global.h`: append the quest record to SaveBlock3.
- `src/start_menu.c`: Field Log entry, callback and ten-entry capacity/layout.
- `src/menu.c`: widen the Start window and bound its height.
- `data/event_scripts.s`: include quest constants and development script.
- `data/specials.inc`: append quest specials without renumbering existing entries.
- `data/maps/LittlerootTown/map.json`: append the second development scientist.
- `test/save.c`: update the expected SaveBlock3 size to 108 bytes.
- `docs/legends_awakening/PROJECT.md`: milestone architecture, APIs and playtest checklist.

An unrelated existing change to `src/data/pokemon/all_learnables.json` remains
in the working tree and is not part of this milestone's implementation.

Final validation: normal ROM build passed; mGBA executed and passed 10 Field Log,
5 Trainer Rank and 3 SaveBlock tests. `git diff --check` passed. No compiler
warnings were reported in these final build/test logs. Manual visual layout,
restricted-menu playtesting and in-game save/restart checks remain outstanding.
