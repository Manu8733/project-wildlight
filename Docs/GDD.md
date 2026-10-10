# Wildlight game design

Design baseline: 10 October 2026. Initial language: English. This document defines intended play and acceptance criteria. Check `Requirements.json` for what has actually been implemented and verified. Numeric tuning is provisional; the C++ catalogs are the authoritative implemented values. [The extracted balance baseline](BALANCE_BASELINE.md) records current costs, timings, statistics and English catalog names.

## Player and promise

Wildlight serves players who enjoy readable RTS battles, expressive settlement building and short economic chains. A match should reward thoughtful placement and a small number of timely orders. Players should be able to understand why a production building stopped or a spell failed without opening a spreadsheet.

Comparative references are used for audience expectations: Age of Empires for quick strategic commands, Northgard for readable economic pressure, Warcraft III for recognizable magic roles, and castle-building RTS games for spatial defense. The user's latest visual direction takes the cozy, rounded, softly lit feel of Tiny Glade as a reference, enriched with Wildlight's own magical forest atmosphere, fireflies and living light connections. Wildlight's own identity comes from interruptible light infrastructure and route-changing magic. No competitor assets, geometry, UI layouts, stories or balance tables are copied.

**Build a kingdom. Command magic. Shape the battlefield.** Start with a sanctuary tree and nearby workers; connect beacons through a valley, develop a compact economy, then decide which connections to defend and which enemy connection to break.

## Three pillars

1. **A living light network.** Completed allied source nodes form a reachability graph. A connected beacon extends operational influence. Downstream buildings show both their connection state and practical benefit. Cutting one exposed link must have a readable economic or tactical consequence.
2. **Magic that changes the situation.** Light Bridge opens a legal crossing, Root Barrier blocks a route, and sabotage pressures infrastructure. Spells require a valid caster, range, energy and cooldown. Each effect expires and has an understandable counterplay.
3. **A kingdom shaped by decisions.** Houses, food and logistics support recruitment. Defending a supply route, investing in a stronger light chain and training an army compete for resources. Cosmetic growth supports those decisions instead of hiding them.

## Match contract

Initial mode is one human against one AI in a skirmish. Matches target 20–45 minutes after tuning; this duration has not been established by playtest. Tutorial challenges teach the same rules without granting hidden advantages.

The primary objective is destruction of the rival command center. A scenario may use a separate, explicit capture objective; the current objective scenario is designed around holding two beacon sites for 90 seconds. An objective countdown pauses or resets according to the scenario rule, displayed before starting the match. Surrender immediately ends the match. Victory freezes new gameplay orders and shows duration, units recruited/lost and buildings lost. Restart creates a new state using the selected scenario and seed.

Opening sequence: secure wood and food, add housing and storage, claim stone and gold, train an initial defense, then extend light toward a crossing or contested ruin. The first enemy pressure should arrive after the player has a clear defensive response. Midgame centers on a vulnerable link, a contested crossing or a siege. Neither an endless technology tree nor an enormous narrative campaign is needed for the first accepted slice.

## Economy and population

| Resource | Main source | Strategic use | Failure feedback |
| --- | --- | --- | --- |
| Wood | Workers carry timber from trees | Early buildings and infrastructure | Construction preview lists missing wood |
| Stone | Workers at stone deposits | Durable walls and defenses | Stone source/depot route is visible |
| Food | Farms and short processing chain | Recruitment and upkeep | Food warning and morale consequence |
| Gold | Workers at gold deposits, optional exchange | Specialist units, research and siege | Queue explains unaffordable cost |
| Magic energy | Connected magical infrastructure | Spells and arcane effects | Disconnected building and network warning |

Workers have an explicit job, cargo and delivery destination. Resource gains occur when cargo is returned to eligible storage, rather than immediately at the resource node. Exhausted nodes, interrupted paths and full capacity must produce a visible idle/problem state. Production priorities and assignment rules should avoid permanently starving essential food jobs.

Houses increase population capacity. Recruitment reserves capacity consistently, preventing rapid queue clicks from bypassing the limit. Unit upkeep creates a reason to balance economy and army. Food shortage lowers morale; low morale slows growth rather than silently destroying an army. The exact implemented shortage penalty and capacity reservation must be verified against the catalogs and tests.

Technology baseline: Forestry, Agriculture, Masonry, Steel and Arcana. Each has a named prerequisite, a visible cost and completion time. Benefits use small, understandable increments. The market is optional convenience and must not become a free resource conversion loop.

## Building roster

The current catalog contains 19 types. Final content completion requires a finished mesh, import evidence, collision, placement feedback, English tooltip and a tested gameplay role for each.

| Building | Role | Readable silhouette |
| --- | --- | --- |
| Castle | Command center and match objective | Wide roots, high split crown, warm core |
| House | Population capacity | Small rounded timber roof |
| Farm | Food production | Terraced beds and cloth awning |
| Mill | Food chain efficiency | Distinct broad wheel or sails |
| Depot | Cargo return and capacity | Open bays, stacked baskets |
| Lumber Camp | Wood job hub | Long canopy, cut-log rack |
| Quarry | Stone job hub | Terraced stone and lifting frame |
| Gold Mine | Gold job hub | Amber seam and supported entrance |
| Barracks | Melee recruitment | Broad shield sign, robust roof |
| Archer Range | Ranged recruitment | Open target lane and high bow rack |
| Forge | Military upgrades | Low chimney, orange furnace |
| Market | Resource exchange | Three colored canopy bays |
| Wall | Spatial defense | Pale masonry with dark root braces |
| Gate | Controlled passage | Tall paired posts and clear opening |
| Watchtower | Ranged defense | Narrow profile, visible firing platform |
| Beacon | Network extension | Three petals around a lit crystal |
| Sanctuary | Root of the light network | Living tree with suspended lens |
| Mage Tower | Caster training and arcane support | Slender asymmetric prism crown |
| Siege Workshop | Siege recruitment | Low wide workshop and articulated crane |

