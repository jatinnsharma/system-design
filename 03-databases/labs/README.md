# Labs — Level 3 — Databases & Storage Engines (Deep Dive)

- [ ] model a booking/scheduling domain (users, providers, availability, appointments, overlapping-slot prevention) with real constraints — including an exclusion constraint that makes double-booking impossible.
- [ ] take a 10 M-row table; write 10 queries; make each one fast using an index; prove it with `EXPLAIN ANALYZE`; then measure the write-throughput cost of the indexes you added.
- [ ] reproduce every row of the anomaly table above with two concurrent `psql`/`mysql` sessions. Then create a write-skew bug (two bookings for the same slot) and fix it four different ways.
- [ ] run the same write-heavy benchmark against MySQL/InnoDB and RocksDB/Cassandra. Graph write throughput, read p99, and disk usage over an hour of sustained load.
- [ ] build a Postgres primary + 2 replicas. Measure lag under write load. Cause a failover. Then engineer a read-your-writes guarantee three ways and measure the latency cost.
- [ ] shard a table across 4 databases with hash routing, implement scatter-gather for one query, then resplit to 8 shards with zero downtime.
- [ ] model the same "activity feed" domain in Postgres, Cassandra, and DynamoDB. Write the same five queries against each. Document what became hard.
- [ ] perform a real PITR restore to a timestamp 10 minutes in the past. Then run an `ALTER TABLE` on a 50 M-row table with zero downtime.
