## Appendix F — Lab Track (build these, in order)

Each lab is deliberately small but must be *measured*, not just built. Keep results in a notes file.

1. **L0** Latency lab — measure L1/RAM/SSD/network latencies yourself; build a Bloom filter + Count-Min Sketch.
2. **L0** Concurrency lab — three server models (thread/epoll/async); find the knee for each.
3. **L1** Network lab — Wireshark a handshake; `tc netem` 200 ms + 2% loss; compare H1/H2/H3.
4. **L1** Proxy lab — HAProxy/Envoy over 3 backends; kill nodes under load; compare LB algorithms.
5. **L2** Three-tier lab — LB + app + Postgres + Redis; load test; fix three successive bottlenecks.
6. **L3** Index lab — 10 M rows, 10 queries, index each, prove with `EXPLAIN ANALYZE`, measure write cost.
7. **L3** Isolation lab — reproduce every anomaly in the isolation matrix with two sessions; fix a write-skew bug 4 ways.
8. **L3** Replication lab — primary + 2 replicas; measure lag; force failover; implement read-your-writes.
9. **L3** Sharding lab — hash-shard across 4 DBs; scatter-gather; reshard to 8 with zero downtime.
10. **L3** Backup lab — PITR restore to a timestamp; measure real RTO; online `ALTER` on 50 M rows.
11. **L4** Clock lab — Lamport vs vector clocks; detect a concurrent write conflict.
12. **L4** Raft lab — leader election + log replication for a 3-node KV store; verify with a linearizability checker.
13. **L4** CRDT lab — offline-editable counter + text; converge after a 5-minute partition.
14. **L5** Capacity lab — throughput/latency/utilization curves; verify Little's Law; find the knee.
15. **L5** Cache lab — cache-aside + single-flight + jittered TTL + pub/sub invalidation; survive a full flush under load.
16. **L5** Rate limit lab — token bucket + sliding window in Redis/Lua; then CoDel-style shedding holding p99.
17. **L5** Tail lab — 10-way fan-out; measure p99 amplification; add hedging + deadline propagation.
18. **L6** Resilience lab — timeout + jittered retry + breaker + bulkhead + fallback; fault-inject 100% dependency failure.
19. **L6** Chaos lab — one experiment per failure domain; one documented surprise each.
20. **L6** TLA+ lab — specify a lease-based lock; let the checker find the violation.
21. **L7** Messaging lab — at-least-once + idempotent consumer + dedupe; duplicate/reorder/replay tests.
22. **L7** Kafka lab — 3 brokers; `acks=1` vs `acks=all` data loss under leader kill; rebalance stall measurement.
23. **L7** Outbox lab — order/payment flow via outbox + relay + idempotent consumer; then the same in Temporal.
24. **L7** Streaming lab — event-time windows with out-of-order and late data; compare watermark strategies.
25. **L8** Extraction lab — strangler-fig one service out of a monolith with dual-write + shadow read + cutover.
26. **L9** API lab — cursor pagination + ETag concurrency + idempotency keys + RFC 9457 errors; lint with Spectral.
27. **L10** Auth lab — OIDC PKCE + rotating refresh tokens + ReBAC (OpenFGA) + a 20-case cross-tenant leak test.
28. **L10** Tenancy lab — pooled multi-tenant API with Postgres RLS + per-tenant limits; then plan a bridge migration.
29. **L11** Telemetry lab — OpenTelemetry across 4 services + a Kafka hop; find an injected regression in <3 min.
30. **L11** SLO lab — multi-window burn-rate alerts; delete every non-actionable alert; write a readiness review.
31. **L12** Kubernetes lab — probes + PDB + topology spread + graceful drain; kill a node mid-load-test with zero errors.
32. **L12** Delivery lab — canary with automated rollback; expand/contract column rename across two deploys.
33. **L13** Pipeline lab — Postgres → Debezium → Kafka → Iceberg → dbt → dashboard, with freshness/volume tests.
34. **L13** Analytics lab — ClickHouse per-tenant dashboard, p95 < 200 ms over 1 B rows.
35. **L13** Search lab — BM25 + vector hybrid retrieval + reranking; measure recall@10 on a labelled set.
36. **L13** RAG lab — RAG service with eval set in CI, tenant isolation, and a spend cap.

---

