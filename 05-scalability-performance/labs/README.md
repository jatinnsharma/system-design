# Labs — Level 5 — Scalability & Performance Engineering

- [ ] load test a service; plot throughput vs latency vs utilization; find the knee; verify Little's Law holds with your measured numbers.
- [ ] build a cache-aside layer with single-flight + jittered TTL + pub/sub invalidation. Then flush the cache under load and confirm the DB survives.
- [ ] instrument a fan-out endpoint calling 10 services, measure p99 amplification, then add hedging and deadline propagation and re-measure.
- [ ] implement token bucket and sliding-window in Redis with Lua; then implement CoDel-style shedding and prove the service stays at target p99 while rejecting excess.
- [ ] build a feed two ways (fan-out-on-write and fan-out-on-read), then a hybrid. Measure write amplification and read latency for a user with 10 M followers.
