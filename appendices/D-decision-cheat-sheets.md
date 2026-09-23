## Appendix D — Decision Cheat Sheets

### Which database?
Start: **Postgres.** Move only for a measured reason.
- Need flexible ad-hoc queries + transactions → relational
- Need >100 k writes/s on time-ordered per-key data → wide-column (Cassandra/Scylla)
- Need single-digit-ms key lookups at any scale, no joins → DynamoDB/Redis
- Need relevance-ranked text search → search engine as a **derived** index
- Need scans over billions of rows for analytics → columnar (ClickHouse/BigQuery)
- Need multi-hop relationship queries → graph
- Need global ACID with low-latency regional reads → distributed SQL (Spanner/Cockroach)
- Need similarity search → pgvector first; dedicated vector DB when recall/scale demands

### Sync or async?
Async unless the caller genuinely needs the result to proceed. Sync when: the user must see the outcome now, or the operation is a read. Async when: it's slow, can be retried, fans out, or crosses a service boundary for a non-critical effect (email, thumbnail, index update, analytics, webhook).

### Strong or eventual consistency?
Per operation, not per system. Strong for: money, inventory, unique constraints, auth/permissions, anything with an invariant a user can exploit. Eventual for: feeds, counts, search indexes, recommendations, analytics, notifications, caches. Then state the maximum staleness you're willing to expose.

### Cache or not?
Cache when read:write ≥ 10:1, the same keys are re-read, staleness is tolerable, and the origin cost is high. Don't cache when data is per-request unique, writes dominate, or a stale value is dangerous. Always answer: **"can the origin survive a 0% hit ratio?"** If not, you don't have a cache, you have a dependency.

### Shard or not?
Not until you must. First: indexes, query fixes, caching, read replicas, vertical scale, archiving cold data, partitioning inside one DB. Then shard — and choose the key by the dominant access pattern so ≥95% of queries hit one shard.

### Monolith or microservices?
Monolith (modular) unless you have: independent scaling profiles, multiple teams blocked on each other's deploys, hard isolation/compliance requirements, or radically different tech needs. "Microservices" is an organizational solution with a technical cost.

### Multi-region?
Only for: latency for a global user base, regulatory residency, or an availability SLO that a single region can't meet. Cheapest first step: read-local/write-global. Full active-active multi-master last, and only with a conflict-resolution story per data type.

---

