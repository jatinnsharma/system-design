# Labs — Level 13 — Data-Intensive, Analytics & ML Systems

- [ ] build a pipeline: Postgres → Debezium CDC → Kafka → object storage (Iceberg) → dbt models → BI dashboard. Add freshness and row-count tests, then break a schema on purpose and watch the contract test fail.
- [ ] stream events into ClickHouse and serve a per-tenant dashboard with p95 < 200 ms over 1 B rows using pre-aggregation.
- [ ] build search over 1 M documents with BM25, add vector retrieval, fuse the results, and measure recall@10 against a hand-labelled set.
- [ ] build a RAG service with hybrid retrieval, a reranker, an eval set of 50 questions, and a regression suite that runs in CI. Add per-tenant isolation and a spend cap.
