# System Design Roadmap — Beginner → Principal / Architect

> A living, self-contained curriculum. Every level lists **Topics → Subtopics → Advanced concepts**, the
> **prerequisites** it assumes, **hands-on labs** that prove you learned it, and an **exit gate** you must
> pass before moving on. Nothing is skipped: each level only uses ideas introduced in an earlier level.

- **Owner:** Jatin
- **Created:** 2026-09-08
- **Last updated:** 2026-09-08
- **Status:** Level 0 — not started
- **How to maintain this file:** see [Appendix H — Maintaining This Roadmap](#appendix-h--maintaining-this-roadmap)

---

## How to use this document

1. **Do not read it linearly end-to-end.** Work one level at a time. Each level is ~2–6 weeks of part-time study.
2. **Every level has three passes:**
   - **Pass 1 — Concepts:** read/watch until you can explain each subtopic in 3 sentences without notes.
   - **Pass 2 — Hands-on:** build the lab. Reading about replication teaches nothing; breaking a replica does.
   - **Pass 3 — Design:** apply the level to a design problem from [Level 14](#level-14--case-studies-progressive-difficulty) and write it up.
3. **Write everything down.** One markdown note per subtopic. Your notes are the real artifact; this file is the index.
4. **Trade-offs, not answers.** From Level 5 onward, the correct response to almost every question is
   *"it depends — here are the axes, here is what I'd pick, here is what would change my mind."* Practice that shape.
5. **Track progress** in [Appendix G — Progress Tracker](#appendix-g--progress-tracker). Check boxes only after the exit gate.

### Level map

| Level | Title | Focus | Est. time |
|---|---|---|---|
| 0 | [Prerequisites & Mental Models](#level-0--prerequisites--mental-models) | Machine reality, cost of operations | 3–4 wks |
| 1 | [Networking & The Web](#level-1--networking--the-web) | How bytes actually move | 3–4 wks |
| 2 | [Core Building Blocks](#level-2--core-building-blocks-of-any-system) | The vocabulary of components | 2–3 wks |
| 3 | [Databases & Storage Engines](#level-3--databases--storage-engines-deep-dive) | Where all hard problems live | 6–8 wks |
| 4 | [Distributed Systems Theory](#level-4--distributed-systems-theory) | Why distributed is hard | 6–8 wks |
| 5 | [Scalability & Performance](#level-5--scalability--performance-engineering) | Making it fast under load | 4–6 wks |
| 6 | [Reliability & Resilience](#level-6--reliability-resilience--operability) | Making it survive failure | 4–6 wks |
| 7 | [Messaging & Event-Driven](#level-7--messaging-streaming--event-driven-architecture) | Decoupling in time | 5–6 wks |
| 8 | [Architecture Styles & DDD](#level-8--architecture-styles-boundaries--domain-driven-design) | Drawing boundaries | 4–5 wks |
| 9 | [API & Interface Design](#level-9--api--interface-design) | Contracts that outlive code | 3–4 wks |
| 10 | [Security & Multi-Tenancy](#level-10--security-privacy--multi-tenancy) | Trust, identity, isolation | 4–6 wks |
| 11 | [Observability](#level-11--observability--production-readiness) | Knowing what your system is doing | 3–4 wks |
| 12 | [Infrastructure & Delivery](#level-12--infrastructure-deployment--delivery) | Shipping safely, repeatedly | 4–5 wks |
| 13 | [Data-Intensive & ML Systems](#level-13--data-intensive-analytics--ml-systems) | Analytics, pipelines, AI | 5–7 wks |
| 14 | [Case Studies](#level-14--case-studies-progressive-difficulty) | Whole-system synthesis | ongoing |
| 15 | [Principal / Architect Practice](#level-15--principal--architect-practice) | Judgment, influence, strategy | ongoing |

### Fast-track routes

- **Backend interview in 8 weeks:** L0 (skim) → L1 → L2 → L3 (§3.1–3.6) → L4 (§4.1–4.3) → L5 → L6 (§6.1–6.4) → L7 (§7.1–7.3) → L14 (10 designs) → L15 (§15.1).
- **Already a senior engineer:** exit-gate-test yourself on L0–L2, then start at L3 and do everything.
- **Architect track (already know L0–L9):** L10 → L12 → L13 → L15, with two case studies per week.

---

## Level 0 — Prerequisites & Mental Models

> **Why this level exists:** Every system design decision is a bet about the cost of an operation — a disk
> seek, a network hop, a lock, a context switch. If you don't know those costs in your body, you will
> confidently design systems that cannot work. This is the level most people skip and the reason their
> designs collapse under questioning.

**Assumes:** you can write and run programs in at least one language.

### 0.1 Computer architecture as a performance model

- **Topics**
  - CPU, cores, hardware threads, sockets, NUMA nodes
  - Memory hierarchy: registers → L1/L2/L3 cache → RAM → SSD → HDD → network → tape/cold storage
  - Cache lines (64B), spatial and temporal locality, cache misses
  - Instruction pipelining, branch prediction, speculative execution, SIMD
- **Subtopics**
  - Why array traversal beats linked-list traversal despite identical Big-O
  - False sharing: two threads writing adjacent fields destroy each other's cache
  - Sequential vs random access on SSD vs HDD; why databases are built around this single fact
  - Page size (4KB), page cache, `mmap`, dirty pages, write-back vs write-through
  - DMA, zero-copy (`sendfile`, `splice`) — how Kafka and Nginx get their throughput
- **Advanced**
  - NUMA-aware allocation and thread pinning; why a 96-core box is really 2–8 smaller machines
  - Mechanical sympathy: designing data layout for the hardware (struct-of-arrays vs array-of-structs)
  - Hardware failure statistics: bit rot, silent data corruption, AFR of disks, why checksums are mandatory
  - Modern storage: NVMe queue depth, write amplification, SSD wear levelling, TRIM, persistent memory
- **Hands-on:** write a benchmark that traverses 1 GB sequentially vs randomly, in RAM and on disk. Plot the four numbers. Keep them.

### 0.2 Latency numbers every engineer must know

- **Topics**
  - The canonical latency table (memorize the orders of magnitude, not the digits)
  - Converting latency into throughput and vice versa
- **Reference table (2020s hardware, order-of-magnitude)**

  | Operation | Time | Human-scale |
  |---|---|---|
  | L1 cache reference | ~1 ns | 1 s |
  | Branch mispredict | ~3 ns | 3 s |
  | L2 cache reference | ~4 ns | 4 s |
  | Mutex lock/unlock (uncontended) | ~17 ns | 17 s |
  | Main memory reference | ~100 ns | 1.7 min |
  | Compress 1 KB with snappy | ~2 µs | 33 min |
  | Read 1 MB sequentially from memory | ~50 µs | 14 h |
  | SSD random read (NVMe) | ~50–150 µs | ~1 day |
  | Read 1 MB sequentially from NVMe SSD | ~100–300 µs | ~2 days |
  | Round trip within same datacenter | ~0.5 ms | 6 days |
  | HDD seek | ~5–10 ms | 2–4 months |
  | Read 1 MB sequentially from HDD | ~5–20 ms | ~3 months |
  | Cross-region RTT (US-East ↔ US-West) | ~60–70 ms | ~2 years |
  | Cross-continent RTT (US ↔ EU) | ~80–100 ms | ~3 years |
  | US ↔ Australia RTT | ~150–200 ms | ~5 years |
  | TLS handshake (new, 2-RTT) | 2 × RTT | — |
  | Speed of light in fibre | ~200,000 km/s (~5 µs/km) | — |

- **Subtopics**
  - Why the speed of light is a hard architectural constraint (Sydney can never see US-East writes in <70 ms)
  - Bandwidth vs latency vs throughput vs goodput vs IOPS — four different bottlenecks
  - Latency budgets: a 200 ms page budget spent across DNS, TLS, gateway, 6 services, 2 DBs, and render
- **Advanced**
  - Latency as a *distribution*, never a mean: p50/p90/p99/p99.9/p99.99, and why the mean is a lie
  - Fan-out amplification: with 100 parallel calls, p99 of one call becomes ~p63 of the request
  - Coordinated omission — why your load test's p99 is optimistic
- **Hands-on:** compute the theoretical minimum latency for a user in Mumbai calling a service in Virginia, then measure it.

### 0.3 Data structures & algorithms as design tools

- **Topics**
  - Arrays, dynamic arrays, linked lists, stacks, queues, deques, ring buffers
  - Hash tables: hashing, collisions (chaining vs open addressing), load factor, resizing
  - Trees: BST, balanced trees (AVL/red-black), B-tree/B+tree, tries, heaps, skip lists
  - Graphs: representations, BFS/DFS, topological sort, shortest path
  - Sorting & searching, external sorting (merge sort on disk)
- **Subtopics**
  - Amortized vs worst-case complexity; why p99 cares about worst-case (rehash pauses, GC pauses)
  - Why B+trees dominate on-disk indexes: fanout, height 3–4 for billions of rows, node = page
  - LSM-tree structure: memtable, SSTables, levels, compaction (contrast with B-tree in Level 3)
  - Priority queues for schedulers, timers, rate limiters, delayed queues
- **Advanced probabilistic & sketch structures** (used constantly in real systems)
  - **Bloom filter** — membership with false positives; sizing formula `m = -n·ln(p)/(ln2)²`; used in LSM reads, cache filtering
  - **Cuckoo filter / Quotient filter** — deletable alternatives to Bloom
  - **HyperLogLog** — cardinality in ~1.5 KB with ~2% error; unique visitors at scale
  - **Count-Min Sketch** — frequency estimation; heavy hitters, hot-key detection
  - **t-digest / HdrHistogram** — mergeable percentile estimation for metrics
  - **MinHash / SimHash / LSH** — near-duplicate detection, similarity search
  - **Merkle trees** — anti-entropy repair (Dynamo/Cassandra), Git, blockchains
  - **Consistent hashing & rendezvous (HRW) hashing** — key placement with minimal reshuffling; virtual nodes
  - **Inverted index** — the core of search; postings lists, skip pointers
  - **Trie / FST** — autocomplete, IP routing tables
  - **Roaring bitmaps** — compressed set operations for analytics
  - **Ring buffer / disruptor pattern** — lock-free single-producer/single-consumer pipelines
- **Hands-on:** implement a Bloom filter and a Count-Min Sketch from scratch. Measure false-positive rate vs your formula.

### 0.4 Operating systems for system designers

- **Topics**
  - Processes vs threads vs coroutines; the scheduler; context switch cost (~1–5 µs)
  - Virtual memory, page tables, TLB, swap, OOM killer
  - File systems: inodes, page cache, `fsync`, journaling, durability guarantees
  - I/O models: blocking, non-blocking, I/O multiplexing (`select`/`poll`/`epoll`/`kqueue`), async (`io_uring`), signal-driven
  - System calls, user vs kernel space, the cost of crossing the boundary
- **Subtopics**
  - Thread-per-request vs event loop vs async runtimes vs goroutines/green threads
  - The C10K → C10M problem and what actually solved it
  - `ulimit`, file descriptors as a hard scaling limit, ephemeral port exhaustion (~28k per tuple)
  - cgroups & namespaces — the actual mechanism behind containers
  - CPU throttling in containers (CFS quota) and why it destroys p99 latency
- **Advanced**
  - `fsync` semantics, write barriers, disk cache lies, `O_DIRECT`, and how databases achieve durability
  - Copy-on-write, `fork` semantics, why Redis `BGSAVE` can double memory
  - Huge pages, transparent huge pages, and their latency side effects
  - Kernel bypass: DPDK, SR-IOV, user-space TCP; when this matters (trading, CDNs)
- **Hands-on:** write a TCP echo server three ways — thread-per-connection, `epoll` event loop, async runtime. Load test all three to find the knee.

### 0.5 Concurrency & parallelism

- **Topics**
  - Concurrency vs parallelism; Amdahl's law; Gustafson's law
  - Race conditions, critical sections, mutual exclusion
  - Locks: mutex, spinlock, read-write lock, reentrant lock
  - Deadlock (4 Coffman conditions), livelock, starvation, priority inversion
  - Atomics, compare-and-swap, memory ordering, `volatile`, memory barriers
- **Subtopics**
  - Lock contention and convoying; lock granularity; sharded locks/striping
  - Optimistic vs pessimistic concurrency control (returns in Level 3 as MVCC vs 2PL)
  - Producer–consumer, thread pools, work stealing, bounded queues as backpressure
  - Immutability and copy-on-write as concurrency strategies
  - Thundering herd, cache stampede
- **Advanced**
  - Lock-free and wait-free algorithms; ABA problem; hazard pointers; epoch-based reclamation
  - Memory models: sequential consistency, happens-before, acquire/release, relaxed ordering
  - Actor model, CSP/channels, software transactional memory
  - False sharing and padding; per-core data structures
  - Little's law applied to thread pools: `concurrency = arrival_rate × latency`
- **Hands-on:** build a bounded thread-safe queue; then deliberately create a deadlock and a livelock and diagnose both from a thread dump.

### 0.6 Reliability arithmetic & back-of-envelope estimation

- **Topics**
  - Powers of 2 and 10; bytes/KB/MB/GB/TB/PB; seconds in a day (86,400) / year (~31.5 M)
  - QPS ↔ daily volume conversions; peak-to-average ratio (typically 2–10×)
  - Storage estimation: rows × row size × replication × retention × index overhead × growth
  - Bandwidth estimation: QPS × payload size; ingress vs egress (egress is what you pay for)
- **Subtopics**
  - Availability math: nines table, series vs parallel composition
  - Cost estimation: compute, storage, egress, managed-service premiums
  - Read:write ratio as the single most important number in a design
- **Nines reference**

  | Availability | Downtime/year | Downtime/month | Downtime/week |
  |---|---|---|---|
  | 99% | 3.65 d | 7.3 h | 1.7 h |
  | 99.9% | 8.77 h | 43.8 m | 10.1 m |
  | 99.95% | 4.38 h | 21.9 m | 5 m |
  | 99.99% | 52.6 m | 4.4 m | 1 m |
  | 99.999% | 5.26 m | 26 s | 6 s |

- **Advanced**
  - Series dependency: 10 services at 99.9% each ⇒ 99.0% overall. Dependencies multiply.
  - Parallel redundancy: 2 replicas at 99% ⇒ 99.99% *if failures are independent* (they rarely are)
  - Correlated failure: shared power, shared control plane, shared config push, shared deploy — the real killer
  - MTBF, MTTR, MTTD, MTTF; `availability = MTBF / (MTBF + MTTR)` ⇒ **reducing MTTR is usually cheaper than raising MTBF**
  - Birthday-paradox collision math for ID generation; UUID/ULID collision probability
  - Queueing theory preview: utilization ρ, and why latency → ∞ as ρ → 1 (`M/M/1: W = 1/(μ-λ)`)
- **Worked example to reproduce from scratch**
  - 100 M DAU, 10 actions/user/day ⇒ 1 B req/day ⇒ ~11.6 k QPS average ⇒ ~35–60 k QPS peak
  - 2 KB/record ⇒ 2 TB/day raw ⇒ ×3 replication ⇒ 6 TB/day ⇒ ~2.2 PB/year
  - At 60 k QPS and 5 ms/request per core, Little's law ⇒ 300 concurrent requests ⇒ ~300 cores minimum, so ~40 × 8-core nodes at 70% target utilization plus headroom
- **Hands-on:** estimate storage/bandwidth/servers for Instagram, WhatsApp, and Uber. Compare against published engineering blogs.

### 0.7 Thinking tools & the design method

- **Topics**
  - The 7-step design method: **Requirements → Constraints → API → Data model → High-level design → Deep dives → Bottlenecks & evolution**
  - Functional vs non-functional requirements (NFRs)
  - Trade-off vocabulary: latency vs throughput, consistency vs availability, cost vs performance, simplicity vs flexibility, coupling vs autonomy
- **Subtopics**
  - Requirement clarification questions checklist (scale, read/write ratio, consistency, latency SLO, geography, retention, growth, budget, team size)
  - Drawing systems: C4 model (Context, Container, Component, Code); sequence diagrams; data-flow diagrams
  - Napkin math before diagrams; diagrams before code
- **Advanced**
  - Explicitly naming your **failure domains** and **blast radius** in every design
  - Reversible vs irreversible decisions (one-way vs two-way doors) — invest review effort proportionally
  - YAGNI vs designing for 10× (rule of thumb: design for 10×, plan to rewrite at 100×)
  - Second-system effect, premature abstraction, resume-driven architecture — named anti-patterns
- **Hands-on:** design a URL shortener using the 7 steps in exactly 45 minutes, timed, on paper.

### ✅ Exit gate for Level 0

You can, without notes:
1. Recite the latency table to the right order of magnitude and explain why cross-region writes cap at ~70 ms RTT.
2. Explain why a B+tree suits disk and a hash table suits memory.
3. Compute availability of a 6-service serial call chain, and say whether MTBF or MTTR is the better lever.
4. Estimate servers, storage, and bandwidth for a 50 M DAU app in under 10 minutes.
5. Explain why p99 latency is more actionable than average latency, with a fan-out example.

---

## Level 1 — Networking & The Web

> **Why this level exists:** Every distributed system is a set of programs shouting across an unreliable
> network. Almost every "mysterious" production bug — timeouts, half-open connections, retry storms,
> TLS costs, DNS caching, head-of-line blocking — is a networking fact you didn't know.

**Assumes:** Level 0 (latency numbers, I/O models, file descriptors).

### 1.1 The layered model & IP

- **Topics**
  - OSI 7 layers vs TCP/IP 4 layers; encapsulation; MTU (1500 typical) and fragmentation
  - Ethernet/ARP basics; switches vs routers; MAC vs IP
  - IPv4 addressing, CIDR notation, subnetting, private ranges (RFC 1918), NAT, PAT
  - IPv6: address format, no NAT, dual-stack, why adoption matters for mobile
  - Routing: BGP, ASes, anycast, route leaks/hijacks
- **Subtopics**
  - VPC/subnet/route-table/security-group/NACL model in clouds
  - Public vs private subnets, NAT gateway, egress cost, VPC peering, transit gateway, PrivateLink
  - Jumbo frames, path MTU discovery and black-hole MTU bugs
- **Advanced**
  - **Anycast** for DNS/CDN/edge: one IP announced from many locations; how it enables DDoS absorption
  - BGP-based traffic engineering and how a bad BGP announcement takes a company offline
  - Network partitions in practice: asymmetric partitions, gray failures, partial reachability

### 1.2 TCP, UDP, and QUIC

- **Topics**
  - TCP: 3-way handshake, sequence/ack numbers, sliding window, 4-way close, TIME_WAIT
  - Reliability: retransmission, RTO, fast retransmit, SACK
  - Flow control (receiver window) vs congestion control (network)
  - UDP: connectionless, unordered, no congestion control — when that's the right choice
- **Subtopics**
  - Congestion control algorithms: Reno, CUBIC (default on Linux), BBR (Google), and their different fairness/latency behaviour
  - Slow start, congestion avoidance, congestion window; why short connections never reach full bandwidth
  - Bandwidth-delay product; why a 100 ms RTT link needs a large window for high throughput
  - Nagle's algorithm + delayed ACK interaction (the classic 40 ms stall); `TCP_NODELAY`
  - Keep-alives, half-open connections, why you need application-level heartbeats
  - Backlog queue, SYN flood, SYN cookies
  - Head-of-line blocking at the TCP level
- **Advanced**
  - **QUIC**: UDP-based, 0/1-RTT handshake, per-stream loss recovery (kills HOL blocking), connection migration across networks
  - Buffer bloat and Active Queue Management (CoDel, FQ-CoDel)
  - TCP tuning for servers: `somaxconn`, `tcp_tw_reuse`, window scaling, ephemeral port ranges
  - Ephemeral port exhaustion math: ~28,000 ports per (src IP, dst IP, dst port) tuple — a real scaling wall for service meshes and proxies
- **Hands-on:** capture a TCP handshake and a QUIC handshake in Wireshark. Then use `tc netem` to add 200 ms latency + 2% loss and observe throughput collapse.

### 1.3 DNS

- **Topics**
  - Hierarchy: root → TLD → authoritative; resolvers (recursive vs iterative)
  - Record types: A, AAAA, CNAME, ALIAS/ANAME, MX, TXT, SRV, NS, SOA, CAA, PTR
  - TTL and caching layers (browser, OS, resolver, application runtime)
  - Registrars, zones, delegation, glue records
- **Subtopics**
  - DNS-based load balancing: round-robin, weighted, latency-based, geo-based, failover routing policies
  - Health checks + DNS failover, and why TTL makes DNS failover slow (minutes, not seconds)
  - Split-horizon DNS; internal service discovery via DNS (Kubernetes `ClusterIP` + CoreDNS)
  - Negative caching, DNS pinning in JVM/other runtimes (a classic outage cause)
- **Advanced**
  - DNSSEC, DoH/DoT, and their privacy/debuggability trade-offs
  - GSLB (global server load balancing) vs anycast: control vs simplicity
  - EDNS Client Subnet and its effect on CDN accuracy
  - DNS as a hidden single point of failure (Dyn 2016, and every "it was DNS" postmortem)
- **Hands-on:** trace a lookup with `dig +trace`. Set up a domain with weighted + failover routing and measure real failover time.

### 1.4 HTTP & the web transport stack

- **Topics**
  - HTTP semantics: methods, status codes, headers, bodies
  - Safe / idempotent / cacheable method matrix (GET safe+idempotent+cacheable; PUT/DELETE idempotent; POST neither; PATCH neither by default)
  - Status code families and the ones that matter operationally: 400, 401, 403, 404, 409, 412, 422, 425, 428, 429, 499, 500, 502, 503, 504
  - Content negotiation, compression (gzip/brotli/zstd), chunked transfer encoding
  - Cookies, `SameSite`, `Secure`, `HttpOnly`, CORS + preflight
- **Subtopics**
  - HTTP/1.1: keep-alive, pipelining (dead), 6-connections-per-origin limit, domain sharding (obsolete)
  - HTTP/2: binary framing, multiplexed streams, HPACK header compression, server push (deprecated), stream prioritization, TCP-level HOL blocking remains
  - HTTP/3: HTTP over QUIC, QPACK, no HOL blocking, better mobile behaviour
  - Caching headers in depth: `Cache-Control` (max-age, s-maxage, no-store, no-cache, private, public, immutable, stale-while-revalidate, stale-if-error), `ETag`/`If-None-Match`, `Last-Modified`/`If-Modified-Since`, `Vary`, `Age`
  - Conditional requests for optimistic concurrency (`If-Match` + ETag = compare-and-swap over HTTP)
  - Range requests, resumable uploads/downloads
- **Advanced**
  - Cache key design and cache poisoning; `Vary` explosion; the "unkeyed input" class of vulnerabilities
  - Connection pooling and multiplexing behaviour of your HTTP client — the most common source of production stalls
  - Timeouts you must set explicitly: connect, TLS, read/write, total request, idle, pool-acquire. **Defaults are almost always infinite or wrong.**
  - Retry safety: only retry idempotent operations, or use idempotency keys (Level 9)
  - `Retry-After`, 429 handling, and cooperative backoff
- **Hands-on:** serve the same page over H1, H2, H3 and compare waterfall + p95 on a lossy link. Implement ETag-based optimistic concurrency on a PUT endpoint.

### 1.5 TLS & transport security

- **Topics**
  - Symmetric vs asymmetric crypto; hashes; HMAC; digital signatures
  - TLS 1.2 vs 1.3 handshakes; cipher suites; forward secrecy (ECDHE)
  - Certificates, CAs, chains of trust, SNI, wildcard vs SAN certs, expiry
  - HTTPS, HSTS, certificate transparency, OCSP/OCSP stapling, CRLs
- **Subtopics**
  - Session resumption, session tickets, TLS 1.3 0-RTT (and its replay risk)
  - TLS termination points: CDN edge → LB → service; where each hop decrypts and what that means for trust
  - mTLS: client certs, rotation, SPIFFE/SPIFFE IDs (returns in Level 10 and service mesh)
  - Cost of TLS: CPU per handshake, why connection reuse and session resumption dominate
- **Advanced**
  - Certificate lifecycle automation (ACME/Let's Encrypt, cert-manager), rotation, pinning (and why pinning bricks apps)
  - Post-quantum readiness, hybrid key exchange
  - End-to-end vs point-to-point encryption; encrypting payloads *inside* the transport for zero-trust internals
- **Hands-on:** stand up TLS with your own CA and enforce mTLS between two services. Then let a cert expire on purpose and watch the failure mode.

### 1.6 Proxies, load balancers & the edge

- **Topics**
  - Forward proxy vs reverse proxy; L4 (TCP) vs L7 (HTTP) load balancing
  - LB algorithms: round-robin, weighted RR, least-connections, least-response-time, random-two-choices, IP hash, consistent hash
  - Health checks: active vs passive, shallow vs deep, and health-check-induced outages
  - Sticky sessions/session affinity — and why statelessness is better
  - TLS termination vs passthrough; SSL offload
- **Subtopics**
  - Load balancer tiers: DNS → anycast edge → L4 → L7 → service mesh sidecar
  - Direct Server Return (DSR), NAT mode, connection draining, slow start on new nodes
  - Nginx / HAProxy / Envoy / cloud ALB-NLB feature comparison
  - **Power of two random choices** — why it approaches optimal balancing with almost no coordination
  - Outlier detection and automatic ejection
- **Advanced**
  - Global load balancing: latency-based, geo-based, capacity-aware; **shuffle sharding** for blast-radius reduction
  - Load balancer as a control plane: xDS, dynamic endpoint discovery
  - Queueing at the LB: what happens when all backends are busy (queue vs shed — see Level 5)
  - Client-side load balancing (gRPC, Finagle) vs proxy-based: fewer hops vs harder rollout
  - Zone-aware routing to avoid cross-AZ data transfer cost and latency
- **Hands-on:** put HAProxy or Envoy in front of 3 backends, kill one mid-load-test, and measure error count and recovery time under each algorithm.

### 1.7 CDN & content delivery

- **Topics**
  - PoPs, edge vs origin, cache hit ratio, origin shield, tiered caching
  - Static vs dynamic content; cache-busting via versioned URLs; purge/invalidation
  - Push vs pull CDNs
- **Subtopics**
  - Cache key composition (URL + query + headers + cookies + device class)
  - Range requests and large media; HLS/DASH segmenting for video
  - Signed URLs / signed cookies for private content
  - Edge compute: Cloudflare Workers / Lambda@Edge / Fastly Compute — auth, A/B, personalization at the edge
- **Advanced**
  - Cache stampede at the edge; request coalescing/collapsing
  - Multi-CDN strategies, CDN failover, RUM-driven CDN steering
  - Image/video optimization pipelines at the edge; adaptive bitrate; per-device transcoding
  - Cost model: egress-dominated economics; why cache hit ratio is a P&L line item
- **Hands-on:** deploy static assets behind a CDN, measure hit ratio, then deliberately break `Cache-Control` and watch origin load explode.

### 1.8 Application protocols beyond request/response

- **Topics**
  - REST over HTTP (deep-dived in Level 9)
  - WebSockets: handshake/upgrade, frames, ping/pong, subprotocols
  - Server-Sent Events (SSE); long polling; short polling — the full push-vs-pull spectrum
  - gRPC & HTTP/2 streaming: unary, server-stream, client-stream, bidi
  - GraphQL over HTTP; subscriptions
- **Subtopics**
  - Choosing a real-time transport: SSE for one-way server push, WebSocket for bidirectional, long-poll as fallback
  - Sticky routing problems for stateful connections; connection-count-based scaling (not CPU-based)
  - Presence, heartbeats, reconnect with exponential backoff + jitter, resume tokens/cursors
  - Pub/sub fan-out to millions of connections: connection gateway + broker topology
  - MQTT/CoAP for IoT; AMQP; SMTP; FTP/SFTP; WebRTC (STUN/TURN/ICE) for peer-to-peer media
- **Advanced**
  - Designing a WebSocket gateway tier: connection registry, routing by user/room, backpressure per connection, graceful drain on deploy
  - Message ordering, deduplication, and gap detection on a resumed real-time stream (patch-not-refetch: apply the delivered payload, don't re-poll)
  - Protocol negotiation and graceful degradation across corporate proxies
  - Binary serialization: Protobuf, Avro, Thrift, FlatBuffers, Cap'n Proto, MessagePack vs JSON — size, speed, schema evolution
- **Hands-on:** build a chat room with WebSockets that survives a rolling deploy without losing messages (hint: resume cursor + drain period).

### ✅ Exit gate for Level 1

1. Draw the full path of an HTTPS request from browser to service, naming every hop and every cache.
2. Explain HOL blocking at both the HTTP/1.1 and TCP layers, and how H2 and H3 each address it.
3. List all six timeouts an HTTP client must set and what breaks if each is missing.
4. Pick a real-time transport for three different scenarios and defend each choice.
5. Explain why DNS failover takes minutes and what to use when you need seconds.

---

## Level 2 — Core Building Blocks of Any System

> **Why this level exists:** You need a shared vocabulary of components before you can compose them.
> This level is deliberately broad and shallow; Levels 3–13 go deep on each box.

**Assumes:** Levels 0–1.

### 2.1 The canonical web architecture

- **Topics**
  - Client → DNS → CDN → edge/WAF → load balancer → API gateway → services → cache → database → queue → workers → object store → analytics
  - Vertical vs horizontal scaling; stateless service tier as the enabling idea
  - Read replicas, caching layers, async workers — the three standard first moves
- **Subtopics**
  - Where state lives: client, session store, cache, primary DB, object store, warehouse, archive
  - Synchronous request path vs asynchronous background path; keep the user-facing path short
  - The "single server → 10 M users" progression: split tiers, add cache, add replicas, shard, add queues, split services, go multi-region
- **Advanced**
  - Control plane vs data plane separation, and why control-plane outages are the worst outages
  - Cell-based (bulkheaded) architecture as the end-state of blast-radius thinking (Level 6/8)
- **Hands-on:** deploy a 3-tier app (LB + 2 app nodes + DB + Redis) and load test it. Find and fix the first bottleneck. Repeat three times.

### 2.2 Compute options

- **Topics**
  - Bare metal, VMs, containers, serverless functions, edge functions
  - Autoscaling: reactive (CPU/RPS), scheduled, predictive; scale-to-zero
  - Instance families: CPU-, memory-, storage-, network-, GPU-optimized
- **Subtopics**
  - Cold starts, warm pools, provisioned concurrency
  - Spot/preemptible instances and interruption-tolerant workload design
  - Right-sizing, bin packing, reservations vs on-demand vs savings plans
- **Advanced**
  - Serverless trade-offs: limits on duration/memory/concurrency, connection-pool exhaustion against relational DBs (use a pooler/proxy), vendor coupling
  - Multi-tenant noisy neighbours; burstable (T-class) credit exhaustion cliffs
  - Choosing compute by workload shape: spiky+stateless → serverless; steady+high-throughput → containers; latency-critical+tuned → dedicated

### 2.3 Storage options

- **Topics**
  - Block storage (EBS-like), file storage (NFS/EFS), object storage (S3-like)
  - Storage classes/tiers: hot, warm, cold, archive; lifecycle policies
  - Durability vs availability (11 nines durability ≠ always reachable)
- **Subtopics**
  - Object storage semantics: flat keyspace, key prefixes and request-rate scaling, multipart upload, presigned URLs, versioning, object lock/WORM, events on write
  - Read-after-write consistency guarantees; strong vs eventual listing
  - When to use object storage as a system of record (media, logs, backups, data lake)
- **Advanced**
  - Erasure coding vs replication (storage overhead vs rebuild cost vs latency)
  - Write-once-read-many patterns; immutable data lakes; compaction of small files
  - Direct-to-storage uploads with presigned URLs to keep large payloads off your app tier
  - Cross-region replication, requester-pays, egress economics

### 2.4 Caching (introduction; deepened in Level 5)

- **Topics**
  - Cache layers: client → CDN → API gateway → application (local/in-process) → distributed (Redis/Memcached) → DB buffer pool → OS page cache
  - Patterns: cache-aside (lazy), read-through, write-through, write-behind (write-back), refresh-ahead
  - Eviction: LRU, LFU, FIFO, ARC, TinyLFU/W-TinyLFU, TTL-based
- **Subtopics**
  - Cache hit ratio, working-set sizing, cost per hit vs cost per miss
  - Invalidation strategies: TTL, explicit purge, versioned keys, event-driven invalidation
  - Negative caching; caching "not found" to protect the DB
- **Advanced**
  - Thundering herd / cache stampede mitigation: request coalescing (single-flight), probabilistic early expiry, staggered TTL jitter, `stale-while-revalidate`
  - Hot-key problem: key splitting, local L1 in front of L2, replicated hot keys
  - Consistency: TTL vs write-through vs CDC-driven invalidation; the unavoidable staleness window
  - Redis vs Memcached; Redis data structures as system primitives (sorted sets for leaderboards/rate limits, streams, HyperLogLog, geo, pub/sub, Lua for atomicity)

### 2.5 Databases (introduction; deepened in Level 3)

- **Topics**
  - Relational vs document vs key-value vs wide-column vs graph vs time-series vs search vs vector vs columnar
  - OLTP vs OLAP vs HTAP
  - ACID vs BASE; when each is the honest answer
- **Subtopics**
  - Primary/replica topologies, connection pooling, migrations
  - Polyglot persistence: pick storage per access pattern, pay in operational complexity
- **Advanced**
  - The default recommendation and why: **start with one well-run relational database**; add specialized stores only when a measured access pattern demands it

### 2.6 Asynchronous processing

- **Topics**
  - Message queues vs pub/sub vs log-based streams
  - Workers, job scheduling, cron, delayed jobs, priority queues
  - Idempotency, retries, dead-letter queues
- **Subtopics**
  - Why async: latency hiding, load smoothing, decoupling, retryability, fan-out
  - Task granularity, batching, poison messages, visibility timeouts
- **Advanced**
  - Backpressure across an async chain; queue depth as the primary health signal
  - Exactly-once as an illusion: at-least-once + idempotency is the real design (Level 7)

### 2.7 Supporting components

- **Topics**
  - API gateway: routing, auth, rate limiting, request/response transformation, quotas, versioning
  - Service discovery & registry; health checks; DNS-based vs registry-based
  - Configuration & secrets management; dynamic config; feature flags
  - Search engine (inverted index; Elasticsearch/OpenSearch/Solr)
  - Blob/media pipeline: upload → virus scan → transcode → thumbnail → CDN
  - Notification fan-out: email, SMS, push (APNs/FCM), in-app, webhooks
  - Scheduler/orchestrator for workflows (Temporal, Airflow, Step Functions)
  - ID generation: auto-increment, UUIDv4/v7, ULID, KSUID, Snowflake, DB ticket servers
- **Subtopics**
  - ID trade-offs: monotonicity (index locality!) vs unpredictability vs coordination cost vs size
  - Why random UUIDv4 primary keys hurt B-tree insert locality, and why UUIDv7/ULID fix it
  - Webhook design: signing, retries with backoff, replay protection, at-least-once semantics, consumer idempotency
- **Advanced**
  - Snowflake ID layout: timestamp | datacenter | worker | sequence — and clock-skew handling
  - Multi-region ID generation without coordination
  - Feature flags as an architectural tool: dark launches, gradual rollout, kill switches, experiment assignment

### ✅ Exit gate for Level 2

1. Draw the canonical architecture from memory and name the failure mode of every box.
2. For a given read-heavy workload, choose cache pattern + eviction + invalidation strategy and justify it.
3. Explain three ways to mitigate a cache stampede.
4. Pick an ID scheme for a sharded, multi-region system and defend it.
5. Decide async vs sync for five concrete operations (payment, email, thumbnail, search index update, audit log).

---

## Level 3 — Databases & Storage Engines (Deep Dive)

> **Why this level exists:** Almost every serious system design problem is a data problem. Architects are
> separated from senior engineers mainly by depth here: indexes, isolation levels, replication lag,
> sharding, and the physical layout of bytes on disk.

**Assumes:** Level 0 (B-trees, memory hierarchy, `fsync`), Level 1 (connections, timeouts).

### 3.1 Relational fundamentals & data modelling

- **Topics**
  - Relational algebra basics; SQL: DDL, DML, joins, aggregation, subqueries, CTEs, window functions
  - Keys: primary, foreign, composite, natural vs surrogate; unique constraints
  - Normalization: 1NF → 2NF → 3NF → BCNF → 4NF/5NF; functional dependencies
  - Denormalization as a deliberate, measured optimization
- **Subtopics**
  - Cardinality and relationship modelling (1:1, 1:N, M:N with join tables)
  - Data types and their real cost (`TEXT` vs `VARCHAR`, `NUMERIC` vs float for money, `TIMESTAMPTZ` always, enums vs lookup tables)
  - Nullability, defaults, check constraints, and pushing invariants into the schema
  - Soft deletes, audit columns, temporal/bi-temporal tables, slowly changing dimensions
  - Modelling hierarchies: adjacency list, path enumeration, nested sets, closure table, recursive CTEs
  - Multi-tenancy schema options: shared table + tenant_id, schema-per-tenant, DB-per-tenant (revisited in Level 10)
- **Advanced**
  - Anti-patterns: EAV, polymorphic FKs, "one big table", JSON columns as a schema-avoidance device
  - Schema evolution as a first-class concern: additive-only changes, expand/contract migration, backfills
  - Constraint enforcement in app vs DB: the DB is the only place that actually holds under concurrency
- **Hands-on:** model a booking/scheduling domain (users, providers, availability, appointments, overlapping-slot prevention) with real constraints — including an exclusion constraint that makes double-booking impossible.

### 3.2 Indexing

- **Topics**
  - What an index is; clustered vs non-clustered/secondary; heap vs index-organized tables
  - B+tree index mechanics: order, fanout, height, page splits, fill factor
  - Composite indexes and the **leftmost-prefix rule**; column order matters
  - Covering indexes and index-only scans; INCLUDE columns
  - Partial/filtered indexes; expression/functional indexes; unique indexes
- **Subtopics**
  - Index selectivity, cardinality, and when a full scan is genuinely faster
  - Hash indexes; GIN/GiST (Postgres) for arrays, JSONB, full-text, geometry; BRIN for huge append-only tables
  - Bitmap indexes and bitmap heap scans
  - Write amplification: every index slows down writes and consumes cache
  - Index bloat, `REINDEX`, `VACUUM`, fillfactor tuning, HOT updates
  - Foreign-key indexes (frequently forgotten, cause lock escalation and slow deletes)
- **Advanced**
  - Read the query plan: `EXPLAIN (ANALYZE, BUFFERS)`; seq scan vs index scan vs index-only vs bitmap; nested loop vs hash vs merge join
  - Statistics, histograms, n_distinct, correlated-column mis-estimation, extended statistics
  - Query planner defeat: functions on indexed columns, implicit casts, `OR` chains, leading wildcards, low-selectivity predicates
  - Index-only pagination; **keyset/cursor pagination** vs `OFFSET` (OFFSET is O(n) and drifts under writes)
  - Concurrent index builds (`CREATE INDEX CONCURRENTLY`) and lock-free migrations
  - Multi-column ordering + index for `ORDER BY ... LIMIT` (top-N without sorting)
  - Search-specific indexing: inverted index, tokenization, stemming, tf-idf/BM25 (Level 13)
  - Vector indexes: HNSW, IVF-PQ, recall/latency trade-offs (Level 13)
- **Hands-on:** take a 10 M-row table; write 10 queries; make each one fast using an index; prove it with `EXPLAIN ANALYZE`; then measure the write-throughput cost of the indexes you added.

### 3.3 Transactions, isolation & concurrency control

- **Topics**
  - ACID precisely defined: Atomicity, Consistency, Isolation, Durability
  - Anomalies: dirty read, dirty write, non-repeatable read, phantom read, read skew, write skew, lost update
  - Standard isolation levels: Read Uncommitted, Read Committed, Repeatable Read, Serializable
  - What each level actually prevents, and each engine's real (non-standard) behaviour
- **Anomaly ↔ isolation matrix**

  | Anomaly | RU | RC | RR | Snapshot | Serializable |
  |---|---|---|---|---|---|
  | Dirty read | ✗ | ✓ | ✓ | ✓ | ✓ |
  | Lost update | ✗ | ✗ | ✓* | ✓* | ✓ |
  | Non-repeatable read | ✗ | ✗ | ✓ | ✓ | ✓ |
  | Phantom read | ✗ | ✗ | ✓* | ✓ | ✓ |
  | Read skew | ✗ | ✗ | ✓ | ✓ | ✓ |
  | **Write skew** | ✗ | ✗ | ✗ | **✗** | ✓ |

  ✓ = prevented, ✗ = possible, ✓* = engine-dependent (MySQL RR uses gap locks; Postgres RR is snapshot isolation)

- **Subtopics**
  - Pessimistic concurrency: 2-phase locking (2PL), shared/exclusive locks, intent locks, gap/next-key locks, `SELECT ... FOR UPDATE` / `FOR SHARE` / `SKIP LOCKED` / `NOWAIT`
  - Optimistic concurrency: version columns, compare-and-swap updates, retry loops
  - MVCC: row versions, transaction IDs, visibility rules, snapshots, undo logs vs version chains
  - Vacuum/garbage collection of dead tuples; long-running transactions blocking cleanup; XID wraparound
  - Deadlock detection vs timeout; consistent lock ordering as prevention
  - Lock escalation, lock waits, `pg_locks`/`innodb_trx` diagnosis
- **Advanced**
  - **Write skew and the phantom problem** — the anomaly that silently breaks business invariants under snapshot isolation (double-booking, balance-constraint violations); fixes: `SELECT FOR UPDATE` on a parent row, materializing the conflict, exclusion constraints, or Serializable
  - Serializable Snapshot Isolation (SSI) — optimistic serializability, false-positive aborts, retry requirement
  - Durability mechanics: WAL/redo log, checkpoints, group commit, `fsync` batching, `synchronous_commit` levels, and the durability/latency dial
  - ARIES recovery: analysis → redo → undo; crash consistency; torn pages, double-write buffer, full-page writes
  - Transaction chopping and long-transaction avoidance; why transactions must not span user think-time or network calls
  - Distributed transactions preview: 2PC/3PC, XA, saga — see Level 4/7
- **Hands-on:** reproduce every row of the anomaly table above with two concurrent `psql`/`mysql` sessions. Then create a write-skew bug (two bookings for the same slot) and fix it four different ways.

### 3.4 Storage engines: B-tree vs LSM

- **Topics**
  - Page-oriented B-tree engines (InnoDB, Postgres heap+btree)
  - Log-structured merge trees (LevelDB, RocksDB, Cassandra, ScyllaDB, HBase)
  - The three amplifications: read, write, space — you can only pick two
- **Subtopics**
  - LSM path: WAL → memtable (skip list) → immutable memtable → flush to SSTable → compaction
  - Compaction strategies: size-tiered (STCS), leveled (LCS), time-window (TWCS), universal; their space/write trade-offs
  - Bloom filters + sparse index + block cache to keep LSM reads fast
  - Tombstones, delete handling, and tombstone-driven read degradation
  - B-tree: in-place update, page splits/merges, WAL for crash safety, buffer pool management
- **Advanced**
  - Choosing engine by workload: write-heavy + range scans → LSM; read-heavy point/range with predictable latency → B-tree
  - Compaction as a latency hazard: p99 spikes, I/O saturation, compaction backlog as an outage signal
  - Column-oriented storage: Parquet/ORC, row groups, dictionary + RLE + delta encoding, min/max zone maps, predicate pushdown, vectorized execution (Level 13)
  - Copy-on-write vs write-ahead-log engines; fractal trees; Bw-trees
  - Disaggregated storage (Aurora, Neon, Socrates): "the log is the database", separating compute from storage
- **Hands-on:** run the same write-heavy benchmark against MySQL/InnoDB and RocksDB/Cassandra. Graph write throughput, read p99, and disk usage over an hour of sustained load.

### 3.5 Replication

- **Topics**
  - Single-leader, multi-leader, leaderless replication
  - Synchronous vs asynchronous vs semi-synchronous replication
  - Replication mechanisms: statement-based, WAL/physical, logical/row-based, trigger-based
  - Read replicas and read scaling; replica lag
- **Subtopics**
  - Failover: automatic vs manual, promotion, fencing, STONITH; **split brain** and how to avoid it
  - Replication lag consequences: read-your-writes violations, monotonic-read violations, stale reads after redirect
  - Consistency fixes on top of async replication: sticky reads to leader, read-after-write routing, LSN/GTID-based read tokens, bounded staleness
  - Catch-up recovery, snapshot + log shipping to bootstrap a new replica
  - Cascading replication, delayed replicas as a "human error" backup
- **Advanced**
  - Multi-leader: use cases (multi-DC writes, offline clients, collaborative editing), write conflicts, conflict detection & resolution (LWW, higher-timestamp, application merge, CRDTs)
  - Leaderless (Dynamo-style): sniff quorum math `R + W > N`, sloppy quorums, hinted handoff, read repair, anti-entropy with Merkle trees
  - Chain replication and its throughput/consistency properties
  - Group replication / consensus-backed replication (Raft groups per shard) — Level 4
  - Failure detection tuning: heartbeats vs phi-accrual; the trade-off between fast failover and false positives
- **Hands-on:** build a Postgres primary + 2 replicas. Measure lag under write load. Cause a failover. Then engineer a read-your-writes guarantee three ways and measure the latency cost.

### 3.6 Partitioning & sharding

- **Topics**
  - Vertical partitioning (split columns/tables) vs horizontal partitioning (split rows)
  - Sharding strategies: range-based, hash-based, directory/lookup-based, geo/entity-based, composite
  - Shard key selection — the most consequential and most irreversible decision in a data design
  - Rebalancing: fixed-partition counts, consistent hashing with virtual nodes, dynamic splits/merges
- **Subtopics**
  - Hot spots, celebrity/hot-key problem, and mitigations (salting, key splitting, dedicated shards, caching)
  - Cross-shard queries, scatter-gather, fan-out latency, and cross-shard joins (avoid)
  - Cross-shard transactions: avoid, or use 2PC / saga (Level 7)
  - Secondary indexes on partitioned data: **local (document-partitioned)** vs **global (term-partitioned)** — read vs write cost inversion
  - Routing tiers: client-side routing, proxy (Vitess, ProxySQL, Citus coordinator), or DB-native
  - Resharding without downtime: dual-write, backfill, verify, cutover, backout plan
- **Advanced**
  - Entity-group / co-location design so that 95% of transactions stay single-shard
  - Tenant-aware sharding, per-tenant isolation, noisy-neighbour throttling
  - Global secondary index consistency (async index shards, index repair jobs)
  - Table partitioning in a single DB (declarative partitioning, partition pruning, partition-wise joins, detach-to-archive)
  - Automatic sharding systems: Vitess, Citus, CockroachDB ranges, Spanner splits, DynamoDB partitions and adaptive capacity
- **Hands-on:** shard a table across 4 databases with hash routing, implement scatter-gather for one query, then resplit to 8 shards with zero downtime.

### 3.7 NoSQL families & when each is right

- **Topics**
  - **Key-value** (Redis, DynamoDB, Memcached, Riak): O(1) access by key, no query flexibility
  - **Document** (MongoDB, Couchbase, Firestore): nested documents, flexible schema, per-document atomicity
  - **Wide-column** (Cassandra, ScyllaDB, HBase, Bigtable): partition key + clustering key, query-first modelling
  - **Graph** (Neo4j, JanusGraph, Neptune): traversals, variable-depth relationships
  - **Time-series** (InfluxDB, TimescaleDB, Prometheus, ClickHouse): append-only, time-partitioned, downsampled
  - **Search** (Elasticsearch/OpenSearch): inverted index, relevance ranking, aggregations
  - **Columnar/OLAP** (ClickHouse, BigQuery, Snowflake, Redshift, Druid, Pinot): scan-heavy analytics
  - **Vector** (pgvector, Milvus, Qdrant, Pinecone): approximate nearest neighbour
  - **Ledger/immutable** (QLDB-style), **object** (S3 as a DB), **embedded** (SQLite, RocksDB, DuckDB)
- **Subtopics**
  - Cassandra data modelling: model per query, denormalize aggressively, partition-size limits (~100 MB / 100k rows), clustering order, no joins, tunable consistency (ONE/QUORUM/ALL/LOCAL_QUORUM)
  - DynamoDB modelling: single-table design, PK/SK, GSI/LSI, sparse indexes, hot partitions, RCU/WCU and on-demand mode, adaptive capacity, DynamoDB Streams, transactions
  - MongoDB: embedding vs referencing, 16 MB document limit, write concern (`w`, `j`, `wtimeout`) and read concern/read preference, aggregation pipeline, change streams
  - Elasticsearch: shards/replicas, refresh interval, near-real-time semantics, mapping explosion, deep pagination (`search_after`), it is **not** a system of record
  - Redis: persistence (RDB/AOF), eviction policies, Cluster mode + hash slots, keyspace notifications, Lua/functions for atomic multi-key ops, `SETNX`-based locks and their unsafety
- **Advanced**
  - Query-first vs entity-first modelling; the cost of getting access patterns wrong in a query-first store
  - HTAP and the pull toward a lakehouse; CDC-based fan-out from OLTP to OLAP/search/cache (Level 13)
  - NewSQL / distributed SQL: Spanner, CockroachDB, TiDB, YugabyteDB — how they get ACID at scale (Raft per range + MVCC + TrueTime/HLC)
  - Serverless/autoscaling databases; separation of storage and compute; branching databases
  - Multi-model databases and the honest cost of "one DB for everything"
- **Decision cheat sheet**

  | Access pattern | Pick |
  |---|---|
  | Transactions, joins, evolving queries, correctness-critical | Relational (Postgres first) |
  | Known key, huge scale, predictable single-digit-ms | Key-value (DynamoDB/Redis) |
  | Massive write volume, time-ordered, per-partition queries | Wide-column (Cassandra/Scylla) |
  | Full-text / faceted / relevance-ranked search | Search engine (as a derived index) |
  | Analytical scans over billions of rows | Columnar (ClickHouse/BigQuery) |
  | Deep multi-hop relationship traversal | Graph |
  | Metrics with retention + downsampling | Time-series |
  | Semantic similarity / RAG retrieval | Vector index |

- **Hands-on:** model the same "activity feed" domain in Postgres, Cassandra, and DynamoDB. Write the same five queries against each. Document what became hard.

### 3.8 Operating a database in production

- **Topics**
  - Connection pooling and pool sizing (`pool ≈ cores × 2 + effective_spindles`, then measure); PgBouncer/RDS Proxy; pool exhaustion as an outage
  - Backups: full/incremental/differential, logical vs physical, PITR via WAL archiving
  - RPO/RTO, restore drills (**an untested backup is not a backup**)
  - Monitoring: QPS, p99 latency, replication lag, cache hit ratio, lock waits, slow query log, table/index bloat, autovacuum activity, connection count, disk headroom
- **Subtopics**
  - Zero-downtime schema migrations: expand/contract, additive changes, backfill in batches, dual-read/dual-write, drop last; avoid blocking `ALTER`s; `lock_timeout` + retry
  - Online DDL tooling: `pt-online-schema-change`, `gh-ost`, `CREATE INDEX CONCURRENTLY`
  - Capacity planning: growth curves, storage headroom (never <25% free), IOPS limits, burst-credit cliffs
  - Query governance: statement timeouts, `idle_in_transaction_session_timeout`, workload isolation via replicas, per-tenant quotas
- **Advanced**
  - Major-version upgrades with logical replication and a switchover window
  - Cross-engine migration playbook (dual-write + CDC + shadow read + diff + cutover + rollback)
  - Data lifecycle: partition-based archival, TTL, legal holds, GDPR erasure across replicas/backups/derived stores
  - Multi-region data strategy: single-writer + global reads, region-partitioned writes, or full multi-master with conflict resolution
- **Hands-on:** perform a real PITR restore to a timestamp 10 minutes in the past. Then run an `ALTER TABLE` on a 50 M-row table with zero downtime.

### ✅ Exit gate for Level 3

1. Explain write skew, show a business bug it causes, and give four fixes with trade-offs.
2. Given a slow query + `EXPLAIN` output, diagnose and fix it — and predict the write cost of your fix.
3. Explain LSM vs B-tree in terms of read/write/space amplification and pick one for a stated workload.
4. Choose a shard key for a multi-tenant SaaS with tenants of wildly different sizes, and describe resharding.
5. Explain local vs global secondary indexes on partitioned data and which you'd use for two given queries.
6. Write a zero-downtime plan to rename a column that is read by 12 services.

---

## Level 4 — Distributed Systems Theory

> **Why this level exists:** This is where you stop guessing. Consistency models, consensus, time, and
> failure modes are the theory that tells you which designs are *impossible* — which is more valuable
> than knowing which are popular.

**Assumes:** Level 3 (replication, transactions, MVCC), Level 1 (partitions, timeouts), Level 0 (concurrency).

### 4.1 What makes distributed systems hard

- **Topics**
  - The eight fallacies of distributed computing (network is reliable, latency is zero, bandwidth is infinite, network is secure, topology doesn't change, one administrator, transport cost is zero, network is homogeneous)
  - Partial failure — the defining difference from single-machine programming
  - Unreliable networks: drops, delays, duplicates, reordering, partitions, asymmetric partitions
  - Unreliable clocks and unreliable processes (GC pause, VM pause, swap, CPU throttle)
- **Subtopics**
  - The **two generals** and **Byzantine generals** problems
  - Timeouts as the only failure detector — and the impossibility of distinguishing "slow" from "dead"
  - Gray failure: a node that is up, passes health checks, and is still broken
  - Process pauses: a 60-second GC pause makes a leader believe it's still the leader
- **Advanced**
  - **FLP impossibility**: no deterministic consensus in an asynchronous system with even one crash fault — and why real systems escape it (randomization, partial synchrony, failure detectors)
  - System models: synchronous, partially synchronous, asynchronous; crash-stop, crash-recovery, Byzantine
  - Safety vs liveness properties; you can usually keep safety always, liveness only eventually
  - Fencing tokens: the only correct way to stop a zombie leader from corrupting state
- **Hands-on:** write a small distributed counter with 3 nodes, then use `iptables`/`tc` to create a partition and a one-way partition. Observe how each breaks.

### 4.2 CAP, PACELC & consistency models

- **Topics**
  - **CAP**: during a network **P**artition you must choose **C**onsistency or **A**vailability. It says nothing about the no-partition case.
  - Common CAP misunderstandings ("we chose AP" as an excuse for having no invariants)
  - **PACELC**: if Partition → (A or C); **Else** → (Latency or Consistency). The Else branch is where you live 99.99% of the time.
  - Classification: Spanner = PC/EC, DynamoDB = PA/EL (tunable), Cassandra = PA/EL, MySQL semi-sync = PC/EC
- **Consistency model hierarchy (strongest → weakest)**
  - **Strict serializability / external consistency** (Spanner) — serializable + real-time order
  - **Linearizability** — single-register atomic; every read sees the latest completed write
  - **Sequential consistency** — a single global order, not necessarily real-time
  - **Serializability** — transactions equivalent to *some* serial order (no real-time guarantee)
  - **Snapshot isolation / repeatable read** — consistent snapshot, allows write skew
  - **Causal consistency (+ session guarantees)** — causally related ops ordered; concurrent ops may diverge
  - **Session guarantees**: read-your-writes, monotonic reads, monotonic writes, writes-follow-reads
  - **Eventual consistency** — converges if writes stop; no bound on when
- **Subtopics**
  - Composing weak consistency safely: which invariants survive under eventual consistency and which don't
  - Bounded staleness and its practical value (dashboards, feeds, analytics)
  - Read-your-writes implementation patterns (Level 3.5) and why they're a client-side concern
  - Consistency for caches: your cache makes your system eventually consistent whether or not you said so
- **Advanced**
  - Why linearizability costs a round trip minimum, and cross-region linearizable writes cost RTT
  - Convergence vs consistency; CALM theorem (monotonic programs need no coordination)
  - Invariant confluence: which application invariants can be maintained without coordination
  - Consistency as a *per-operation* choice, not a per-database one (tunable consistency)
- **Hands-on:** write the same "follower count" feature under linearizable, causal, and eventual guarantees. Enumerate what a user can observe in each.

### 4.3 Time, order & clocks

- **Topics**
  - Physical clocks: NTP, clock drift, clock skew, leap seconds, monotonic vs wall clock
  - Why `now()` is dangerous for ordering, timeouts, and expiry across machines
  - Logical clocks: **Lamport timestamps** (total order, loses concurrency info)
  - **Vector clocks** (detect concurrency and causality; size grows with participants)
  - Version vectors, dotted version vectors
- **Subtopics**
  - Happens-before relation; concurrent events; causal history
  - Last-write-wins and the silent data loss it causes
  - Timestamp-based conflict resolution and clock-skew-induced anomalies
- **Advanced**
  - **Hybrid Logical Clocks (HLC)** — physical + logical, monotonic, close to wall clock; used by CockroachDB/YugabyteDB
  - **TrueTime** (Spanner) — GPS + atomic clocks give a bounded uncertainty interval ε; commit-wait of ε yields external consistency
  - Clock uncertainty as a first-class design input; why bounded uncertainty buys you strong consistency
  - Interval tree clocks; causal metadata compaction
  - Idempotency and ordering tokens: sequence numbers, epochs, fencing tokens, LSNs
- **Hands-on:** implement Lamport and vector clocks over a 3-node message passing sim. Produce a concurrent-write conflict and detect it with vector clocks but not with Lamport.

### 4.4 Consensus & coordination

- **Topics**
  - The consensus problem: agreement, validity, termination, integrity
  - Quorums: majority quorum, `R + W > N`, why odd cluster sizes (3, 5, 7)
  - **Paxos** (single-decree, Multi-Paxos), **Raft** (leader election, log replication, safety), **Zab** (ZooKeeper), **Viewstamped Replication**
  - Leader election, terms/epochs, log matching, commit index, snapshotting
- **Subtopics**
  - Raft in detail: RequestVote, AppendEntries, election timeouts + randomization, log truncation, membership changes (joint consensus), read-index/lease reads
  - Why a leader lease read is cheaper than a full quorum read, and what makes it safe
  - Replicated state machines as the universal pattern (log → deterministic apply)
  - Coordination services: ZooKeeper, etcd, Consul — leader election, locks, config, service registry, watches, ephemeral nodes/leases
  - Distributed locks: TTL leases + fencing tokens; why Redis `SETNX` and even Redlock are unsafe for correctness-critical work
- **Advanced**
  - Multi-Raft: one Raft group per shard/range (CockroachDB, TiKV, Kafka KRaft) — how you scale past one consensus group
  - Flexible Paxos, EPaxos (leaderless, low-latency in WAN), Fast Paxos
  - Byzantine fault tolerance: PBFT, HotStuff, quorum size `3f+1`; blockchain consensus (PoW/PoS) as BFT under open membership
  - Witness/tiebreaker replicas across 3 AZs to get quorum with 2 data copies
  - Cost of coordination: every consensus write costs ≥1 RTT to a majority; designs that avoid coordination win at scale
- **Hands-on:** implement Raft leader election + log replication for a 3-node key-value store (or run a Raft library and kill leaders in a loop). Verify linearizability with a checker like Jepsen/Elle or Porcupine.

### 4.5 Distributed transactions

- **Topics**
  - Atomic commit across nodes; **2-phase commit** (prepare/commit), coordinator, participants
  - 2PC failure modes: coordinator crash → participants blocked holding locks (the blocking problem)
  - 3PC and why it isn't a real fix under network partitions
  - XA transactions and their practical unpopularity
- **Subtopics**
  - Percolator-style distributed transactions (client-driven 2PC over a KV store + timestamp oracle) — TiDB
  - Deterministic databases (Calvin): order first, then execute — removes distributed locking
  - Spanner transactions: 2PC over Paxos groups + TrueTime commit-wait
- **Advanced**
  - **Saga pattern**: a sequence of local transactions with compensating actions; orchestration vs choreography; semantic locks, commutative updates, pivot transactions (deep-dived in Level 7)
  - **Transactional outbox + CDC** as the practical replacement for distributed transactions
  - Try-Confirm/Cancel (TCC) and reservation patterns
  - Idempotency + at-least-once + dedupe store = "effectively once"
  - When to genuinely pay for distributed transactions vs redesign boundaries so they aren't needed
- **Hands-on:** implement a money transfer across two services three ways — 2PC, orchestrated saga, outbox+events — and enumerate the failure windows of each.

### 4.6 Replication & convergence without coordination

- **Topics**
  - Quorum reads/writes, sloppy quorums, hinted handoff, read repair, anti-entropy
  - Conflict resolution: LWW, multi-value (siblings), application merge
  - **CRDTs**: state-based (CvRDT) vs operation-based (CmRDT)
- **Subtopics**
  - CRDT catalogue: G-Counter, PN-Counter, G-Set, 2P-Set, OR-Set, LWW-Register, MV-Register, RGA/Logoot for sequences
  - CRDT applications: collaborative editing, offline-first mobile, shopping carts, presence, counters
  - Operational Transformation (OT) as the alternative (Google Docs lineage) and why CRDTs won for P2P
- **Advanced**
  - Tombstone growth and garbage collection in CRDTs; causal stability
  - δ-CRDTs to reduce sync payloads; Yjs/Automerge as production implementations
  - Local-first architecture: sync engines, per-device logs, and the resulting UX guarantees
  - Anti-entropy at scale: Merkle trees, gossip protocols (SWIM, HyParView), epidemic broadcast
- **Hands-on:** build a collaborative counter and a collaborative text buffer with a CRDT library. Take one client offline for 5 minutes, edit both sides, reconnect, and verify convergence.

### 4.7 Group membership, gossip & failure detection

- **Topics**
  - Heartbeats, timeouts, suspicion; membership lists; view changes
  - Gossip/epidemic protocols; SWIM; scalability of O(log n) dissemination
  - Phi-accrual failure detectors (adaptive suspicion levels)
- **Subtopics**
  - Split-brain prevention: quorum, fencing, lease expiry, generation numbers
  - Leader leases and clock-bound safety
  - Cluster metadata propagation and convergence time as an operational metric
- **Advanced**
  - Metastable failures and retry-storm-induced permanent brownouts
  - Correlated failures via shared dependencies (config push, DNS, control plane, TLS cert expiry)
  - Membership churn amplification in large clusters; why big clusters are less stable than several small ones
- **Hands-on:** run a 5-node gossip cluster, partition 2 nodes, and observe convergence and false-positive failure detection as you tune timeouts.

### ✅ Exit gate for Level 4

1. State CAP precisely, then explain PACELC and classify three real databases.
2. Order the consistency models and give one user-visible anomaly permitted by each.
3. Explain Raft leader election and log replication well enough to draw it, including a fencing-token scenario.
4. Explain why a distributed lock without fencing is unsafe, with a concrete corruption sequence.
5. Choose between 2PC, saga, and outbox for a stated cross-service invariant and defend it.
6. Explain how Spanner gets external consistency and what physical infrastructure that requires.

---

## Level 5 — Scalability & Performance Engineering

> **Why this level exists:** Scaling is not "add servers". It is finding the one resource that saturates
> first, and knowing what the system does when it saturates. Queueing theory and tail latency are the
> two ideas that separate real capacity planning from guessing.

**Assumes:** Levels 0–4.

### 5.1 Scaling axes & statelessness

- **Topics**
  - Vertical scaling (simplest, hard ceiling, single failure domain) vs horizontal (complexity, near-unbounded)
  - Stateless services as the enabler of horizontal scale; externalizing state
  - Scale cube (AKF): X = clone/replicate, Y = functional decomposition, Z = data partition/shard
- **Subtopics**
  - Session state: cookies, signed tokens, sticky sessions, external session store — with trade-offs
  - Read scaling (replicas, caches, CQRS read models) vs write scaling (sharding, batching, queues, log-structured writes)
  - Shared-nothing architecture
  - Multi-tenancy scaling: pooled vs siloed vs bridge models
- **Advanced**
  - Universal Scalability Law: `C(N) = N / (1 + α(N−1) + βN(N−1))` — contention (α) flattens the curve, **coherency/crosstalk (β) makes it go down**. Adding nodes can reduce throughput.
  - Identifying the scaling limiter: CPU, memory, disk IOPS, network, locks, DB connections, single-threaded hot path, downstream quota, ephemeral ports, file descriptors
  - Why "just add a cache" often converts a throughput problem into a consistency problem

### 5.2 Queueing theory & capacity planning

- **Topics**
  - Arrival rate λ, service rate μ, utilization ρ = λ/μ, servers c
  - **Little's Law**: `L = λ × W` (concurrency = throughput × latency). The single most useful formula in the field.
  - M/M/1 and M/M/c response time; the hockey-stick curve as ρ → 1
  - Why you must target 50–70% utilization, not 95%
- **Subtopics**
  - Queue discipline: FIFO, LIFO, priority, fair queueing; **LIFO sheds better under overload** (serve fresh requests, drop stale)
  - Bounded vs unbounded queues; unbounded queues convert overload into unbounded latency and OOM
  - Batching: amortizes fixed cost, raises latency; the throughput/latency dial
  - Variability (coefficient of variation) as a latency multiplier — variance hurts as much as mean
- **Advanced**
  - Capacity model: `instances = ceil(peak_QPS × p50_service_time / target_utilization / cores_per_instance)` plus failure headroom (N+1 or N+2) plus AZ headroom (survive losing 1 of 3 AZs ⇒ +50%)
  - Headroom for retries: a 3× retry budget means provision for 3× worst-case
  - Load testing properly: open vs closed workload models, ramp to find the knee, sustained soak, spike test, and correcting for coordinated omission
  - Forecasting: growth curves, seasonality, event-driven spikes (sales, launches), lead time for capacity
- **Hands-on:** load test a service; plot throughput vs latency vs utilization; find the knee; verify Little's Law holds with your measured numbers.

### 5.3 Caching (deep)

- **Topics**
  - All cache layers again, now with consistency reasoning: browser, CDN, gateway, in-process L1, distributed L2, DB buffer pool, page cache, materialized views
  - Cache patterns: cache-aside, read-through, write-through, write-behind, refresh-ahead
  - Eviction and admission: LRU, LFU, ARC, **TinyLFU/W-TinyLFU** (admission filter beats pure eviction), S3-FIFO, SIEVE
- **Subtopics**
  - TTL strategy: absolute vs sliding, jittered TTLs to avoid synchronized expiry
  - Explicit invalidation: on-write purge, versioned/namespaced keys (`user:123:v7`), tag-based invalidation, CDC-driven invalidation
  - Negative caching and null-result caching
  - Multi-level cache coherence: L1 in-process staleness bounded by short TTL + pub/sub invalidation broadcast
  - Cache warming and pre-computation; avoiding cold-start cliffs after deploy
- **Advanced**
  - **Stampede/dogpile control**: single-flight/request coalescing, mutex-per-key, probabilistic early recomputation (XFetch), `stale-while-revalidate`, serve-stale-on-error
  - **Hot key**: consistent-hash hot-spot mitigation, key sharding (`key:{0..N}`), client-side L1, dedicated replica set, adaptive replication
  - Cache as a source of outages: cache flush → origin thundering herd → cascading failure; always ask "can the DB survive a 0% hit ratio?"
  - Working-set estimation and cost/benefit: memory cost vs DB cost vs added latency on miss
  - Write-heavy caching and write-behind durability risk
  - Cache stampede-free counters, sorted sets for leaderboards, sliding-window rate limiters in Redis with Lua atomicity
- **Hands-on:** build a cache-aside layer with single-flight + jittered TTL + pub/sub invalidation. Then flush the cache under load and confirm the DB survives.

### 5.4 Tail latency engineering

- **Topics**
  - Percentiles and why p99/p99.9 drive user experience and fan-out cost
  - Sources of tail latency: GC pauses, JIT warmup, lock contention, queueing, compaction, noisy neighbours, CPU throttling, retries, cold caches, DNS, TLS handshakes, connection-pool waits
  - Fan-out amplification: with n parallel dependencies, `P(slow request) ≈ 1 − (1 − p)^n`
- **Subtopics**
  - **Hedged requests**: send a second request after p95 elapses, take the first response (costs ~5% extra traffic)
  - **Tied requests** / cross-server cancellation
  - Request reissue with a budget; speculative execution; micro-partitioning to reduce straggler impact
  - Selective replication of hot data; latency-aware load balancing; outlier ejection
  - Priority lanes: separate interactive traffic from batch traffic at every tier
- **Advanced**
  - GC tuning as an architecture concern: generational vs region-based (G1/ZGC/Shenandoah), pause-time targets, allocation-rate reduction, off-heap storage
  - Thread-per-core designs, run-to-completion, io_uring, kernel bypass for microsecond tails
  - Latency SLO decomposition across a call graph; per-hop budgets; **deadline propagation** so downstream work is abandoned when the client has already given up
  - Cancellation propagation (context/deadline) to avoid doing work nobody will read
- **Hands-on:** instrument a fan-out endpoint calling 10 services, measure p99 amplification, then add hedging and deadline propagation and re-measure.

### 5.5 Concurrency control & throughput at the service tier

- **Topics**
  - Connection pooling everywhere (DB, HTTP, gRPC); pool sizing; pool-wait as a hidden latency source
  - Thread pools vs event loops vs async; bulkheads by dependency
  - Batching and pipelining; coalescing writes; micro-batching for throughput
- **Subtopics**
  - Bounded concurrency per dependency (semaphores) instead of unbounded parallelism
  - Adaptive concurrency limits (Netflix/Vegas-style, gradient-based) — discover the limit instead of guessing
  - Async I/O vs thread blocking; blocking calls inside an event loop as a classic capacity bug
- **Advanced**
  - Backpressure end-to-end: reactive streams, credit-based flow control, queue-depth-based admission
  - Work shedding vs work queueing decision tree
  - Isolation of slow tenants/queries (workload management, per-tenant concurrency caps)

### 5.6 Rate limiting, throttling, quotas & load shedding

- **Topics**
  - Algorithms: fixed window, sliding window log, sliding window counter, **token bucket**, **leaky bucket**, GCRA
  - Where to enforce: edge/CDN, gateway, service, DB
  - Dimensions: per IP, per user, per API key, per tenant, per endpoint, per resource, global
- **Subtopics**
  - Distributed rate limiting: centralized counter (Redis) vs local approximation + periodic sync; accuracy vs latency vs availability
  - Client cooperation: 429 + `Retry-After`, `X-RateLimit-*` headers, SDK-level backoff
  - Quotas vs rate limits vs concurrency limits vs spend limits — four different controls
- **Advanced**
  - **Load shedding**: prioritized shedding by request criticality, cost-based shedding, CoDel-style latency-based admission control (drop when queue delay exceeds target)
  - **Brownout / graceful degradation**: serve cheaper responses (cached, partial, no personalization) rather than failing
  - Fair queueing across tenants; weighted fair share; **shuffle sharding** to limit a noisy tenant's blast radius
  - Retry budgets and per-client retry accounting to prevent retry storms (Level 6)
  - Admission control at the front door as the last line of defence against metastable collapse
- **Hands-on:** implement token bucket and sliding-window in Redis with Lua; then implement CoDel-style shedding and prove the service stays at target p99 while rejecting excess.

### 5.7 Data-tier performance patterns

- **Topics**
  - Read replicas, connection routing, read/write splitting
  - Materialized views, precomputed aggregates, summary tables, counters
  - Denormalization, fan-out-on-write vs fan-out-on-read (the feed problem)
  - Bulk operations: batch inserts, `COPY`, upserts, bulk deletes in chunks
- **Subtopics**
  - Pagination at scale: keyset/cursor pagination, `search_after`, avoiding deep OFFSET
  - Async write paths: queue + worker + idempotent apply
  - Approximate answers: sampling, sketches (HLL/CMS), pre-aggregation, "about 12k likes"
  - Archival/tiering of cold data to keep hot indexes small
- **Advanced**
  - Hybrid fan-out for the celebrity problem (push for normal users, pull for celebrities)
  - Write-optimized paths: append-only tables + periodic compaction into read models
  - Precompute vs on-demand decision rule: `precompute if read:write ratio × compute cost > storage cost`
  - Query result caching vs object caching vs fragment caching
- **Hands-on:** build a feed two ways (fan-out-on-write and fan-out-on-read), then a hybrid. Measure write amplification and read latency for a user with 10 M followers.

### ✅ Exit gate for Level 5

1. Apply Little's Law to size a thread pool and a fleet from measured latency and target QPS.
2. Explain why 95% utilization is a design error, using the ρ→1 latency curve.
3. Name six sources of tail latency in a service you've built and the mitigation for each.
4. Design a distributed rate limiter with stated accuracy/availability trade-offs.
5. Explain load shedding vs queueing and when each is correct.
6. Solve the celebrity fan-out problem and justify the hybrid threshold.

---

## Level 6 — Reliability, Resilience & Operability

> **Why this level exists:** At scale, failure is continuous. Reliability is not the absence of failure but
> the containment of it. This level is what makes an on-call rotation survivable.

**Assumes:** Levels 0–5.

### 6.1 Reliability vocabulary & SLOs

- **Topics**
  - Availability, reliability, durability, maintainability — distinct properties
  - **SLI / SLO / SLA**: indicator (measurement), objective (internal target), agreement (external contract with penalties)
  - Error budgets: `budget = 1 − SLO`; spending it on releases; freezing when exhausted
  - MTBF, MTTR, MTTD, MTTA, MTTF; change failure rate
- **Subtopics**
  - Choosing good SLIs: availability (success ratio), latency (threshold-based, e.g. "99% of requests < 300 ms"), correctness, freshness, durability, coverage
  - SLI measurement location: client-side vs LB vs service (they disagree — client-side is the truth)
  - Multi-window multi-burn-rate alerting (fast burn = page, slow burn = ticket)
  - DORA metrics: deployment frequency, lead time, change failure rate, MTTR
- **Advanced**
  - Composing SLOs across a dependency graph; **dependency availability must exceed yours**
  - Setting SLOs from user journeys, not from components
  - Error budget policy as an organizational contract between product and reliability
  - Why 100% is the wrong target (cost curve is exponential; the internet is ~99.9% anyway)

### 6.2 Redundancy & failure domains

- **Topics**
  - Redundancy: N+1, N+2, 2N; active-active vs active-passive; hot/warm/cold standby
  - Failure domains: process, host, rack, AZ, region, provider, control plane, human/config
  - Blast radius and containment
- **Subtopics**
  - Multi-AZ as the default; capacity headroom to survive AZ loss (3 AZs at 66% ⇒ survivable)
  - Stateless redundancy is easy; stateful redundancy needs Level 3–4 (quorum, replication, failover)
  - Failover mechanics: health checks, promotion, fencing, DNS/LB updates, client reconnection storms
- **Advanced**
  - **Cell-based architecture**: partition users into independent cells with their own full stack; a bad deploy or bad tenant hits one cell
  - **Shuffle sharding**: assign each tenant a random subset of nodes so overlaps are rare — dramatic blast-radius reduction with little extra capacity
  - Static stability: continue operating correctly when the control plane is down (pre-provisioned capacity, cached config, no dependency on scaling during failure)
  - Avoiding correlated failure: independent config pushes, staggered cert expiry, per-cell dependencies, no global fleet-wide anything
  - Regional isolation: no cross-region synchronous dependency in the critical path

### 6.3 Resilience patterns

- **Topics**
  - **Timeouts** at every boundary (and finite by default)
  - **Retries** with exponential backoff + **full jitter**; retry budgets; retry only idempotent work
  - **Circuit breaker**: closed → open → half-open; failure-rate and slow-call thresholds
  - **Bulkhead**: separate pools per dependency so one slow dependency can't consume all threads
  - **Fallback / graceful degradation**: cached, default, partial, or reduced-fidelity responses
  - **Idempotency**: keys, dedupe stores, natural idempotence, at-least-once + idempotent apply
- **Subtopics**
  - Timeout budget arithmetic: a caller's timeout must exceed the callee's total retry span, or retries are wasted work
  - Deadline propagation and cancellation to prevent orphaned work
  - Retry storms and **exponential backoff with jitter** as the fix; `min(cap, base × 2^attempt) × random()`
  - Health checks: shallow (am I up?) vs deep (are my dependencies up?) — deep checks cause correlated failure; separate readiness from liveness
  - Load shedding & prioritization (Level 5.6) as a resilience pattern
  - Failing fast vs failing safe vs failing open (auth systems must fail *closed*; caches may fail *open*)
- **Advanced**
  - **Metastable failure**: a system that will not recover even after the trigger is gone (retry amplification, cache-miss death spiral, queue backlog); fixes: shed load, drop queues, cold-start with reduced traffic, circuit break inward
  - Cascading failure paths and how to interrupt them at each hop
  - **Constant work** pattern: always do the same amount of work regardless of input (push full config every N seconds instead of deltas on change) so failure modes don't correlate with events
  - Poison-pill isolation; quarantine queues; per-message failure caps
  - Jittering everything periodic (cron, refresh, TTL, reconnect) to break synchronization
  - Client-side resilience: SDK backoff, connection reuse, endpoint failover, adaptive concurrency
- **Hands-on:** wrap a flaky dependency with timeout + jittered retry + circuit breaker + bulkhead + fallback. Prove with a fault-injection test that a 100%-down dependency does not exhaust your threads or violate your SLO for the unaffected paths.

### 6.4 Disaster recovery & business continuity

- **Topics**
  - **RPO** (data loss tolerance) and **RTO** (downtime tolerance) as the two numbers that define DR
  - DR strategies by cost/RTO: backup & restore → pilot light → warm standby → hot/active-active
  - Backup types, offsite/immutable backups, air-gapped copies, 3-2-1 rule
- **Subtopics**
  - PITR, log shipping, cross-region replication, snapshot schedules
  - Restore drills and game days; documented, tested runbooks
  - Data corruption recovery (harder than outage recovery): detection, quarantine, replay from log, backfill
  - Ransomware/insider-threat resilience: immutability, versioning, MFA-delete, separate credentials
- **Advanced**
  - Multi-region topologies: active-passive, active-active with regional partitioning, single-writer with global reads, full multi-master with conflict resolution
  - Regional failover mechanics: data promotion, DNS/global-LB shift, cache warm-up, client reconnect surge, split-brain prevention
  - Backup dependency loops (can you restore if the identity provider or KMS is also down?)
  - Failback and reconciliation after the primary returns; divergence repair
  - Compliance-driven retention, legal holds, and GDPR erasure vs immutable backups (a genuine tension — resolve with crypto-shredding)
- **Hands-on:** write and execute a DR plan for a small app: measure real RPO/RTO by destroying the primary region and recovering. Record the gap between planned and actual.

### 6.5 Chaos engineering & verification

- **Topics**
  - Hypothesis-driven experiments in production (or production-like), with a blast-radius limit and an abort condition
  - Fault injection: latency, errors, resource exhaustion, instance kill, AZ blackhole, dependency failure, clock skew, packet loss, DNS failure
  - Game days, disaster simulations, on-call drills
- **Subtopics**
  - Steady-state metric definition before the experiment
  - Progressive scope: dev → staging → 1% prod → one cell → region
  - Failure-mode catalogue derived from FMEA on your own architecture
- **Advanced**
  - Continuous verification in CI/CD; chaos as a gate
  - Jepsen/Elle-style consistency verification for stateful systems
  - Deterministic simulation testing (FoundationDB/TigerBeetle style): a single-threaded deterministic scheduler that replays exact failure interleavings — the strongest correctness tool available for distributed code
  - Formal methods: TLA+/PlusCal, Alloy, P — model the protocol and let a model checker find the interleaving you'd never test
- **Hands-on:** run one experiment per failure domain against your own service. Document one surprise per experiment. Then write a TLA+ spec for a small protocol (e.g. a lease-based lock) and let it find a bug.

### 6.6 Incident response & operational excellence

- **Topics**
  - On-call: rotations, escalation, paging policy, alert quality, actionable alerts only
  - Incident command: IC, comms lead, scribe, subject-matter experts; severity levels
  - Detect → triage → mitigate → resolve → learn; **mitigate before diagnose**
  - Runbooks, playbooks, dashboards per service; the "first 5 minutes" checklist
- **Subtopics**
  - Blameless postmortems: timeline, contributing factors, action items with owners and dates
  - Alert fatigue and its cure (delete alerts that never led to action; SLO-based alerting)
  - Change management: most incidents are caused by change — deploys, config, feature flags, data migrations, cert rotation
  - Kill switches and rollback as first-class features, not afterthoughts
- **Advanced**
  - Safety science: human error as a symptom of system design; Swiss-cheese model; "second stories"
  - Operational readiness reviews / production readiness checklists as an architectural gate
  - Toil measurement and elimination; the automation ladder
  - Error-budget-driven prioritization and its use in negotiating roadmap with product
- **Hands-on:** write a production-readiness checklist for a service you own, then run a postmortem on any recent real incident using a proper template.

### ✅ Exit gate for Level 6

1. Define SLI/SLO/SLA for a real service, including latency threshold and burn-rate alerts.
2. Compute whether a 6-dependency chain can meet a 99.95% SLO, and say what to change if not.
3. Explain metastable failure with a concrete example and three interventions.
4. Explain deep vs shallow health checks and why deep checks cause correlated outages.
5. Design a DR plan with a stated RPO/RTO and defend the cost.
6. Explain shuffle sharding and quantify its blast-radius reduction.

---

## Level 7 — Messaging, Streaming & Event-Driven Architecture

> **Why this level exists:** Once a system has more than one service, the interesting questions are all
> about *when* and *how* information moves. Async messaging is how real systems decouple, absorb load,
> and stay available — and it is where the subtlest bugs live.

**Assumes:** Level 3 (transactions, outbox), Level 4 (ordering, idempotency, consensus), Levels 5–6.

### 7.1 Messaging fundamentals

- **Topics**
  - Point-to-point queues vs publish/subscribe vs log-based streams
  - Producer, consumer, broker, topic, partition, queue, subscription, consumer group
  - Push vs pull consumption; polling, long polling, prefetch
  - Delivery semantics: at-most-once, at-least-once, exactly-once (and why the last is conditional)
- **Subtopics**
  - Acknowledgement models: auto-ack, manual ack, negative ack, visibility timeout, redelivery
  - Message ordering: global vs per-partition vs per-key vs none
  - Dead-letter queues, redrive, poison messages, max-receive-count
  - Delay queues, scheduled messages, TTL, message priority
  - Fan-out (one message → N consumers) vs work distribution (one message → 1 consumer)
- **Advanced**
  - **Exactly-once, honestly**: at-least-once delivery + idempotent consumers + a dedupe store (or transactional read-process-write within one system). End-to-end exactly-once across heterogeneous systems is not achievable without idempotency.
  - Ordering vs parallelism trade-off: ordering requires per-key serialization, which limits consumer concurrency
  - Message schema evolution and compatibility (forward/backward/full) via a schema registry
  - Backpressure in messaging: bounded queues, consumer lag as the control signal, producer throttling
- **Hands-on:** build a producer/consumer pair with at-least-once delivery, deliberately duplicate messages, and make the consumer idempotent with a dedupe table. Prove correctness under duplicate + reorder + replay.

### 7.2 Broker technologies & their models

- **Topics**
  - **Kafka** (partitioned, replicated, append-only log; retention-based; consumer offsets)
  - **RabbitMQ** (AMQP exchanges/bindings/queues; routing keys; flexible topologies)
  - **SQS/SNS** (managed queue + fan-out; FIFO vs standard)
  - **Pulsar** (segmented storage via BookKeeper; multi-tenant; tiered offload)
  - **NATS / NATS JetStream** (lightweight, request-reply, at-most/at-least once)
  - **Redis Streams** (consumer groups, low-latency, limited durability)
  - **Kinesis / EventBridge / Pub/Sub / Service Bus** (cloud-native equivalents)
- **Kafka internals (learn these properly)**
  - Topics → partitions → segments; offsets; log retention (time/size) and log compaction (keyed latest-value)
  - Replication: leader + followers, ISR (in-sync replicas), `acks=0/1/all`, `min.insync.replicas`, unclean leader election and the data loss it permits
  - Producer: batching (`linger.ms`, `batch.size`), compression, partitioner, `enable.idempotence`, transactions (`transactional.id`, EOS)
  - Consumer: consumer groups, rebalancing (eager vs cooperative sticky), `max.poll.interval.ms`, offset commit strategies, static membership
  - Ordering: guaranteed only within a partition; keying determines ordering and skew
  - Consumer lag as the primary health metric; partition count as the concurrency ceiling
  - KRaft (Raft-based metadata) replacing ZooKeeper; tiered storage
- **Subtopics**
  - RabbitMQ topologies: direct/topic/fanout/headers exchanges, alternate exchanges, quorum queues vs classic mirrored, lazy queues, publisher confirms
  - SQS specifics: visibility timeout, long polling, FIFO with message group IDs, dedupe window, 256 KB limit + claim-check pattern
  - Choosing: **queue** for work distribution with per-message ack; **log** for replay, multiple independent consumers, and event sourcing; **broker with rich routing** for complex topologies
- **Advanced**
  - Partition count planning and the pain of increasing it (key→partition mapping changes)
  - Multi-datacenter replication: MirrorMaker2, Cluster Linking, geo-replication and offset translation
  - Kafka transactional read-process-write for exactly-once *within* Kafka; why it doesn't extend to your DB or third-party API
  - Broker sizing: throughput per partition, page-cache reliance, zero-copy, disk layout, retention cost
- **Hands-on:** run a 3-broker Kafka cluster; kill a leader mid-produce with `acks=1` vs `acks=all` and quantify data loss; then trigger a rebalance under load and measure the stall.

### 7.3 Event-driven architecture styles

- **Topics**
  - Event notification (thin event, consumer re-queries)
  - Event-carried state transfer (fat event, consumer keeps a local replica)
  - Event sourcing (the log of events *is* the state)
  - CQRS (separate write model from read models)
- **Subtopics**
  - Commands vs events vs queries; naming (past tense for events, imperative for commands)
  - Event schema design: event type, version, ID, timestamp, causation ID, correlation ID, tenant, payload; thin vs fat payload trade-off
  - Choreography vs orchestration (emergent flow vs explicit coordinator) — debuggability vs autonomy
  - Domain events vs integration events; internal event contracts vs published contracts
- **Advanced**
  - **Event sourcing in depth**: append-only event store, aggregates, replay to rebuild state, snapshots for fast load, projections, versioning/upcasting old events, GDPR erasure in an immutable log (crypto-shredding), event-store storage growth
  - **CQRS in depth**: eventual consistency between write and read models, read-model rebuild, multiple specialized read models (search, cache, analytics), handling "read your own write" in a CQRS UI
  - When *not* to use event sourcing (most CRUD apps): high conceptual cost, hard queries, hard debugging, hard schema change
  - Event-driven pitfalls: hidden coupling via payload shape, event storms, unbounded fan-out, no global view, distributed debugging
- **Hands-on:** implement an event-sourced aggregate (bank account or booking) with snapshots and two projections; then evolve the event schema and upcast old events.

### 7.4 Cross-service data consistency patterns

- **Topics**
  - **Transactional outbox** (write state + event in one local transaction; a relay publishes) — the standard solution to dual-write
  - **Inbox / dedupe table** on the consumer for idempotency
  - **Change Data Capture (CDC)** from the DB log (Debezium, logical decoding) as an alternative to explicit events
  - **Saga** — long-running business transaction as local transactions + compensations
- **Subtopics**
  - The dual-write problem and why "save then publish" loses events on crash
  - Saga orchestration (a central coordinator/state machine) vs choreography (each service reacts)
  - Compensating actions design: semantically reversible, idempotent, and durable; the pivot transaction
  - Semantic locks, reservation/TCC patterns, and countermeasures for lack of isolation in sagas
  - Correlation and causation IDs for tracing a saga end-to-end
- **Advanced**
  - Workflow engines: Temporal/Cadence (durable execution, deterministic replay, timers, signals), Step Functions, Camunda; when a workflow engine beats hand-rolled sagas (almost always, past 3 steps)
  - Idempotency at API boundaries: idempotency keys, request fingerprinting, response replay, key TTL, concurrent-same-key handling (409/conflict semantics)
  - Ordering guarantees across services; per-entity partitioning to serialize work per aggregate
  - Backfills and replay: replaying a topic into a new consumer without double-applying side effects (side-effect gating, replay flags, separate replay pipelines)
  - Data mesh / event contracts as products; consumer-driven contracts for events
- **Hands-on:** implement outbox + relay + idempotent consumer for an order/payment flow. Then implement the same flow in Temporal and compare the amount of code you had to write for retries, timeouts, and compensation.

### 7.5 Stream processing

- **Topics**
  - Stateless transforms (map/filter) vs stateful (aggregations, joins, windows)
  - Windowing: tumbling, hopping/sliding, session, global
  - **Event time vs processing time vs ingestion time**; watermarks; allowed lateness; late-arrival handling
  - Frameworks: Kafka Streams, Flink, Spark Structured Streaming, Beam, Materialize/RisingWave/ksqlDB
- **Subtopics**
  - State stores, changelog topics, checkpointing, savepoints, recovery
  - Stream-table duality; KTable vs KStream; materialized views over streams
  - Joins: stream-stream (windowed), stream-table (enrichment), table-table
  - Exactly-once processing via checkpointed state + transactional sinks
- **Advanced**
  - Backpressure and rescaling in stream jobs; keyed state partitioning; hot-key skew
  - Out-of-order handling strategies and correctness/latency trade-off of watermark lateness
  - Lambda vs **Kappa** architecture (batch+stream vs stream-only with replay) — see Level 13
  - Incremental view maintenance and differential dataflow (Materialize) as "SQL over streams"
  - Idempotent sinks and end-to-end EOS across Kafka → Flink → OLAP store
- **Hands-on:** compute a 5-minute windowed count with event-time semantics, inject out-of-order and very late events, and observe the difference between watermark strategies.

### ✅ Exit gate for Level 7

1. Explain the dual-write problem and solve it with the outbox pattern, including failure windows.
2. Explain "exactly-once" honestly, and implement idempotent consumption.
3. Choose queue vs log vs rich-routing broker for three workloads and defend each.
4. Design a saga for a 4-step booking flow with compensations, and say what isolation you lose.
5. Explain event time vs processing time and why watermarks exist.
6. Argue *against* event sourcing for a given CRUD system.

---

## Level 8 — Architecture Styles, Boundaries & Domain-Driven Design

> **Why this level exists:** Technology choices are recoverable. Boundary choices are not. Where you draw
> service and data boundaries determines your team's velocity for years. This is the level where system
> design becomes an organizational discipline.

**Assumes:** Levels 3, 4, 7 (you cannot reason about service boundaries without understanding data consistency).

### 8.1 Architecture styles

- **Topics**
  - **Monolith** (single deployable) — simplest, fastest to start, still correct for most products
  - **Modular monolith** — enforced internal module boundaries, one deployable
  - **Microservices** — independently deployable services around business capabilities
  - **SOA**, ESB-era architecture, and what went wrong with it
  - **Serverless / FaaS**; event-driven serverless composition
  - **Layered / n-tier**, **hexagonal (ports & adapters)**, **onion**, **clean architecture**
  - **Pipeline / pipes-and-filters**, **space-based**, **microkernel/plugin**, **CQRS+ES**, **cell-based**
- **Subtopics**
  - Distributed monolith — the worst outcome: microservice cost with monolith coupling
  - Coupling taxonomy: temporal, data/schema, deployment, semantic, operational; afferent vs efferent
  - Cohesion; the single-responsibility principle applied at service granularity
  - Service granularity heuristics: one team's cognitive load, one deployment cadence, one data owner, one consistency boundary
  - Sync vs async coupling; the "orchestrator that owns nothing" smell
- **Advanced**
  - Fitness functions for architecture (evolutionary architecture): automated tests that fail when a boundary is violated (dependency rules, latency budgets, layer checks)
  - Architecture quantum: the smallest independently deployable unit with high functional cohesion, including its data
  - Migration paths: **strangler fig**, branch-by-abstraction, anti-corruption layer, service extraction with dual-write + CDC + shadow read + cutover
  - Modular monolith → microservices only when a specific, measured pain exists (deploy contention, scaling asymmetry, team autonomy, isolation requirements)
  - Cell-based architecture as the reliability-driven decomposition (contrast with domain-driven decomposition)
- **Decision guidance**

  | Situation | Recommended style |
  |---|---|
  | New product, <15 engineers, unclear domain | Modular monolith |
  | Clear bounded contexts, multiple teams, different scaling profiles | Microservices |
  | Spiky/irregular workloads, glue and integrations | Serverless |
  | Extreme reliability + tenant isolation requirements | Cell-based |
  | Heavy analytical/data transformation | Pipeline + lakehouse |

### 8.2 Domain-Driven Design

- **Topics**
  - Strategic DDD: **ubiquitous language**, **bounded context**, **context map**, **subdomains** (core / supporting / generic)
  - Tactical DDD: entity, value object, aggregate + aggregate root, repository, factory, domain service, domain event, application service
  - Aggregate design rules: small aggregates, one aggregate per transaction, reference other aggregates by ID, eventual consistency between aggregates
- **Subtopics**
  - Context integration patterns: shared kernel, customer/supplier, conformist, **anti-corruption layer**, open host service, published language, separate ways, partnership
  - Event storming and domain storytelling as boundary-discovery techniques
  - Mapping bounded contexts → services → databases → teams (the alignment that makes microservices work)
  - Invariant location: which invariants must be transactional (same aggregate) and which can be eventual (saga)
- **Advanced**
  - Core-domain-first investment: buy/adopt generic subdomains, build only the core
  - Context boundaries as consistency boundaries — the single most useful DDD insight for system design
  - Polyseme handling: the same word meaning different things in different contexts ("customer" in billing vs support)
  - Team topologies alignment: stream-aligned, platform, enabling, complicated-subsystem teams; Conway's law as a design tool ("inverse Conway manoeuvre")
- **Hands-on:** run an event-storming session (even solo) on a domain you know. Produce a context map, then propose a service decomposition and mark which invariants become eventual.

### 8.3 Microservices mechanics

- **Topics**
  - Service template/chassis: config, logging, metrics, tracing, health, auth, retries baked in
  - Service discovery, API gateway, BFF (backend-for-frontend)
  - Database-per-service; no shared database; anti-pattern: shared tables across services
  - Inter-service communication: sync (REST/gRPC) vs async (events)
  - Distributed tracing, correlation IDs (Level 11)
- **Subtopics**
  - Shared libraries vs duplication vs sidecars; versioning shared libraries across many services
  - Data ownership, read models for cross-service data, API composition vs CQRS view
  - Distributed query problem: joining data across services (API composition, materialized view, data virtualization)
  - Sync-call chain depth as an availability multiplier — keep chains ≤2–3 deep
  - Contract testing (Pact), consumer-driven contracts, schema registries
- **Advanced**
  - **Service mesh**: sidecar (Envoy) vs sidecarless/ambient; mTLS, retries, timeouts, circuit breaking, traffic splitting, outlier ejection moved to infrastructure; control plane vs data plane; cost (latency, complexity, another failure domain)
  - Multi-tenancy in microservices: tenant context propagation, per-tenant quotas, tenant-aware routing
  - Platform engineering: internal developer platform, golden paths, self-service infrastructure
  - Governance without central bottleneck: architecture decision records, tech radar, paved roads, guardrails as code
  - Microservice anti-patterns: nano-services, chatty interfaces, shared DB, synchronous chains, entity services, "orchestrator that owns nothing", per-service tech sprawl
- **Hands-on:** extract one service from a monolith using strangler fig + anti-corruption layer + dual-write, with a documented rollback plan at each step.

### ✅ Exit gate for Level 8

1. Given a domain description, produce bounded contexts and defend each boundary in terms of invariants.
2. Explain the coupling types and identify each in a design you've seen.
3. Give three concrete signals that a monolith should be split, and three that it should not.
4. Explain the distributed monolith and name the specific smells that produce it.
5. Plan a full strangler-fig extraction with cutover and rollback.
6. Explain what a service mesh gives you and what it costs.

---

## Level 9 — API & Interface Design

> **Why this level exists:** APIs are the only part of your architecture that other people build on.
> They outlive implementations and are the hardest thing to change.

**Assumes:** Levels 1 (HTTP semantics), 3 (data modelling), 7 (idempotency), 8 (boundaries).

### 9.1 REST & resource design

- **Topics**
  - Resources, URIs, representations; nouns not verbs; collection vs item
  - Correct method/status usage; Richardson maturity model; HATEOAS (and its rarity in practice)
  - Idempotency of PUT/DELETE, non-idempotency of POST/PATCH
  - Filtering, sorting, sparse fieldsets, expansion/embedding
- **Subtopics**
  - **Pagination**: offset/limit (simple, drifts, O(n)), keyset/cursor (stable, scalable), page tokens (opaque, versionable). Prefer opaque cursors.
  - Bulk/batch endpoints; partial success semantics (207-style multi-status, per-item errors)
  - Long-running operations: 202 + operation resource + polling, or webhook/callback
  - **Error model**: stable machine-readable code + human message + field-level details + trace/correlation ID + retryability hint (RFC 9457 `application/problem+json`)
  - Conditional requests (ETag + `If-Match`) for optimistic concurrency
  - **Idempotency keys** for POST: client-generated key, server stores request hash + response, replays identical response, rejects same key with different body
- **Advanced**
  - **Versioning**: URI path (`/v2`), header/media-type, query param; additive evolution as the preferred default; deprecation policy with sunset headers, dual-running, client telemetry to know who's still on v1
  - Backward/forward compatibility rules: never remove or repurpose a field, never tighten validation, never change enum semantics, always tolerate unknown fields
  - API governance: style guide, linting (Spectral), design review, published changelog, SLA per endpoint
  - Multi-tenant API design: tenant in token not URL, per-tenant rate limits, tenant-scoped IDs
  - Field-level permissions and response shaping by scope
- **Hands-on:** design a complete API for a booking system: cursor pagination, ETag concurrency, idempotency keys, RFC 9457 errors, and a versioning/deprecation policy. Lint it with Spectral.

### 9.2 gRPC, GraphQL & alternatives

- **Topics**
  - **gRPC**: Protobuf IDL, code generation, HTTP/2, unary + 3 streaming modes, deadlines, metadata, status codes, interceptors
  - **GraphQL**: schema, queries/mutations/subscriptions, resolvers, single endpoint, client-specified shape
  - **tRPC / OpenAPI-generated clients / JSON-RPC / SOAP** as points on the spectrum
  - Webhooks and callbacks as inverted APIs
- **Subtopics**
  - Protobuf schema evolution rules: never reuse field numbers, `reserved`, optional/singular semantics, wire compatibility
  - gRPC in browsers (grpc-web/Connect), load balancing gRPC (L7-aware or client-side, since H2 multiplexes on one connection)
  - GraphQL problems and solutions: N+1 (DataLoader batching), query cost analysis + depth limits, persisted queries, caching difficulty (no URL cache key), authorization per field, error partiality
  - Federation/schema stitching for multi-team GraphQL; supergraph governance
- **Advanced**
  - Choosing: **gRPC** for internal service-to-service (perf, contracts, streaming); **REST+JSON** for public APIs (ubiquity, caching, debuggability); **GraphQL** for many-client, aggregation-heavy front-ends; **webhooks/events** for integration
  - BFF pattern to keep client-specific shaping out of domain services
  - Contract-first development, code generation pipelines, and CI contract compatibility checks
  - API as a product: onboarding, keys, sandbox, docs, SDKs, quotas, billing, deprecation comms
- **Hands-on:** expose the same domain over REST, gRPC, and GraphQL. Measure payload size and latency, then list what each made easy and hard.

### ✅ Exit gate for Level 9

1. Design an idempotent POST endpoint that is safe under client retries and concurrent duplicates.
2. Explain cursor vs offset pagination with the failure mode of each under concurrent writes.
3. Define an evolution policy that lets you change an API for 5 years without breaking clients.
4. Solve GraphQL N+1 and query-cost abuse.
5. Choose a protocol for four scenarios and defend each.

---

## Level 10 — Security, Privacy & Multi-Tenancy

> **Why this level exists:** Security is an architectural property, not a feature you add. At Principal
> level you are expected to own threat models, tenant isolation, key management, and compliance
> constraints — and to say no to designs that cannot be made safe.

**Assumes:** Level 1 (TLS), Level 3 (data modelling), Level 8 (boundaries).

### 10.1 Foundations & threat modelling

- **Topics**
  - CIA triad + authenticity, non-repudiation; defence in depth; least privilege; fail-closed
  - Attack surface, trust boundary, threat actor, blast radius
  - **STRIDE** (Spoofing, Tampering, Repudiation, Information disclosure, DoS, Elevation of privilege)
  - Risk = likelihood × impact; risk acceptance and compensating controls
- **Subtopics**
  - Threat modelling workflow: diagram → identify trust boundaries → enumerate threats per element → mitigate → validate
  - Secure defaults, minimal exposure, deny-by-default networking
  - Security in the SDLC: design review, SAST/DAST/SCA, dependency scanning, secret scanning, pen testing, bug bounty
- **Advanced**
  - **Zero-trust architecture**: no implicit network trust, identity for every workload, per-request authorization, continuous verification, microsegmentation
  - Supply-chain security: SBOM, dependency pinning, provenance (SLSA), signed artifacts (Sigstore), reproducible builds, protected CI
  - Attack trees; abuse cases; designing for the malicious insider

### 10.2 Authentication & authorization

- **Topics**
  - Authentication factors, MFA/TOTP/WebAuthn-passkeys; password storage (bcrypt/scrypt/Argon2 + per-user salt)
  - Sessions (server-side, opaque cookie) vs tokens (self-contained JWT) — and when each is right
  - **OAuth 2.0/2.1** roles and grants: authorization code + PKCE (the default), client credentials, device code; **implicit and ROPC are deprecated**
  - **OpenID Connect**: ID token vs access token, scopes vs claims, userinfo, discovery
  - SAML, LDAP/AD, SSO, SCIM provisioning, social login
- **Subtopics**
  - JWT mechanics: header/payload/signature, `alg` confusion attacks, `none` algorithm, key rotation via JWKS + `kid`, `aud`/`iss`/`exp`/`nbf` validation
  - **JWT cannot be revoked** — mitigate with short TTL + refresh tokens + a revocation list/introspection; or use opaque tokens with introspection
  - Refresh token rotation with reuse detection; token binding (DPoP/mTLS)
  - Token storage in browsers: HttpOnly cookies vs localStorage (XSS exposure); `SameSite` + CSRF tokens
  - Service-to-service auth: mTLS, SPIFFE/SVID, workload identity, short-lived credentials, no static secrets
- **Authorization models**
  - **RBAC** (roles) — simple, coarse; role explosion at scale
  - **ABAC** (attributes/policies) — flexible, harder to audit
  - **ReBAC** (relationship-based, Google Zanzibar / OpenFGA / SpiceDB) — the right model for "who can access this document" at scale
  - **PBAC/policy-as-code** (OPA/Rego, Cedar) — decoupled decision point, PDP/PEP separation
  - Scopes vs permissions vs entitlements; multi-tenant permission scoping
- **Advanced**
  - Zanzibar architecture: relation tuples, userset rewrites, consistency (zookies), check/expand APIs, caching at massive scale
  - Authorization at the right layer: gateway (coarse) + service (domain) + data (row-level security)
  - Delegated access, impersonation, break-glass access with audit + approval, just-in-time privilege
  - Insecure Direct Object Reference (IDOR) as *the* most common real-world authz bug — enforce tenant/owner scoping in the query, not in the handler
- **Hands-on:** implement OIDC auth-code+PKCE login, short-lived access tokens + rotating refresh tokens with reuse detection, and ReBAC document permissions with OpenFGA. Then write a test suite that tries to access another tenant's data 20 different ways.

### 10.3 Data protection & cryptography in systems

- **Topics**
  - Encryption in transit (TLS/mTLS) vs at rest (disk/volume/DB/field-level) vs in use (confidential computing)
  - Key management: KMS, HSM, envelope encryption (DEK + KEK), key rotation, key hierarchy
  - Hashing vs encryption vs encoding vs tokenization vs masking vs redaction
  - Secrets management: Vault/Secrets Manager, dynamic secrets, no secrets in code/env/images/logs
- **Subtopics**
  - Field-level encryption for PII; deterministic vs randomized encryption and the searchability trade-off
  - **Crypto-shredding**: per-user DEK; delete the key to satisfy GDPR erasure across immutable stores and backups
  - Data classification (public/internal/confidential/restricted) driving controls
  - PII/PHI/PCI scope minimization; tokenization to keep card data out of your systems
- **Advanced**
  - Searchable encryption, blind indexes, HMAC-based equality search
  - Multi-region key residency and data-sovereignty constraints
  - Envelope encryption performance model; caching DEKs safely
  - Confidential computing (SGX/SEV/Nitro Enclaves) for "in use" protection

### 10.4 Application & platform security

- **Topics**
  - **OWASP Top 10**: broken access control, cryptographic failures, injection, insecure design, misconfiguration, vulnerable components, auth failures, integrity failures, logging failures, SSRF
  - **OWASP API Top 10**: BOLA/IDOR, broken auth, object property level authz, resource consumption, function-level authz, unrestricted business flows, SSRF, misconfig, inventory management, unsafe API consumption
  - Injection classes: SQL, NoSQL, command, LDAP, template, header, log; parameterized queries as the fix
  - XSS (stored/reflected/DOM), CSRF, clickjacking, CSP, SRI
  - SSRF (and cloud metadata endpoint theft — IMDSv2), path traversal, deserialization, XXE, ReDoS
- **Subtopics**
  - WAF, bot management, DDoS mitigation (L3/4 scrubbing + L7 rate limiting + anycast absorption)
  - Input validation (allow-lists), output encoding, canonicalization
  - Rate limiting as a security control (credential stuffing, enumeration, scraping)
  - Audit logging: what to log, what never to log (tokens, PII, card data), tamper-evident logs
- **Advanced**
  - Abuse and fraud architecture: velocity checks, device fingerprinting, risk scoring, step-up auth, shadow bans
  - Cloud IAM design: least-privilege roles, permission boundaries, cross-account trust, avoiding wildcard policies, workload identity federation (no long-lived keys)
  - Kubernetes security: RBAC, network policies, pod security standards, image signing/admission control, runtime detection, secrets encryption at rest
  - Compliance architecture: SOC 2, ISO 27001, PCI DSS, HIPAA, GDPR/DPDP — what each imposes on design (audit trails, retention, residency, access review, encryption, DR testing)

### 10.5 Multi-tenancy & isolation

- **Topics**
  - Isolation models: **silo** (per-tenant stack), **pool** (shared, tenant_id everywhere), **bridge** (shared compute, isolated data)
  - Tenant identity propagation and enforcement points
  - Noisy-neighbour control: quotas, rate limits, per-tenant concurrency caps, resource classes
- **Subtopics**
  - Row-level security (RLS) as a database-enforced backstop
  - Per-tenant encryption keys; per-tenant backups and restores; tenant export/offboarding
  - Tenant-aware observability: per-tenant SLOs, dashboards, cost attribution
  - Tenant tiering (free/pro/enterprise) mapped onto isolation and capacity
- **Advanced**
  - Cell-based multi-tenancy: tenants pinned to cells; migration between cells; per-cell deploys
  - Shuffle sharding for tenant blast-radius control (Level 6.2)
  - Data residency per tenant (EU-only tenants) and its architectural cost
  - Tenant-level rollout: per-tenant feature flags, staged migrations, tenant-scoped rollback
- **Hands-on:** build a pooled multi-tenant API with tenant-scoped tokens, Postgres RLS, per-tenant rate limits, and a cross-tenant leakage test suite. Then design (on paper) the migration to a bridge model for one enterprise tenant.

### ✅ Exit gate for Level 10

1. Threat-model a service with STRIDE and produce ranked mitigations.
2. Explain why JWTs can't be revoked and design a token strategy that handles logout and compromise.
3. Choose RBAC vs ABAC vs ReBAC for three scenarios and justify.
4. Explain envelope encryption and how crypto-shredding satisfies right-to-erasure.
5. Explain three ways cross-tenant data leaks happen and the enforcement layer that prevents each.
6. Design DDoS defence in depth across four tiers.

---

## Level 11 — Observability & Production Readiness

> **Why this level exists:** A system you cannot observe is a system you cannot operate, debug, or
> improve. Observability is the difference between a 5-minute and a 5-hour incident.

**Assumes:** Levels 5–6 (SLOs, latency), Level 8 (distributed call graphs).

### 11.1 The three pillars (and the fourth)

- **Topics**
  - **Metrics**: counters, gauges, histograms, summaries; pull (Prometheus) vs push (StatsD/OTLP)
  - **Logs**: structured (JSON) logging, levels, sampling, correlation IDs, retention
  - **Traces**: spans, parent/child, trace context propagation (W3C `traceparent`), sampling
  - **Profiles** (continuous profiling): CPU, heap, lock, off-CPU — the fourth pillar
  - Events/audit trails as a distinct stream
- **Subtopics**
  - **Cardinality** — the cost driver of metrics; never put user ID, request ID, or unbounded values in labels
  - Histogram buckets vs summaries; why you cannot average percentiles; mergeable estimators (t-digest, HdrHistogram)
  - Log volume control: sampling, level tuning, dropping health checks, cost per GB
  - Sampling strategies for traces: head-based (probabilistic), tail-based (keep slow/error traces), adaptive
  - Exemplars linking metrics → traces; trace → logs correlation
- **Advanced**
  - **OpenTelemetry** as the standard: SDK, auto-instrumentation, Collector (receive/process/export), OTLP, semantic conventions; vendor-neutral pipelines
  - Observability vs monitoring: monitoring answers known questions, observability lets you ask new ones (high-cardinality, arbitrary-dimension queries)
  - eBPF-based zero-instrumentation observability
  - Cost governance: observability bill often rivals compute; tiering, downsampling, retention policy per signal

### 11.2 What to measure

- **Topics**
  - **Golden signals** (Google): latency, traffic, errors, saturation
  - **RED** (Rate, Errors, Duration) for services; **USE** (Utilization, Saturation, Errors) for resources
  - Business/product metrics as reliability signals (orders/min is the best outage detector)
- **Subtopics**
  - Per-dependency metrics: call rate, error rate, latency, timeouts, circuit-breaker state, pool waits
  - Queue metrics: depth, age of oldest message, consumer lag, DLQ rate, processing time
  - Data metrics: replication lag, freshness/staleness of derived data, row-count deltas, reconciliation mismatches
  - Client-side/RUM metrics: Core Web Vitals (LCP/INP/CLS), app start time, real user latency by geography
- **Advanced**
  - Data quality/freshness SLOs for pipelines (Level 13)
  - Synthetic monitoring / black-box probes from multiple regions vs white-box internal metrics — you need both
  - Multi-window multi-burn-rate SLO alerting (e.g. page at 14.4× burn over 1 h, ticket at 3× over 6 h)
  - Anomaly detection and its false-positive economics; prefer SLO burn alerts over ML magic

### 11.3 Debugging distributed systems

- **Topics**
  - Correlation ID / request ID / trace ID propagation through every hop including async (put trace context in message headers)
  - Distributed tracing to find the slow hop; span attributes for DB queries, cache hits, retries
  - Log-trace-metric pivoting; the "one click from alert to root cause" goal
- **Subtopics**
  - Debugging tools: `perf`, flame graphs, `strace`, `tcpdump`, heap dumps, thread dumps, `pprof`, `EXPLAIN ANALYZE`
  - Reproducing production issues: shadow traffic, request replay, canary comparison
  - Detecting gray failure: client-observed vs server-observed disagreement
- **Advanced**
  - Causal debugging across async boundaries (causation vs correlation IDs)
  - Deterministic replay and simulation testing (Level 6.5) as a debugging superpower
  - Change-correlated diagnosis: overlay deploys, config changes, and flag flips on every dashboard — this alone resolves a large fraction of incidents
- **Hands-on:** instrument a 4-service call chain with OpenTelemetry, propagate context through a Kafka hop, then inject a slow dependency and find it from a single alert in under 3 minutes.

### 11.4 Dashboards, alerting & readiness

- **Topics**
  - Dashboard hierarchy: exec/business → service SLO → service internals → resource-level
  - Alert design: symptom-based (user impact) not cause-based; every page must be actionable and have a runbook
  - Ticket vs page vs log-only severity routing
- **Subtopics**
  - Runbook contents: what the alert means, how to confirm, immediate mitigations, escalation, rollback
  - Deployment markers, feature-flag audit, config-change events on dashboards
  - On-call handover and health of the rotation as a metric
- **Advanced**
  - Production readiness review checklist: SLOs defined, alerts wired, runbooks written, dashboards built, load tested, failure tested, DR tested, rollback verified, quota headroom, cost modelled, security reviewed, data lifecycle defined
  - Observability-driven development: instrument before you ship; the feature isn't done until it's observable
- **Hands-on:** write SLO burn-rate alerts for one service, delete every alert that has never been actionable, and produce a readiness review document.

### ✅ Exit gate for Level 11

1. Explain metric cardinality and give three real examples that would blow up cost.
2. Explain why averaging percentiles is invalid and what to do instead.
3. Design head-based + tail-based sampling for traces and justify the mix.
4. Write a multi-window multi-burn-rate alert for a 99.9% SLO.
5. Trace a latency regression end-to-end across an async boundary.

---

## Level 12 — Infrastructure, Deployment & Delivery

> **Why this level exists:** A design that cannot be deployed safely is not a design. Delivery mechanics
> determine how fast you can fix things — and MTTR is the dominant term in availability.

**Assumes:** Levels 0 (OS/cgroups), 1 (networking), 6 (reliability), 11 (observability).

### 12.1 Containers & orchestration

- **Topics**
  - Images, layers, registries, tags vs digests; multi-stage builds; distroless/minimal base images
  - cgroups + namespaces (the actual isolation mechanism); container runtimes (containerd, runc)
  - Kubernetes objects: Pod, ReplicaSet, Deployment, StatefulSet, DaemonSet, Job/CronJob, Service, Ingress/Gateway, ConfigMap, Secret, PVC, HPA/VPA, PDB, Namespace
  - Scheduling: requests vs limits, QoS classes, affinity/anti-affinity, taints/tolerations, topology spread
- **Subtopics**
  - Probes: liveness (restart me) vs readiness (route to me) vs startup (I'm slow to boot) — misconfigured liveness probes cause outages
  - Graceful shutdown: `SIGTERM` → drain → `preStop` hook → `terminationGracePeriodSeconds`; connection draining before exit
  - **CPU limits cause CFS throttling and destroy p99** — usually set requests, avoid tight CPU limits, always set memory limits
  - Pod Disruption Budgets, node drain, cluster autoscaler/Karpenter, spot node handling
  - StatefulSets, persistent volumes, storage classes; operators for stateful systems
- **Advanced**
  - Kubernetes networking: CNI, Services (ClusterIP/NodePort/LoadBalancer), kube-proxy/iptables/eBPF, DNS, Gateway API, network policies
  - Multi-cluster and multi-region Kubernetes; cluster as a failure domain; per-cell clusters
  - Operators and CRDs; extending the control plane; admission webhooks (and their outage potential)
  - When *not* to use Kubernetes (small teams, simple workloads — managed PaaS/serverless is cheaper in total cost)
- **Hands-on:** deploy a stateful + stateless app to Kubernetes with correct probes, PDBs, topology spread, and graceful drain. Then delete a node during a load test and confirm zero errors.

### 12.2 Infrastructure as code & configuration

- **Topics**
  - Declarative IaC (Terraform/OpenTofu, CloudFormation, Pulumi, CDK); state files, drift, plan/apply
  - Modules, environments, workspaces; immutable infrastructure
  - Configuration management: env vars, config files, config services, dynamic config
  - Secrets injection at runtime (never baked into images)
- **Subtopics**
  - Config vs code vs data; config as a change with a blast radius equal to a deploy — version, review, and roll it back the same way
  - Environment parity; ephemeral preview environments per PR
  - GitOps (Argo CD/Flux): git as the desired-state source, reconciliation loops, drift detection
- **Advanced**
  - Policy as code (OPA/Conftest/Sentinel) as a guardrail on IaC
  - Blast-radius-limited IaC: per-cell/per-region state, no global apply
  - Config-push outages: staged config rollout, validation, canary config, automatic revert (config is the #1 cause of large correlated outages)

### 12.3 CI/CD & deployment strategies

- **Topics**
  - CI: build, unit/integration/contract tests, lint, SAST/SCA, artifact build, provenance
  - CD: environment promotion, migrations, smoke tests, progressive rollout, automated rollback
  - Trunk-based development, short-lived branches, feature flags over long-lived branches
- **Deployment strategies**
  - **Rolling** — gradual replacement; simple, needs backward-compatible versions
  - **Blue/green** — two full environments, instant switch, easy rollback, double cost
  - **Canary** — small % of traffic, automated metric analysis, progressive promotion
  - **Shadow/dark traffic** — mirror real traffic to the new version with no user impact
  - **Feature flags / dark launch** — decouple deploy from release; per-user/per-tenant rollout
  - **A/B and experiments** — release as a measurement device
- **Subtopics**
  - Backward/forward compatibility across versions (N and N+1 running simultaneously — always)
  - **Expand/contract database migrations** coordinated with code rollout (add column → dual-write → backfill → read new → stop writing old → drop)
  - Rollback vs roll-forward; when rollback is impossible (irreversible migrations, consumed events) and how to plan for it
  - Automated rollback triggers wired to SLO/error metrics
- **Advanced**
  - Progressive delivery with automated analysis (Argo Rollouts/Flagger + metric providers)
  - Deployment as the top cause of incidents ⇒ smaller, more frequent, more automated deploys reduce risk (DORA)
  - Multi-region, multi-cell deployment ordering: one cell → one AZ → one region → global, with bake time between waves
  - Schema/contract compatibility gates in CI (protobuf/avro/OpenAPI breaking-change detection)
- **Hands-on:** set up a canary pipeline with automatic rollback on error-rate regression; then perform an expand/contract column rename across two deploys with zero downtime.

### 12.4 Cloud architecture, multi-region & cost

- **Topics**
  - Regions, AZs, edge locations; the shared-responsibility model
  - Managed vs self-hosted trade-off (operational cost vs control vs lock-in)
  - Multi-AZ (default), multi-region (deliberate), multi-cloud (rarely worth it)
  - Cost model: compute, storage, egress, requests, managed-service premium, support, observability
- **Subtopics**
  - Multi-region patterns: active-passive (simple, wasteful), read-local/write-global (common and good), region-partitioned writes (best scaling, needs routing), active-active multi-master (hardest, needs conflict resolution)
  - Data residency and routing users to their home region; global routing with anycast/geo-DNS
  - Cross-AZ/cross-region data transfer cost as an architecture driver (zone-aware routing pays for itself)
- **Advanced**
  - **FinOps**: unit economics (cost per request/tenant/feature), showback/chargeback, tagging discipline, rightsizing, commitment planning, autoscaling for cost, storage tiering, egress reduction via CDN/compression/peering
  - Lock-in analysis: which services are commodity (compute, object storage) vs sticky (proprietary DB, workflow, IAM); abstraction only where the cost is justified
  - Capacity reservation and quota management as a reliability concern (you can't scale into a quota wall during an incident)
  - Carbon/energy awareness as an emerging constraint
- **Hands-on:** build a cost model for a system you know: cost per 1000 requests and cost per tenant per month. Find the three largest line items and propose reductions with a latency/reliability trade-off statement for each.

### ✅ Exit gate for Level 12

1. Explain readiness vs liveness vs startup probes and the outage each misconfiguration causes.
2. Explain why CPU limits can hurt p99 and what to set instead.
3. Plan an expand/contract migration with the exact deploy ordering.
4. Design a canary pipeline with automated rollback criteria.
5. Choose a multi-region topology for a stated latency/consistency/compliance requirement.
6. Produce a unit-cost model and identify the dominant term.

---

## Level 13 — Data-Intensive, Analytics & ML Systems

> **Why this level exists:** Every mature product grows a second system behind the product: pipelines,
> warehouses, experimentation, and increasingly ML/LLM serving. Architects are expected to own the
> boundary between operational and analytical worlds.

**Assumes:** Levels 3, 4, 7 (CDC, streams, consistency), 11 (observability).

### 13.1 Data platform architecture

- **Topics**
  - OLTP vs OLAP separation; why analytical queries must never run on your primary
  - **Data warehouse** (schema-on-write, curated) vs **data lake** (schema-on-read, raw) vs **lakehouse** (open table formats over object storage)
  - Medallion architecture: bronze (raw) → silver (cleaned/conformed) → gold (business-level marts)
  - **ETL vs ELT**; why ELT won (cheap storage + powerful warehouse compute)
- **Subtopics**
  - Ingestion: batch loads, CDC streams, event streams, third-party connectors (Fivetran/Airbyte), API pulls
  - Open table formats: **Iceberg**, **Delta Lake**, **Hudi** — ACID on object storage, snapshot isolation, time travel, schema evolution, partition evolution, compaction/small-file problem
  - Storage layout: Parquet/ORC, partitioning, clustering/Z-ordering, file sizing, statistics
  - Orchestration: Airflow/Dagster/Prefect; DAGs, retries, backfills, idempotent tasks, SLAs per task
  - Transformation: dbt (models, tests, docs, lineage), SQL-first pipelines
- **Advanced**
  - **Lambda vs Kappa architecture**: dual batch+stream paths (correct but duplicated logic) vs stream-only with replay (simpler, requires a durable replayable log)
  - Data contracts between producers and the platform: schema, semantics, SLA, ownership, breaking-change process
  - **Data mesh**: domain-owned data products, self-serve platform, federated governance — and when it's premature
  - Reverse ETL (warehouse → operational systems) and the loops it creates
  - Query engines: MPP (Snowflake/BigQuery/Redshift), vectorized single-node (DuckDB), federated (Trino/Presto); separation of storage and compute; result caching
- **Hands-on:** build a pipeline: Postgres → Debezium CDC → Kafka → object storage (Iceberg) → dbt models → BI dashboard. Add freshness and row-count tests, then break a schema on purpose and watch the contract test fail.

### 13.2 Analytical data modelling

- **Topics**
  - Dimensional modelling: facts vs dimensions, star vs snowflake schema, grain
  - Slowly changing dimensions (Type 0/1/2/3/6); surrogate keys
  - Additive/semi-additive/non-additive measures; conformed dimensions
  - Wide/denormalized tables (One Big Table) as the modern columnar-friendly alternative
- **Subtopics**
  - Data Vault as an integration-layer alternative; when it's justified
  - Incremental models, late-arriving data, backfill strategy, partition-level idempotent rewrites
  - Metric layer / semantic layer: define a metric once, use everywhere (avoids "which revenue number is right?")
  - Testing data: uniqueness, not-null, referential integrity, accepted values, freshness, volume anomalies, reconciliation against source
- **Advanced**
  - Data observability: freshness, volume, schema, distribution, lineage; ownership and on-call for data
  - Governance: catalogue, lineage, PII tagging, access control, masking, retention, GDPR deletion propagation through derived tables
  - Cost control in warehouses: partition pruning, clustering, materialization strategy, query concurrency slots, spend alerts

### 13.3 Real-time analytics & serving

- **Topics**
  - Real-time OLAP stores: ClickHouse, Druid, Pinot, StarRocks — sub-second aggregation over streams
  - Pre-aggregation, rollups, materialized views, approximate aggregation (HLL/quantile sketches)
  - Serving layer for user-facing analytics (dashboards inside the product)
- **Subtopics**
  - Ingest from Kafka with exactly-once/idempotent upserts
  - Query latency vs freshness vs cost triangle
  - Hot/cold tiering; retention and downsampling
- **Advanced**
  - Incremental view maintenance (Materialize/RisingWave) for always-fresh SQL views
  - User-facing analytics multi-tenancy: per-tenant isolation, row-level filters, query cost limits
  - Streaming joins with dimension data (lookup vs broadcast vs CDC-materialized dimension tables)
- **Hands-on:** stream events into ClickHouse and serve a per-tenant dashboard with p95 < 200 ms over 1 B rows using pre-aggregation.

### 13.4 Search & recommendation systems

- **Topics**
  - Inverted index, tokenization, analyzers, stemming, stop words, n-grams
  - Ranking: TF-IDF, BM25, learning-to-rank; boosting and business rules
  - Query understanding: spell correction, synonyms, intent, entity extraction
  - Index pipeline: source of truth → CDC → index build → alias swap
- **Subtopics**
  - Sharding and replication of indexes; refresh interval vs freshness; near-real-time indexing
  - Faceting, aggregations, autocomplete (FST/trie/edge n-grams), typo tolerance
  - Deep pagination limits (`search_after`, no random deep offsets); result diversity
  - Reindexing safely with zero downtime (build new index, verify, atomically swap alias)
- **Advanced**
  - Hybrid retrieval: lexical (BM25) + vector (ANN) with reciprocal rank fusion; rerankers
  - Recommendations: collaborative filtering (matrix factorization, ALS), content-based, two-tower retrieval, candidate generation → filtering → ranking → re-ranking pipeline
  - Feature freshness, real-time signals, exploration/exploitation (bandits)
  - Offline vs online evaluation: NDCG/MAP/recall@k offline vs CTR/engagement in A/B tests
- **Hands-on:** build search over 1 M documents with BM25, add vector retrieval, fuse the results, and measure recall@10 against a hand-labelled set.

### 13.5 ML & LLM systems (MLOps)

- **Topics**
  - ML lifecycle: data collection → labelling → features → training → evaluation → deployment → monitoring → retraining
  - Training/serving skew; **feature store** (offline + online consistency, point-in-time correctness)
  - Batch inference vs online inference vs streaming inference; model registry & versioning
  - Model serving: latency budget, batching, GPU utilization, autoscaling, quantization, distillation, caching
- **Subtopics**
  - Data/label leakage; point-in-time joins; backtesting correctly
  - Drift detection: data drift, concept drift, prediction drift; monitoring model quality without labels
  - Shadow deployment, canary, champion/challenger, multi-armed bandit rollout for models
  - Experimentation platform: assignment, sample-ratio-mismatch checks, guardrail metrics, CUPED, sequential testing, network effects
- **Advanced LLM/RAG systems**
  - Retrieval-augmented generation architecture: chunking strategy, embedding model choice, vector index (HNSW/IVF-PQ), hybrid retrieval, reranking, context assembly, citation
  - Vector index internals: HNSW graph parameters (M, efConstruction, efSearch), IVF-PQ compression, recall/latency/memory trade-offs, filtered ANN search
  - LLM serving: KV cache, continuous batching, paged attention, speculative decoding, prefix caching, token streaming; cost per 1k tokens as a capacity unit
  - Agentic system design: tool schemas, sandboxing, loop termination, cost/latency budgets, human-in-the-loop gates, replayable traces
  - Evaluation: golden datasets, LLM-as-judge (with its bias caveats), regression suites, online feedback, guardrail classifiers
  - Safety/abuse: prompt injection (especially indirect via retrieved content), output validation, PII redaction, rate/spend limits, tenant isolation of embeddings
  - Caching for LLMs: exact-match, semantic cache, prefix cache — and staleness/quality trade-offs
- **Hands-on:** build a RAG service with hybrid retrieval, a reranker, an eval set of 50 questions, and a regression suite that runs in CI. Add per-tenant isolation and a spend cap.

### ✅ Exit gate for Level 13

1. Explain ELT vs ETL and lakehouse vs warehouse, and pick for a stated scenario.
2. Explain Lambda vs Kappa and when the duplication of Lambda is worth it.
3. Design a zero-downtime reindex for a search cluster.
4. Explain training/serving skew and how a feature store prevents it.
5. Design a RAG pipeline with explicit latency, cost, recall, and safety budgets.
6. Explain point-in-time correctness and the bug its absence causes.

---

## Level 14 — Case Studies (Progressive Difficulty)

> **How to do a case study:** 45–60 minutes, timed, using the 7-step method from §0.7. Write it up, then
> compare against a real engineering blog and list what you missed. Do **one per week minimum**.
> The write-up is the deliverable: requirements, estimates, API, schema, diagram, deep dives, bottlenecks, evolution.

### Tier 1 — Fundamentals (after Levels 0–3)

1. **URL shortener** — ID generation (base62, counter vs hash), collision handling, redirect latency, caching, click analytics, custom aliases, expiry
2. **Pastebin / text storage** — object storage vs DB, size limits, expiry, access control, syntax highlighting at read time
3. **Key-value store (single node)** — hash index, WAL, compaction, crash recovery, TTL, eviction
4. **Rate limiter** — algorithm choice, distributed counters, accuracy vs latency, per-dimension limits, fail-open vs fail-closed
5. **Unique ID generator** — Snowflake vs UUIDv7 vs ticket server; clock skew; monotonicity; multi-region
6. **Web crawler** — frontier queue, politeness/robots.txt, dedupe (Bloom + canonical URLs), DNS caching, trap detection, distributed workers, freshness scheduling
7. **Pagination & infinite scroll API** — cursor design, stability under writes, deletions, jumping to page N

### Tier 2 — Read-heavy at scale (after Levels 4–5)

8. **News feed / timeline (Twitter/Facebook)** — fan-out on write vs read, hybrid for celebrities, ranking, pagination, caching, feed staleness
9. **Instagram** — media upload pipeline, presigned URLs, transcoding, CDN, feed, follow graph, stories with TTL
10. **YouTube / Netflix** — upload → transcode ladder → packaging (HLS/DASH) → CDN → adaptive bitrate; recommendations; view counts; live vs VOD
11. **Twitter search / typeahead** — inverted index, real-time indexing, trending detection (Count-Min Sketch), autocomplete with tries
12. **Top-K / trending / leaderboard** — Redis sorted sets, sharded counters, approximate heavy hitters, time-decay, ties, pagination in ranks
13. **Web analytics / metrics system** — high-volume ingest, sketches for uniques (HLL), rollups, cardinality control, retention tiers
14. **Notification system** — multi-channel (push/email/SMS/in-app), templating, preferences, dedupe, batching/digest, provider failover, rate limits, delivery tracking
15. **Distributed cache** — consistent hashing, replication, hot keys, eviction, cluster membership, client library design

### Tier 3 — Write-heavy, real-time, stateful (after Levels 6–7)

16. **WhatsApp / chat** — connection gateway, presence, 1:1 and group messaging, ordering, offline delivery, read receipts, E2E encryption, multi-device sync, media
17. **Google Docs / collaborative editor** — OT vs CRDT, presence/cursors, offline edit merge, version history, snapshots, access control
18. **Uber / ride matching** — geospatial indexing (geohash/S2/H3), driver location ingest at high write rate, matching algorithm, ETA, surge pricing, trip state machine, payments
19. **Food delivery / logistics** — three-sided marketplace, order state machine, courier assignment, real-time tracking, ETA prediction, cancellation/compensation sagas
20. **Payment system** — idempotency, double-entry ledger, exactly-once charge, reconciliation, PSP failover, PCI scope, refunds/chargebacks, currency, audit
21. **Ticket booking (Ticketmaster/BookMyShow)** — inventory reservation, holds with TTL, preventing double-booking (write skew!), queueing during onsale spikes, fairness, payment timeout
22. **Hotel/flight booking** — availability search, inventory across providers, overbooking policy, price caching, distributed transactions vs sagas
23. **Scheduling / appointment system** — availability generation, timezone/DST correctness, recurrence (RRULE), overlap prevention with exclusion constraints, reschedule/cancel flows, reminders, real-time updates pushed via socket patches
24. **Stock exchange / order matching** — order book data structure, matching engine determinism, single-writer per symbol, sequencer + replicated log, market data fan-out, microsecond latency, fairness

### Tier 4 — Infrastructure-scale (after Levels 8–12)

25. **Google Drive / Dropbox** — chunking, content-addressed dedupe, delta sync, metadata service, conflict resolution, sharing/permissions, offline client, quotas
26. **S3-like object store** — namespace metadata, placement, erasure coding, durability math, multipart upload, consistency, lifecycle, request-rate scaling
27. **Distributed message queue (build Kafka)** — partitioned log, replication + ISR, leader election, consumer groups + offsets, retention, exactly-once semantics
28. **Distributed job scheduler / cron** — leader election, at-least-once vs at-most-once firing, missed-fire policy, timezone handling, backfill, sharded scheduling, dedupe
29. **Workflow engine (Temporal-like)** — durable execution, deterministic replay, event history, timers, signals, versioning of running workflows
30. **Search engine (Google-lite)** — crawl → index → serve; sharded inverted index, scatter-gather with tail-latency mitigation, ranking, caching, freshness
31. **Distributed SQL database** — range sharding, Raft per range, MVCC + HLC, distributed transactions, rebalancing, online schema change
32. **Monitoring & alerting platform (Prometheus/Datadog-scale)** — ingest, TSDB compression (Gorilla/delta-of-delta), cardinality control, query engine, rule evaluation, long-term storage
33. **Feature flag / config service** — low-latency global reads, streaming updates, targeting rules, consistency during rollout, static stability if the service is down
34. **CDN** — anycast, PoP cache hierarchy, purge propagation, TLS at the edge, edge compute, origin protection
35. **API gateway / rate-limited public API platform** — auth, quotas, plans/billing, versioning, developer portal, multi-tenant isolation
36. **Multi-region active-active SaaS** — data residency, routing, conflict handling, failover, per-tenant cells, global control plane with regional data planes
37. **Ad serving / real-time bidding** — <100 ms budget, candidate retrieval, pacing, budget enforcement under concurrency, fraud, event pipeline, billing accuracy
38. **RAG/LLM platform** — ingestion + chunking, vector index, hybrid retrieval, reranking, model serving, caching, eval harness, per-tenant isolation, spend limits

### Case-study write-up template

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

## Level 15 — Principal / Architect Practice

> **Why this level exists:** Above senior, the job stops being "produce the best design" and becomes
> "produce the best *decision* in this organization, with these people, on this timeline, and make it
> stick." Technical depth is table stakes; judgment, communication, and sequencing are the differentiators.

**Assumes:** Levels 0–13, plus real production experience.

### 15.1 Design judgment & trade-off analysis

- **Topics**
  - Requirements elicitation: eliciting the real constraint behind a stated request
  - Explicit trade-off framing: axes, options, weights, recommendation, what would change your mind
  - Reversibility analysis (one-way vs two-way doors); decision cost vs delay cost
  - Risk registers; assumptions made explicit and tracked
- **Subtopics**
  - Constraint discovery checklist: scale, latency, consistency, compliance, budget, team skill, timeline, existing systems, vendor contracts, organizational appetite
  - "Simplest thing that could possibly work" as a default, with a documented trigger for the next step
  - Build vs buy vs adopt-open-source: TCO including operations, hiring, migration, and exit cost
  - Designing for the team you have, not the team you wish you had
- **Advanced**
  - Recognizing and resisting resume-driven and hype-driven architecture
  - Pre-mortems: "it's 12 months later and this failed — why?"
  - Systems thinking: feedback loops, delays, unintended consequences, local optimum vs global optimum
  - Knowing the *shape* of the cost curve for every choice (what gets 10× more expensive at 10× scale)

### 15.2 Communication artifacts

- **Topics**
  - **ADR (Architecture Decision Record)**: context, decision, status, consequences, alternatives — one page, immutable, numbered
  - **RFC / design doc**: problem, goals/non-goals, proposal, alternatives, risks, migration, rollout, testing, observability, security, cost, open questions
  - **C4 diagrams** at the right altitude; sequence diagrams for flows; one diagram per question
  - Executive summaries: the decision, the cost, the risk, the ask — in five sentences
- **Subtopics**
  - Writing for the audience: engineers (mechanism), managers (risk/cost/timeline), executives (outcome/trade-off)
  - Review culture: how to run a design review that improves the design rather than defending it
  - Documenting non-goals explicitly — the highest-leverage section of any design doc
- **Advanced**
  - Tech radar / tech strategy documents; paved roads and guardrails
  - Migration narratives that survive leadership changes (phased, each phase independently valuable)
  - Influence without authority: build coalitions, prototype to persuade, let others own pieces
- **Hands-on:** write one ADR and one full RFC for a real decision you've faced. Get them reviewed by someone who disagrees with you.

### 15.3 Migration & modernization at scale

- **Topics**
  - Strangler fig, branch by abstraction, anti-corruption layer, parallel run
  - Dual-write + CDC + shadow read + diff + cutover + backout — the universal data migration pattern
  - Incremental value delivery; avoiding multi-year big-bang rewrites
- **Subtopics**
  - Shadow/dark traffic and diffing (compare old vs new responses in production without user impact)
  - Data reconciliation tooling and acceptable divergence thresholds
  - Rollback design for irreversible steps (feature flags, versioned schemas, replayable logs)
  - Deprecation: telemetry on usage, migration guides, hard deadlines, forced-migration mechanics
- **Advanced**
  - Sequencing a multi-year migration into independently shippable phases with kill criteria at each phase
  - Managing dual-cost periods and communicating them to finance
  - Organizational aspects: who owns the old system while the new one is built; avoiding a permanent hybrid state
- **Hands-on:** write a phased migration plan (with rollback criteria per phase) for a real legacy component you know.

### 15.4 Organization, process & platform

- **Topics**
  - **Conway's law** and the inverse Conway manoeuvre; Team Topologies (stream-aligned, platform, enabling, complicated-subsystem); interaction modes (collaboration, X-as-a-service, facilitating)
  - Cognitive load as the real constraint on service count per team
  - Ownership models: you build it, you run it; on-call ownership as a design incentive
- **Subtopics**
  - Platform engineering: golden paths, service templates, self-service infra, internal developer portal
  - Governance that scales: guardrails as code, automated fitness functions, architecture review only for one-way doors
  - Technical debt: taxonomy (deliberate/prudent vs inadvertent/reckless), interest rate, paydown budgeting (fixed % of capacity)
  - Hiring/skills as an architectural constraint; documentation and onboarding as reliability features
- **Advanced**
  - Standardization vs autonomy: standardize interfaces and observability, allow variation inside services
  - Multi-team API/event contract governance and versioning at organizational scale
  - Architecture strategy on a 2–3 year horizon: capability roadmap, investment sequencing, sunsets
  - Measuring architecture: DORA metrics, change failure rate, incident classes, lead time, cost per unit, cognitive load surveys

### 15.5 Interview & presentation craft (if you're targeting roles)

- **Topics**
  - Time allocation for a 45-minute design round: 5 requirements, 5 estimates, 5 API/data, 10 high-level, 15 deep dives, 5 wrap-up
  - Driving the conversation; stating assumptions; asking clarifying questions; checking in before deep dives
  - Signals interviewers grade: structure, breadth, depth on demand, trade-off reasoning, scale math, failure thinking, communication
- **Subtopics**
  - Common failure modes: jumping to a diagram, no estimates, buzzword soup, one solution with no alternatives, ignoring failure and operations, arguing instead of exploring
  - Handling "how would you scale this 100×?" and "what breaks first?" — always have an answer
  - Principal-level extras: organizational impact, migration path, cost, staffing, risk, and a stated recommendation
- **Advanced**
  - Whiteboarding legibly under time pressure; the four-box high-level diagram you can always start from
  - Turning a vague prompt into a scoped problem in 3 questions
  - Disagreeing with the interviewer productively
- **Hands-on:** do 20 timed mock designs from Level 14, recording yourself. Review for filler, structure, and whether you ever said "it depends, and here's my pick."

### 15.6 Staying current without chasing hype

- **Topics**
  - Primary sources: papers (Dynamo, Bigtable, Spanner, Kafka, Raft, Chubby, Borg, Zanzibar, Chain Replication, Calvin, Percolator, MapReduce, GFS, Dapper, Monarch, F1, Aurora), engineering blogs, conference talks (SREcon, QCon, Strange Loop, VLDB/SIGMOD, CIDR)
  - Reading a paper efficiently: problem → constraints → key insight → evaluation → limitations
  - Keeping a personal decision journal; revisiting predictions
- **Advanced**
  - Distinguishing a genuine capability shift from repackaging (ask: what new thing is now *possible*, and what does it cost?)
  - Evaluating new tech: maturity, operational burden, community, exit cost, does it remove a real constraint you have today?
  - Teaching as the strongest retention mechanism — write, present, mentor

### ✅ Exit gate for Level 15

1. Write an ADR that a skeptical peer accepts without a meeting.
2. Present a design at three altitudes (engineer, manager, exec) in under 5 minutes each.
3. Produce a phased migration plan with per-phase rollback and kill criteria.
4. Defend a deliberately "boring" architecture against a hype-driven alternative on cost, risk, and time.
5. Explain the organizational consequences of a technical boundary you proposed.
6. Complete 20 timed case studies with written trade-off sections.

---

# Appendices

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

## Appendix B — Pattern Catalogue (quick reference)

**Scaling:** horizontal scaling · stateless services · read replicas · sharding · consistent hashing · caching (aside/through/behind) · CQRS · materialized views · fan-out on write/read/hybrid · precomputation · batching · async processing · CDN · edge compute · connection pooling · database proxy · queue-based load levelling

**Consistency:** single-leader replication · quorum (R+W>N) · read-your-writes · monotonic reads · bounded staleness · MVCC · optimistic concurrency (ETag/version) · pessimistic locking (`FOR UPDATE`) · exclusion constraints · consensus (Raft) · leader lease · fencing tokens · CRDTs · saga · transactional outbox · inbox/dedupe · idempotency keys · two-phase commit · event sourcing

**Reliability:** timeouts · retries with jittered backoff · retry budget · circuit breaker · bulkhead · fallback/graceful degradation · load shedding · rate limiting · backpressure · health checks (shallow/deep) · redundancy (N+1/2N) · multi-AZ · multi-region · cell-based architecture · shuffle sharding · static stability · constant work · DLQ · quarantine queue · chaos experiments · canary · feature flag kill switch · blue/green · progressive rollout

**Latency:** local L1 cache · hedged requests · tied requests · deadline propagation · cancellation · request coalescing (single-flight) · prefetch · compression · protocol upgrade (H2/H3/gRPC) · zone-aware routing · geo-partitioning · read-local/write-global · anycast · connection reuse · TLS session resumption · thread-per-core

**Data movement:** CDC · outbox · change streams · event-carried state transfer · claim check (pointer to blob) · dual-write + backfill + shadow read + cutover · reverse ETL · log compaction · replay

**Boundaries:** bounded context · anti-corruption layer · BFF · API gateway · sidecar/service mesh · strangler fig · branch by abstraction · published language · database-per-service · shared-nothing

**Multi-tenancy:** silo/pool/bridge · tenant_id + RLS · per-tenant keys · per-tenant quotas · cell pinning · shuffle sharding · tenant tiering

---

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

## Appendix E — Resources (curated, ordered)

### Books — core sequence
1. **Designing Data-Intensive Applications** — Kleppmann. The single most important book here. Read twice: once at Level 3, once at Level 4.
2. **Understanding Distributed Systems** — Vitillo. Excellent, shorter companion.
3. **Database Internals** — Petrov. Storage engines + distributed systems internals.
4. **Site Reliability Engineering** + **The SRE Workbook** — Google. Free online. SLOs, error budgets, incident response.
5. **Release It!** — Nygard. Stability and capacity patterns; the origin of many resilience patterns.
6. **Fundamentals of Software Architecture** + **Software Architecture: The Hard Parts** — Richards & Ford. Architecture styles, trade-off analysis, decomposition.
7. **Domain-Driven Design Distilled** (Vernon) → **Implementing DDD** (Vernon) → **Domain-Driven Design** (Evans).
8. **Building Microservices** (2e) — Newman.
9. **System Design Interview Vol 1 & 2** — Xu. Best for case-study drilling, not for depth.
10. **Systems Performance** (2e) + **BPF Performance Tools** — Gregg. For Level 5.
11. **Team Topologies** — Skelton & Pais. For Level 15.
12. **Data Mesh** (Dehghani) / **Fundamentals of Data Engineering** (Reis & Housley) — for Level 13.
13. **Streaming Systems** — Akidau et al. Event time, watermarks, the definitive treatment.
14. **Designing Machine Learning Systems** — Huyen. For Level 13.5.

### Papers (read at Level 4+, in this order)
Raft → Chubby → Dynamo → Bigtable → GFS → MapReduce → Chain Replication → Zanzibar → Percolator → Spanner → Calvin → Kafka → Dapper → Borg → Monarch → Aurora → FoundationDB → Delta Lake/Iceberg → The Tail at Scale → Harvest & Yield → CAP Twelve Years Later.

### Blogs & continuous sources
Engineering blogs: Netflix, Uber, Airbnb, Stripe, Cloudflare, Discord, Slack, Shopify, Meta, Google (SRE + research), Dropbox, Figma, Canva, Segment, Notion, Datadog, LinkedIn, Grab, Zalando, Booking, Pinterest, DoorDash.
Also: AWS Builders' Library (short, excellent, pattern-focused), Martin Fowler's site, Brendan Gregg's blog, Aphyr/Jepsen analyses, Marc Brooker's blog, Murat Demirbas's blog, ByteByteGo, Kleppmann's talks, InfoQ/QCon and SREcon talks, ACM Queue.

### Hands-on platforms
Local Kubernetes (kind/minikube), Docker Compose stacks, `tc netem` for network faults, Toxiproxy, k6/Gatling/wrk/vegeta for load, Jepsen/Elle/Porcupine for consistency checking, TLA+ Toolbox, Testcontainers, LocalStack, Debezium, ClickHouse/DuckDB locally, Temporal dev server.

---

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

## Appendix G — Progress Tracker

Check a level only after passing its exit gate. Record the date and one line about what surprised you.

```
[ ] L0  Prerequisites & Mental Models        started ______  passed ______
[ ] L1  Networking & The Web                 started ______  passed ______
[ ] L2  Core Building Blocks                 started ______  passed ______
[ ] L3  Databases & Storage Engines          started ______  passed ______
[ ] L4  Distributed Systems Theory           started ______  passed ______
[ ] L5  Scalability & Performance            started ______  passed ______
[ ] L6  Reliability & Resilience             started ______  passed ______
[ ] L7  Messaging & Event-Driven             started ______  passed ______
[ ] L8  Architecture Styles & DDD            started ______  passed ______
[ ] L9  API & Interface Design               started ______  passed ______
[ ] L10 Security & Multi-Tenancy             started ______  passed ______
[ ] L11 Observability                        started ______  passed ______
[ ] L12 Infrastructure & Delivery            started ______  passed ______
[ ] L13 Data-Intensive & ML Systems          started ______  passed ______
[ ] L14 Case Studies                         ____ / 38 completed
[ ] L15 Principal / Architect Practice       ____ / 20 mock designs, ____ ADRs, ____ RFCs
```

### Labs completed

```
[ ] 1  [ ] 2  [ ] 3  [ ] 4  [ ] 5  [ ] 6  [ ] 7  [ ] 8  [ ] 9  [ ] 10
[ ] 11 [ ] 12 [ ] 13 [ ] 14 [ ] 15 [ ] 16 [ ] 17 [ ] 18 [ ] 19 [ ] 20
[ ] 21 [ ] 22 [ ] 23 [ ] 24 [ ] 25 [ ] 26 [ ] 27 [ ] 28 [ ] 29 [ ] 30
[ ] 31 [ ] 32 [ ] 33 [ ] 34 [ ] 35 [ ] 36
```

### Spaced repetition schedule

Re-test yourself on these at 1 week / 1 month / 3 months / 6 months after passing each level:
- Latency table + estimation math (L0) — the most perishable and most used
- Isolation levels + anomalies (L3)
- Consistency model hierarchy (L4)
- Quorum and consensus rules (L4)
- Resilience pattern list + failure modes (L6)
- Delivery semantics + idempotency (L7)

---

## Appendix H — Maintaining This Roadmap

This file is meant to be edited as you learn. Keep it useful, not pristine.

**Weekly (15 min)**
- Update `Status:` and `Last updated:` at the top.
- Tick anything finished in [Appendix G](#appendix-g--progress-tracker).
- Add a line to the changelog below for anything you added or corrected.

**Per level completed**
- Add a short "**My notes:**" bullet list under the level with the 3–5 things that surprised you.
- Add any subtopic you discovered was missing (this file is a starting set, not a closed one).
- Link your own detailed notes: `> Notes: ~/notes/system-design/level-3-indexing.md`

**Per case study**
- Add a line under [Level 14](#level-14--case-studies-progressive-difficulty): date, which design, what you missed, which real blog post you compared against.

**Quarterly**
- Re-read your ADRs/design docs from the last quarter and note what you'd decide differently now.
- Prune resources that didn't help; add ones that did.
- Re-do one spaced-repetition item per level you've passed.

**When technology changes**
- Add new technologies as *subtopics under an existing concept*, never as a new level. Concepts are stable;
  implementations are not. If something doesn't fit an existing concept, that's a signal it may be genuinely new —
  note why in the changelog.

**Changelog**

```
2026-09-08  Initial version. Levels 0–15 + appendices A–I.
```

---

## Appendix I — Glossary

**ACID** — Atomicity, Consistency, Isolation, Durability. **ABAC** — attribute-based access control. **ADR** — architecture decision record. **AF** — amplification factor. **ANN** — approximate nearest neighbour. **AZ** — availability zone.
**BASE** — basically available, soft state, eventual consistency. **BDP** — bandwidth-delay product. **BFF** — backend for frontend. **BFT** — Byzantine fault tolerance. **BM25** — a lexical relevance ranking function.
**CAP** — consistency, availability, partition tolerance. **CDC** — change data capture. **CDN** — content delivery network. **CFS** — the Linux completely fair scheduler (source of container CPU throttling). **CQRS** — command query responsibility segregation. **CRDT** — conflict-free replicated data type. **CTE** — common table expression.
**DEK/KEK** — data/key encryption key (envelope encryption). **DLQ** — dead letter queue. **DORA** — the four delivery metrics. **DR** — disaster recovery.
**EOS** — exactly-once semantics. **ETL/ELT** — extract-transform-load / extract-load-transform.
**FLP** — Fischer-Lynch-Paterson impossibility result. **FMEA** — failure mode and effects analysis.
**GSI/LSI** — global/local secondary index. **HLC** — hybrid logical clock. **HLL** — HyperLogLog. **HNSW** — hierarchical navigable small world (vector index). **HOL** — head-of-line blocking. **HTAP** — hybrid transactional/analytical processing.
**IDOR/BOLA** — insecure direct object reference / broken object level authorization. **IaC** — infrastructure as code. **ISR** — in-sync replicas (Kafka).
**LSM** — log-structured merge tree. **LSN** — log sequence number. **LWW** — last write wins.
**MTBF/MTTR/MTTD** — mean time between failures / to repair / to detect. **MVCC** — multi-version concurrency control.
**NFR** — non-functional requirement. **OLTP/OLAP** — online transaction/analytical processing. **OTel** — OpenTelemetry.
**PACELC** — partition → A/C, else → latency/consistency. **PDP/PEP** — policy decision/enforcement point. **PITR** — point-in-time recovery. **PoP** — point of presence.
**QUIC** — UDP-based transport underlying HTTP/3. **RBAC/ReBAC** — role/relationship-based access control. **RED/USE** — service and resource metric frameworks. **RLS** — row-level security. **RPO/RTO** — recovery point/time objective.
**SLI/SLO/SLA** — service level indicator/objective/agreement. **SSI** — serializable snapshot isolation. **SSRF** — server-side request forgery. **SSTable** — sorted string table (LSM on-disk file). **STRIDE** — a threat taxonomy. **SWIM** — a gossip membership protocol.
**TCC** — try-confirm/cancel. **TSDB** — time-series database. **TTL** — time to live. **USL** — universal scalability law. **WAL** — write-ahead log. **WORM** — write once read many. **XID** — transaction ID. **2PC/3PC** — two/three-phase commit. **2PL** — two-phase locking.

---

*End of roadmap. It is meant to be edited — see [Appendix H](#appendix-h--maintaining-this-roadmap).*
