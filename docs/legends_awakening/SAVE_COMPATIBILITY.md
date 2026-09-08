# LA v0.5.0 SaveBlock3 compatibility audit

## Finding and decision

Fake RTC remains enabled, as do Morning / Day / Evening / Night encounter
tables. The previous failures were real layout-compatibility failures, not
capacity overflow. Simply accepting a 120-byte size would leave old Field Log
records at the wrong address.

The clean v0.4.0 baseline (`6e017f84fd`) passed Field Log 10/10 and SaveBlock
3/3. Enabling fake RTC inserted 12 bytes at the front of SaveBlock3, moving
the quest record from 4 to 16 and the DexNav chain from 0 to 12. A compiler
probe with only that configuration change reproduced the difference.

The final layout keeps the shipped LA prefix and quest record in place and
appends fake RTC plus a four-byte layout tag. No data moves between save
blocks. No gameplay features or new progression fields are added.

## Exact layouts (GBA ABI, bytes)

| Field | v0.2-v0.4, RTC disabled | Experimental RTC-first v0.5 | Final v0.5 |
| --- | --- | --- | --- |
| DexNav chain (1 byte) | 0 | 12 | 0 |
| Alignment padding (3 bytes) | 1-3 | 13-15 | 1-3 |
| Field Log record (104 bytes) | 4-107 | 16-119 | 4-107 |
| Fake RTC (12 bytes including ABI padding) | absent | 0-11 | 108-119 |
| Layout tag `LAS1` (4 bytes) | absent | absent | 120-123 |
| Total size | 108 | 120 | 124 |
| Remaining capacity | 1516 | 1504 | 1500 |

Capacity is `14 * 116 = 1624` bytes: one 116-byte footer chunk in each of the
14 sectors of the chosen save slot. The final layout occupies chunk 0 and
8 bytes of chunk 1. The existing chunk-copy code already handles partial
chunks and rotating physical sector order. SaveBlock1 remains 15568 bytes;
SaveBlock2 remains 3884 bytes. Compile-time offset/size assertions prevent
future configuration switches from silently changing the supported layout.

## Load conversion and old-save safety

`LaSaveBlock3OnLoad()` runs after all 14 validated sectors have been copied,
including the second footer chunk. New games stamp the layout tag in
`ClearSav3()`. Conversion is RAM-only until the next normal save.

- v0.2-v0.4 saves keep their chain and quest bytes at the original offsets.
  A new fake clock starts at the engine epoch (2000-01-01 00:00:00).
  The existing local-time offset in SaveBlock2 is recalibrated to that origin
  (without changing its layout), preventing an old real-RTC offset from
  producing negative local day counts. Clock-based timers may need to be
  checked manually after this one-time calendar reset.
- v0.1/empty pre-Field-Log saves initialize the clock without inventing quest
  progress; Field Log keeps its existing initialize-on-explicit-write policy.
- Experimental 120-byte RTC-first saves are recognized by a `LOG1` record at
  offset 16, or a valid prefix calendar when no quest record exists. A record
  already recognized at offset 4 takes precedence. A local snapshot permits
  overlapping relocation without corrupting either record. The original
  clock and chain are retained.
- Tagged saves skip conversion. Invalid calendar values reset only the clock.
  Recognized future `LAS*` layouts are left untouched rather than downgraded.
- Recognized damaged or unknown-version quest records are copied byte for byte,
  never repaired or rechecksummed by migration. Existing Field Log read/write
  validation still rejects them.

Known previous LA milestone saves are supported for forward loading. Saves
made with the experimental RTC-first layout are also supported when their
layout is identifiable. Arbitrary corrupted or independently modified save
layouts cannot be identified with certainty: the old formats had no layout
tag. Migration is not a corruption-recovery mechanism. Backward loading into
older ROMs is not a supported workflow; keep a backup before upgrading.

## Checksums and isolation

Native sector checksums cover the primary sector data, not the SaveBlock3
footer bytes. That existing format is unchanged. The Field Log's fixed
104-byte `LOG1` v1 record has its own Fletcher checksum, independent of its
absolute location, so relocation does not require a checksum/version change.
The layout marker is an identifier, not a new checksum. Clock range validation
protects calendar indexing but cannot detect every possible bit flip.

The final Field Log range ends at 107; fake RTC begins at 108. Clock ticks,
clock reset, and quest mutations have explicit cross-preservation tests.
Native footer read/write helpers are exercised with permuted chunk order,
including the clock tail and layout tag in chunk 1.

## Validation

The final working-checkout run passes all requested checks without compiler
overrides:

| Check | Result |
| --- | --- |
| Field Log | 13/13 pass |
| SaveBlock | 8/8 pass |
| Wild Scaling | 9/9 pass |
| Dynamic Encounter | 9/9 pass |
| World State | 8/8 pass |
| Trainer Rank | 5/5 pass |
| Normal ROM build | Pass |
| `git diff --check` / staged diff check | Pass |

The staged source tree is also verified in a separate checkout, excluding the
unrelated local learnables and overworld-option edits. Only fake RTC and
time-of-day encounters are staged from the overworld configuration.

Tests cover exact layout/capacity, legacy
progress and favorites, experimental conversion with overlapping ranges,
invalid checksums and future quest versions, blank saves, separate clock and
quest writes, invalid-clock initialization, and footer-chunk round trips.

Manual emulator verification: load a backup of each historical save format,
check rank/world state and quest/subquest/favorite data, advance time, save,
restart the emulator, and verify the clock and quest state again. Real user
save files were not supplied, so these manual checks remain outstanding.
