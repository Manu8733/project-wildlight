# Future multiplayer architecture — plan only

No multiplayer implementation is included. Single-player gameplay, performance and save stability must first be accepted. This plan preserves useful boundaries without adding a lobby, transport, server or networking dependency now.

The preferred future starting point is an authoritative host/server receiving validated command intents. Commands carry player identity, sequence, simulation tick, stable entity IDs, target and schema version. Ownership and affordability are checked by the authority. The authority advances fixed ticks and publishes snapshots/deltas; clients render interpolated state and never author economic outcomes.

Portable simulation and stable IDs are prerequisites, not proof of deterministic cross-platform lockstep. If lockstep is considered later, measure float behavior, command ordering, RNG, path tie-breaking and state checksums across compiler/OS combinations first. Compare its cost with authoritative snapshots before choosing. Replays require an initial state plus an ordered validated command log and versioned content.

Expected additions: transport abstraction, authenticated session identity, command acknowledgement, bounded queues, tick buffering, reconnect snapshot, version/content handshake, latency diagnostics and authoritative pause/surrender rules. A disconnect must have an explicit match rule. Save games and multiplayer session snapshots need separate access/trust handling.

Test plans include latency/loss/reordering, duplicate commands, stale IDs, unauthorized owner, reconnect, desynchronization diagnostics and soak. A later decision should use actual player demand, budget and CPU/network measurements. Steam networking, matchmaking and service setup remain deferred with List 3.
