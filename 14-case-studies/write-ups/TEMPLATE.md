# Case-Study Write-Up Template

```
1. Requirements
   - Functional (bulleted, prioritized: must / should / out-of-scope)
   - Non-functional: scale (DAU, QPS peak, payload), latency SLO (p50/p99),
     availability SLO, consistency needs per operation, retention, geography, budget
2. Estimates
   - QPS (avg/peak), storage/day and /year, bandwidth, cache size, node count
3. API design
   - Endpoints/RPCs, request/response shapes, idempotency, pagination, errors
4. Data model
   - Entities, keys, indexes, shard key, access patterns → storage choice per store
5. High-level architecture
   - Diagram; sync path vs async path; every component's purpose
6. Deep dives (pick 2–3 the interviewer/reader cares about)
   - The hard part: hot keys, ordering, exactly-once, geo-partitioning, fan-out, matching...
7. Failure modes & operations
   - What breaks, blast radius, SLOs, alerts, DR, rollout plan
8. Bottlenecks & evolution
   - What breaks at 10× and 100×; what you'd change first; what you deliberately deferred
9. Trade-offs & alternatives rejected (with reasons)
```


---

**To use:** copy this file to `../<NN>-<slug>.md` (e.g. `01-url-shortener.md`), fill it in
during your timed session, then add a "**Compared to real system:**" section noting what a
real engineering blog post did differently.
