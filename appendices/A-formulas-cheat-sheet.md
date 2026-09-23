## Appendix A — Numbers & Formulas Cheat Sheet

### Estimation constants

- Seconds/day = **86,400** (≈ 10⁵); seconds/month ≈ 2.6 M; seconds/year ≈ 31.5 M
- 1 M requests/day ≈ **12 QPS**; 100 M/day ≈ **1,160 QPS**; 1 B/day ≈ **11,600 QPS**
- Peak ≈ 2–10× average (use 3× unless you know better); design for peak, bill for average
- 2¹⁰ = 1 K, 2²⁰ = 1 M, 2³⁰ = 1 B, 2⁴⁰ = 1 T; 2⁶⁴ ≈ 1.8 × 10¹⁹
- 1 KB text ≈ 1,000 chars; typical JSON API response 1–10 KB; photo 200 KB–5 MB; 1 min 1080p video ≈ 50–100 MB
- One modern core: ~10–50 k simple ops/s for a service with I/O; a single Postgres node: ~5–50 k simple QPS; Redis: ~100 k+ ops/s/core
- Single commodity server: 8–96 cores, 32–768 GB RAM, 1–10 Gbps NIC, NVMe 100 k–1 M IOPS

### Core formulas

| Quantity | Formula |
|---|---|
| Little's Law | `L = λ × W` (concurrency = arrival rate × latency) |
| Required threads/connections | `λ × W / target_utilization` |
| Utilization | `ρ = λ / (c × μ)`; keep ρ ≤ 0.7 |
| M/M/1 response time | `W = 1 / (μ − λ)` |
| Availability (series) | `A = ∏ Aᵢ` |
| Availability (parallel, independent) | `A = 1 − ∏ (1 − Aᵢ)` |
| Availability from MTBF/MTTR | `A = MTBF / (MTBF + MTTR)` |
| Quorum for strong consistency | `R + W > N` (common: N=3, R=W=2) |
| Fault tolerance (crash) | `N = 2f + 1` |
| Fault tolerance (Byzantine) | `N = 3f + 1` |
| Fan-out slow-request probability | `1 − (1 − p)ⁿ` |
| Bloom filter bits | `m = −n·ln(p) / (ln 2)²`; optimal `k = (m/n)·ln 2` |
| Bandwidth-delay product | `BDP = bandwidth × RTT` (window size needed) |
| Storage/year | `QPS_write × 86400 × 365 × bytes × replication × (1 + index_overhead)` |
| Exponential backoff w/ full jitter | `sleep = random(0, min(cap, base × 2^attempt))` |
| Cache miss cost | `E[latency] = h·t_cache + (1−h)·(t_cache + t_origin)` |
| USL throughput | `C(N) = N / (1 + α(N−1) + βN(N−1))` |

### Worked estimation walkthrough (memorize the shape)

```
Given: 200 M DAU, avg 20 reads + 2 writes per user per day, 1 KB per record,
       3× replication, p99 target 200 ms, 5 years retention.

Reads:  200 M × 20 = 4 B/day  → 4e9 / 86400 ≈ 46 k QPS avg → ~140 k QPS peak (3×)
Writes: 200 M × 2  = 400 M/day → ~4.6 k QPS avg → ~14 k QPS peak
Storage: 400 M × 1 KB = 400 GB/day raw
         × 3 replication = 1.2 TB/day → ~440 TB/year → ~2.2 PB over 5 years
         + indexes (~30%) → ~2.9 PB → tier cold data to object storage
Bandwidth: 140 k QPS × 2 KB response ≈ 280 MB/s ≈ 2.2 Gbps egress (before CDN)
Cache: hot 20% of daily reads ≈ 0.2 × 4 B × 1 KB = 800 GB → ~10 × 96 GB Redis nodes
       (or cut it: cache only the top-N working set and measure hit ratio)
Compute: 140 k QPS × 10 ms service time = 1,400 concurrent (Little's Law)
         at ~200 concurrent/node → ~7 nodes, but for 70% utilization + AZ loss
         headroom (÷0.7, ×1.5) → ~15–20 nodes across 3 AZs
Conclusion: read-dominated 10:1 → cache + read replicas first; writes fit one
            sharded cluster; storage is the dominant cost → tier aggressively.
```

---

