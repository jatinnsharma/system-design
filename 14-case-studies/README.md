# Level 14 — Case Studies (Progressive Difficulty)

Full write-up template and guidance live in [`../ROADMAP.md`](../ROADMAP.md#level-14--case-studies-progressive-difficulty).
Do **one per week, minimum**, timed at 45–60 minutes, using the 9-part template in
[`write-ups/TEMPLATE.md`](write-ups/TEMPLATE.md).

## The 38 case studies


**Tier 1 — Fundamentals (after Levels 0–3)**

- [ ] 1. **URL shortener** — ID generation (base62, counter vs hash), collision handling, redirect latency, caching, click analytics, custom aliases, expiry
- [ ] 2. **Pastebin / text storage** — object storage vs DB, size limits, expiry, access control, syntax highlighting at read time
- [ ] 3. **Key-value store (single node)** — hash index, WAL, compaction, crash recovery, TTL, eviction
- [ ] 4. **Rate limiter** — algorithm choice, distributed counters, accuracy vs latency, per-dimension limits, fail-open vs fail-closed
- [ ] 5. **Unique ID generator** — Snowflake vs UUIDv7 vs ticket server; clock skew; monotonicity; multi-region
- [ ] 6. **Web crawler** — frontier queue, politeness/robots.txt, dedupe (Bloom + canonical URLs), DNS caching, trap detection, distributed workers, freshness scheduling
- [ ] 7. **Pagination & infinite scroll API** — cursor design, stability under writes, deletions, jumping to page N

**Tier 2 — Read-heavy at scale (after Levels 4–5)**

- [ ] 8. **News feed / timeline (Twitter/Facebook)** — fan-out on write vs read, hybrid for celebrities, ranking, pagination, caching, feed staleness
- [ ] 9. **Instagram** — media upload pipeline, presigned URLs, transcoding, CDN, feed, follow graph, stories with TTL
- [ ] 10. **YouTube / Netflix** — upload → transcode ladder → packaging (HLS/DASH) → CDN → adaptive bitrate; recommendations; view counts; live vs VOD
- [ ] 11. **Twitter search / typeahead** — inverted index, real-time indexing, trending detection (Count-Min Sketch), autocomplete with tries
- [ ] 12. **Top-K / trending / leaderboard** — Redis sorted sets, sharded counters, approximate heavy hitters, time-decay, ties, pagination in ranks
- [ ] 13. **Web analytics / metrics system** — high-volume ingest, sketches for uniques (HLL), rollups, cardinality control, retention tiers
- [ ] 14. **Notification system** — multi-channel (push/email/SMS/in-app), templating, preferences, dedupe, batching/digest, provider failover, rate limits, delivery tracking
- [ ] 15. **Distributed cache** — consistent hashing, replication, hot keys, eviction, cluster membership, client library design

**Tier 3 — Write-heavy, real-time, stateful (after Levels 6–7)**

- [ ] 16. **WhatsApp / chat** — connection gateway, presence, 1:1 and group messaging, ordering, offline delivery, read receipts, E2E encryption, multi-device sync, media
- [ ] 17. **Google Docs / collaborative editor** — OT vs CRDT, presence/cursors, offline edit merge, version history, snapshots, access control
- [ ] 18. **Uber / ride matching** — geospatial indexing (geohash/S2/H3), driver location ingest at high write rate, matching algorithm, ETA, surge pricing, trip state machine, payments
- [ ] 19. **Food delivery / logistics** — three-sided marketplace, order state machine, courier assignment, real-time tracking, ETA prediction, cancellation/compensation sagas
- [ ] 20. **Payment system** — idempotency, double-entry ledger, exactly-once charge, reconciliation, PSP failover, PCI scope, refunds/chargebacks, currency, audit
- [ ] 21. **Ticket booking (Ticketmaster/BookMyShow)** — inventory reservation, holds with TTL, preventing double-booking (write skew!), queueing during onsale spikes, fairness, payment timeout
- [ ] 22. **Hotel/flight booking** — availability search, inventory across providers, overbooking policy, price caching, distributed transactions vs sagas
- [ ] 23. **Scheduling / appointment system** — availability generation, timezone/DST correctness, recurrence (RRULE), overlap prevention with exclusion constraints, reschedule/cancel flows, reminders, real-time updates pushed via socket patches
- [ ] 24. **Stock exchange / order matching** — order book data structure, matching engine determinism, single-writer per symbol, sequencer + replicated log, market data fan-out, microsecond latency, fairness

**Tier 4 — Infrastructure-scale (after Levels 8–12)**

- [ ] 25. **Google Drive / Dropbox** — chunking, content-addressed dedupe, delta sync, metadata service, conflict resolution, sharing/permissions, offline client, quotas
- [ ] 26. **S3-like object store** — namespace metadata, placement, erasure coding, durability math, multipart upload, consistency, lifecycle, request-rate scaling
- [ ] 27. **Distributed message queue (build Kafka)** — partitioned log, replication + ISR, leader election, consumer groups + offsets, retention, exactly-once semantics
- [ ] 28. **Distributed job scheduler / cron** — leader election, at-least-once vs at-most-once firing, missed-fire policy, timezone handling, backfill, sharded scheduling, dedupe
- [ ] 29. **Workflow engine (Temporal-like)** — durable execution, deterministic replay, event history, timers, signals, versioning of running workflows
- [ ] 30. **Search engine (Google-lite)** — crawl → index → serve; sharded inverted index, scatter-gather with tail-latency mitigation, ranking, caching, freshness
- [ ] 31. **Distributed SQL database** — range sharding, Raft per range, MVCC + HLC, distributed transactions, rebalancing, online schema change
- [ ] 32. **Monitoring & alerting platform (Prometheus/Datadog-scale)** — ingest, TSDB compression (Gorilla/delta-of-delta), cardinality control, query engine, rule evaluation, long-term storage
- [ ] 33. **Feature flag / config service** — low-latency global reads, streaming updates, targeting rules, consistency during rollout, static stability if the service is down
- [ ] 34. **CDN** — anycast, PoP cache hierarchy, purge propagation, TLS at the edge, edge compute, origin protection
- [ ] 35. **API gateway / rate-limited public API platform** — auth, quotas, plans/billing, versioning, developer portal, multi-tenant isolation
- [ ] 36. **Multi-region active-active SaaS** — data residency, routing, conflict handling, failover, per-tenant cells, global control plane with regional data planes
- [ ] 37. **Ad serving / real-time bidding** — <100 ms budget, candidate retrieval, pacing, budget enforcement under concurrency, fraud, event pipeline, billing accuracy
- [ ] 38. **RAG/LLM platform** — ingestion + chunking, vector index, hybrid retrieval, reranking, model serving, caching, eval harness, per-tenant isolation, spend limits

## Working files

- [`notes.md`](notes.md) — running list of patterns you keep reusing across case studies
- [`labs/`](labs/) — how this level works (it has no separate lab; the case study *is* the lab)
- [`write-ups/`](write-ups/) — one file per completed case study, named `NN-slug.md`
