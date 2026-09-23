# Labs — Level 7 — Messaging, Streaming & Event-Driven Architecture

- [ ] build a producer/consumer pair with at-least-once delivery, deliberately duplicate messages, and make the consumer idempotent with a dedupe table. Prove correctness under duplicate + reorder + replay.
- [ ] run a 3-broker Kafka cluster; kill a leader mid-produce with `acks=1` vs `acks=all` and quantify data loss; then trigger a rebalance under load and measure the stall.
- [ ] implement an event-sourced aggregate (bank account or booking) with snapshots and two projections; then evolve the event schema and upcast old events.
- [ ] implement outbox + relay + idempotent consumer for an order/payment flow. Then implement the same flow in Temporal and compare the amount of code you had to write for retries, timeouts, and compensation.
- [ ] compute a 5-minute windowed count with event-time semantics, inject out-of-order and very late events, and observe the difference between watermark strategies.
