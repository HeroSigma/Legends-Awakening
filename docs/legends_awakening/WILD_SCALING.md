# Wild Scaling and Evolution Framework

## Scope

LA v0.5.0 keeps encounter ecology separate from progression. The existing
map, time-of-day, world-state, and Dynamic Encounter selection runs first.
Wild Scaling runs after a slot has selected its species and original level
range. It changes only the spawned level and, for explicitly configured
families, the evolutionary stage.

Wild Scaling stores no new progression fields. Trainer Rank and World Phase
remain event variables, and the party is sampled at encounter time. The separate
fake-RTC compatibility fix appends a clock and layout tag to SaveBlock3; see
[SAVE_COMPATIBILITY.md](SAVE_COMPATIBILITY.md).

## World Level

The effective tier is the greater of the Trainer Rank tier and the World Phase
tier. World Phase Beginning maps to Rookie; World Phase Rookie maps to the
Rookie tier, through World Phase Legend mapping to Legend. This means either
system can lead without copying or mutating the other system.

The configured bands are:

| Tier | Minimum | Maximum |
| --- | ---: | ---: |
| Rookie | 1 | 18 |
| Rising | 12 | 30 |
| Ace | 24 | 45 |
| Elite | 38 | 62 |
| Master | 52 | 82 |
| Legend | 70 | 100 |

Usable party members are non-egg, non-fainted Pokemon. Their levels are
ordered and the strongest three are averaged. A party with one or two usable
members averages only those members. With no usable members, the current tier
minimum is used. The target is `(band minimum + 2 * party average) / 3`,
clamped to the current band. This gives progression a floor while letting a
strong party raise encounters without allowing one weak party member to pull
the result down.

## Map and level calculation

Map modifiers are ROM-resident `{mapGroup, mapNum, levelModifier}` records.
Unlisted maps use zero. The current development table gives Route 101 a -2
modifier; it is intentionally not final balance data.

The original slot range and the level returned by the base chooser are retained.
That chosen level includes the existing lure and Pressure/Hustle/Vital Spirit
behavior. It contributes one quarter of the difference between that level and
the world target. The map modifier is then applied, and a random value selects
a five-level spread from target minus 2 through target plus 2. The result is
clamped to the current progression band and to levels 1 through 100. This
preserves the relative identity of a stronger slot instead of flattening a map.

## Evolutionary stage resolution

Evolution is opt-in through a compact ROM table. The current development
entries cover Treecko/Grovyle/Sceptile, Magikarp/Gyarados, and Wailmer/Wailord.
A family entry supplies stage thresholds and weights. The original encounter
table still stores the base family entry; it is never rewritten.

At a threshold, an earlier stage remains a valid weighted result. Current
development three-stage probabilities are 75% base / 25% middle below the
final threshold. At or above it they are 26.25% base / 8.75% middle / 65% final,
because the middle roll occurs only after the final roll fails. Two-stage
families retain 35% base / 65% final above their final threshold. A final stage is never
guaranteed merely because the level is high. Species absent from the explicit
table are returned unchanged. The resolver does not inspect
ordinary evolution conditions blindly, so item, trade, friendship, time,
location, move, gender, and special evolutions do not become wild evolutions
by accident. Branching families also remain unchanged until a future table
explicitly supplies a branch and its habitat, time, map, or profile context.

Each metadata entry accepts map group/number, Dynamic Encounter profile, and
time-of-day selectors. Treecko-family entries explicitly match Route 101, Profile 1 Day or Profile 2
Morning/Day. Magikarp and Wailmer use wildcard selectors. Future habitat-aware
entries can constrain those fields without changing the encounter engine.

## Integration and exclusions

The effective time-of-day table and Dynamic Encounter profile are selected
before scaling. Therefore a species selected for Morning, Day, Evening, or
Night does not move to another time period. Dynamic Encounter profiles still
control the ecological pool.

The standard land, surfing/water, fishing, Feebas, Rock Smash, Sweet Scent,
mass outbreak, and generated overworld encounter paths use the centralized
resolver. DexNav still discovers species through the effective encounter
table, then applies the final level/species resolver when it creates the
Pokemon.

