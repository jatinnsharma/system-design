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