Construction should spend resources once, produce a partially built state and allow cancellation under a clearly defined refund policy. Repair, upgrade and demolition are distinct commands. Walls snap to consistent footprints; gates have owner-aware path rules. Tower garrisons and complex wall-walks are future features unless explicitly implemented and tested.

## Army roster and counterplay

Worker, Militia, Spearman, Swordsman, Archer, Cavalry, Engineer, Arcane Guard, Forest Ally, Ram, Catapult and Wizard are current simulation definitions. Their numerical tuning is provisional.

The intended counter structure is simple: spear troops deter cavalry; armor reduces repeated light attacks; archers need protection; siege threatens structures but needs an escort; wizards trade direct durability for situational utility. Root effects must not create permanent impassable traps. Siege damage, projectile travel and target validation use simulation rules, so rendering frame rate cannot alter combat output.

Required controls include move, attack, stop, defend/hold, patrol, repair assistance and retreat to a supplied rally position. Group spacing is a clear target; full crowd collision avoidance and sophisticated formations remain separate acceptance work. A unit being drawn in a formation does not prove it can navigate a dense gate safely.

## Light network rules

A source begins powered. An allied completed beacon within the permitted link radius is powered when connected through valid source nodes; downstream reachability is recomputed after construction, destruction, sabotage and restoration. Neutral or enemy nodes never provide accidental player supply. Connection and influence are separate concepts: a linked node spreads a local area of effect; a building within that area still has its own prerequisites.

The UI must explain: source node, link radius, connected state, reason for failure and consequences. An active connection is a thin gold-white line with a moving pulse. A broken connection leaves a muted dotted remnant only when inspected. A raid should visibly disconnect a farm boost or arcane building; restoring the connection should restore its normal rule without double-awarding resources.

An acceptance scenario constructs a chain, confirms a downstream benefit, destroys or disrupts the middle beacon, confirms loss of the benefit, then reconnects the chain. Test it with both owners, cycles and a disconnected island.

## Magic catalog

| School | Spell | Tactical purpose | Validation and counterplay |
| --- | --- | --- | --- |
| Keeper of Roots | Root Barrier | Close a pass temporarily | Legal terrain; duration; alternate route or expiry |
| Keeper of Roots | Mend | Restore nearby allies | Range and energy; cannot revive destroyed entities |
| Keeper of Roots | Harvest Bloom | Short farm productivity window | Eligible allied farm; no stacking exploit |
| Architect of Light | Light Bridge | Open a permitted crossing | Designated crossing sites; expiry recalculates paths |
| Architect of Light | Amplify Beacon | Strengthen a connected node briefly | Allied valid node; clear return to baseline |
| Architect of Light | Sanctuary Shield | Protect infrastructure | Target and duration; does not grant invulnerability |
| Weaver of Shadows | Veil | Limit enemy visibility in an area | Owner-aware visibility; expiring area |
| Weaver of Shadows | Disrupt | Interrupt an enemy light connection | Valid enemy node; repair/redundancy or expiry |
| Weaver of Shadows | Illusion | Create a temporary diversion | Decoy expires, grants no resources or population |

The first required playable pair is Light Bridge plus Root Barrier. The other definitions require integrated visual, audio, balance and onboarding acceptance. Exact mana, range, duration and cooldown come from the implementation catalog; do not publish invented tuning values as tested balance.

## AI behavior

Builder prioritizes economy and local fortification, Conqueror creates earlier military pressure, and Arcanist targets and defends light infrastructure. All personalities must pay the same visible prices and obey the same command validation. Difficulty may change planning cadence and tactical mistakes; hidden unlimited resources are excluded from the baseline design.

AI acceptance is behavioral: it recovers from a lost depot, stops queuing unaffordable units, creates a viable economy, scouts before committing, responds to a threatened command center and finds a legal siege target. Merely changing a personality enum is insufficient. Each scenario/seed test records progress, outcome and evidence of light/magic interactions.

## Scenarios and onboarding

The simulation defines three maps; generated grids are scenario prototypes, not three polished Unreal environments. The first accepted slice uses one forest valley with a river, legal crossing sites, readable start areas and a contested beacon chain. Subsequent maps must change routing and objective decisions rather than recolor the same space.

Tutorial sequence: pan and select; build a house; assign a gather job and observe returned cargo; construct a beacon; notice the connected production benefit; defend a threatened link; cast a bridge; seal an alternate pass; defeat the rival. Each step requires an actual state predicate, supports retry and teaches a single new action. An instructional overlay alone is not a completed guided tutorial.

## UX, accessibility and audio

Costs, selection, health, invalid placements and network failures must be explained in English. Friendly/enemy/neutral identity uses silhouette and label cues in addition to color. All controls should be remappable; until a tested remap screen exists, configured input names are implementation scaffolding. The HUD must remain readable at 1280×720 and 1920×1080 with 100–150% scale. Reduce camera shake, flashes and ambient particles independently.

Audio categories: economy, construction, command acknowledgement, network state, combat, magic and ambience. Use short spatial cues for information and restrained layers for atmosphere. Sound specifications or synthesized placeholders do not count as final recorded sound or a composed soundtrack.

## Scope gates

The first slice is accepted only after one integrated, saved-and-loaded match can be completed in Unreal using original art, a working network, two route spells and an AI rival; measured laptop performance and independent playtest evidence are required. Expansion to all catalog content follows that gate. Multiplayer, storefront work and guaranteed sales are outside this implementation phase.
