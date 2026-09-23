## Appendix C — Anti-Pattern Catalogue

| Anti-pattern | Why it fails | Do instead |
|---|---|---|
| Distributed monolith | Microservice cost, monolith coupling | Modular monolith, or fix boundaries first |
| Shared database across services | Schema coupling, no autonomy | DB per service + events/CDC |
| Dual write (DB then publish) | Lost events on crash | Transactional outbox |
| Unbounded queues | Overload → unbounded latency + OOM | Bounded queues + shed load |
| Retry without backoff/jitter/budget | Retry storm, metastable failure | Jittered backoff + budget + circuit breaker |
| No timeouts (or infinite defaults) | Thread pool exhaustion, cascade | Explicit timeouts at every boundary |
| Deep synchronous call chains | Availability multiplies down | Async events, ≤2–3 hops, composition at edge |
| Deep health checks | One dependency fails ⇒ whole fleet marked down | Shallow liveness, separate readiness |
| Distributed lock without fencing | Zombie leader corrupts data | Lease + fencing token, or consensus |
| JWT as a session with long TTL | Cannot revoke | Short TTL + rotating refresh + introspection |
| `OFFSET` pagination at scale | O(n) scans, drifting results | Keyset/cursor pagination |
| Random UUIDv4 as clustered PK | Index fragmentation, poor locality | UUIDv7/ULID or bigint |
| Cache with no stampede protection | Cache flush ⇒ origin meltdown | Single-flight + jitter + stale-while-revalidate |
| One global cron/leader/config push | Correlated global failure | Per-cell/region, staggered, constant work |
| Read-modify-write without version | Lost updates | Optimistic concurrency or atomic update |
| Snapshot isolation for invariants | Write skew (double-booking) | Serializable, `FOR UPDATE`, or exclusion constraint |
| Elasticsearch/cache as source of truth | Data loss, no transactions | Derived index from a durable system of record |
| Long transactions spanning network calls | Locks held, vacuum blocked, deadlocks | Short transactions; do I/O outside |
| Premature microservices | Distributed debugging with no team scaling need | Modular monolith until a measured pain exists |
| Premature event sourcing | Query pain, schema pain, debugging pain | CRUD + audit log; ES only for real audit/temporal needs |
| Multi-cloud "for portability" | 2× ops cost, lowest-common-denominator design | One cloud well; abstract only commodity layers |
| Metrics labelled with user/request ID | Cardinality explosion, cost blowup | Traces/logs for high cardinality |
| Averaged percentiles | Mathematically meaningless | Histograms/t-digest, aggregate then compute |
| Untested backups | Restore fails when it matters | Scheduled restore drills with measured RTO |
| Config change without rollout controls | Instant global outage | Staged config, validation, auto-revert |
| "We're AP so consistency doesn't matter" | Silent data loss and broken invariants | Name each invariant and its required model |

---