The following remain outside normal scaling: roamers, static or stationary
scripted encounters, gifts, eggs, trainer parties, legendary and mythical
encounters, special bosses, Battle Frontier-specific generated parties, and
manual overworld wild objects. A future system can opt those paths in with
independent rules. This avoids changing story or facility balance.

Runtime work is constant for each encounter: party scan is at most six
members, map lookup is a small linear ROM table, and evolution lookup is a
small bounded family table. There is no allocation, Pokédex scan, or recursive
evolution walk.

## Development inspector

`Script_InspectWildScaling` is development-only and writes transient values:

- `STR_VAR_1`: Trainer Rank number
- `STR_VAR_2`: World Phase number
- `STR_VAR_3`: top-three usable party average
- `STR_VAR_4`: calculated World Level
- `VAR_0x8006`: current map modifier
- `VAR_RESULT`: final slot-aware target after progression-band clamping

It reads existing Trainer Rank, World Phase, and party state. It does not set
progression, quest, world-state, dynamic-profile, or save data.

The development encounter map remains Route 101. Dynamic profile 0 is the
unconfigured/default profile; existing Dynamic Encounter development profiles
remain responsible for changing the selected pool. Use the existing Trainer
Rank and World State testers to move between Rookie, Ace, and Legend cases.

## Manual validation

1. Set Hoenn state to 1 for Route 101 Profile 1, or to 2 for Profile 2, using the existing World State tester. Profile 0 is the normal fallback. For three-stage tests, use Day in either profile, or Morning in Profile 2. Profile 1 Morning contains non-evolving Sableye; both profiles use Magikarp in Evening and Wailmer at Night.
2. Set both Rank and Phase to Rookie. Use a low-level party and enter grass. Confirm low levels and base stages.
3. Set both systems to Ace with a mid-level party. Confirm higher levels and occasional middle stages.
4. Set both systems to Legend with a high-level party. Confirm high levels, final stages, and eventual base-stage results.
5. Repeat in Morning, Day, Evening, and Night. Confirm time-specific species remain in their original tables.
6. Test surfing, fishing, Sweet Scent, Rock Smash, mass outbreaks, DexNav, and generated overworld encounters.
7. Save normally, close the emulator, reopen, and Continue. Confirm the result derives from persisted progression and current party levels.
8. Lower Rank and Phase and confirm the calculated level decreases.

## Automated coverage

`test/wild_scaling.c` covers progression floors, top-three averaging, an empty
party, live egg filtering, rank/phase independence, map modifiers, and level
spread/clamps. Fixed RNG seeds make evolution checks repeatable. Mid-level
and high-level retention are asserted independently, and each configured
Treecko-family context must produce base, middle, and final stages at high
level. Separate mismatch checks cover map, time, and profile.

The encounter integration test replays the real level chooser and scaling
with the same seed used by `TryGenerateWildMon`, then checks the created
Pokemon's level. It exercises Pressure, Vital Spirit, Hustle, and active lures
through the shared generator's hidden-area path (slot zero), avoiding unrelated
slot-selection RNG. It does not constitute a separate fishing or DexNav UI test.

Dynamic Encounter, World State, Trainer Rank, Field Log, SaveBlock, and existing
wild encounter tests are separate regression suites. Manual emulator checks
above are still required for visible encounter behavior and save/reload.

## Validation

The fake-RTC layout issue found during the polish pass is addressed by the
append-only layout and loader conversion documented in
[SAVE_COMPATIBILITY.md](SAVE_COMPATIBILITY.md). The old quest offset remains 4;
the clock is separate at offset 108. Existing Field Log record checksums and
versions are retained. Fake RTC and time-specific encounters remain enabled.

## Next milestone

LA v0.6.0 - Rookie Rank / Foreign Footprints Prototype. Canonical Foreign
Footprints, story encounters, habitat rosters, Reversal, travel, Champion
Island, and legendary content are outside v0.5.0.
