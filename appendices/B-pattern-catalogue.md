## Appendix B — Pattern Catalogue (quick reference)

**Scaling:** horizontal scaling · stateless services · read replicas · sharding · consistent hashing · caching (aside/through/behind) · CQRS · materialized views · fan-out on write/read/hybrid · precomputation · batching · async processing · CDN · edge compute · connection pooling · database proxy · queue-based load levelling

**Consistency:** single-leader replication · quorum (R+W>N) · read-your-writes · monotonic reads · bounded staleness · MVCC · optimistic concurrency (ETag/version) · pessimistic locking (`FOR UPDATE`) · exclusion constraints · consensus (Raft) · leader lease · fencing tokens · CRDTs · saga · transactional outbox · inbox/dedupe · idempotency keys · two-phase commit · event sourcing

**Reliability:** timeouts · retries with jittered backoff · retry budget · circuit breaker · bulkhead · fallback/graceful degradation · load shedding · rate limiting · backpressure · health checks (shallow/deep) · redundancy (N+1/2N) · multi-AZ · multi-region · cell-based architecture · shuffle sharding · static stability · constant work · DLQ · quarantine queue · chaos experiments · canary · feature flag kill switch · blue/green · progressive rollout

**Latency:** local L1 cache · hedged requests · tied requests · deadline propagation · cancellation · request coalescing (single-flight) · prefetch · compression · protocol upgrade (H2/H3/gRPC) · zone-aware routing · geo-partitioning · read-local/write-global · anycast · connection reuse · TLS session resumption · thread-per-core

**Data movement:** CDC · outbox · change streams · event-carried state transfer · claim check (pointer to blob) · dual-write + backfill + shadow read + cutover · reverse ETL · log compaction · replay

**Boundaries:** bounded context · anti-corruption layer · BFF · API gateway · sidecar/service mesh · strangler fig · branch by abstraction · published language · database-per-service · shared-nothing

**Multi-tenancy:** silo/pool/bridge · tenant_id + RLS · per-tenant keys · per-tenant quotas · cell pinning · shuffle sharding · tenant tiering

---

