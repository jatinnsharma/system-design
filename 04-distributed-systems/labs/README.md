# Labs — Level 4 — Distributed Systems Theory

- [ ] write a small distributed counter with 3 nodes, then use `iptables`/`tc` to create a partition and a one-way partition. Observe how each breaks.
- [ ] write the same "follower count" feature under linearizable, causal, and eventual guarantees. Enumerate what a user can observe in each.
- [ ] implement Lamport and vector clocks over a 3-node message passing sim. Produce a concurrent-write conflict and detect it with vector clocks but not with Lamport.
- [ ] implement Raft leader election + log replication for a 3-node key-value store (or run a Raft library and kill leaders in a loop). Verify linearizability with a checker like Jepsen/Elle or Porcupine.
- [ ] implement a money transfer across two services three ways — 2PC, orchestrated saga, outbox+events — and enumerate the failure windows of each.
- [ ] build a collaborative counter and a collaborative text buffer with a CRDT library. Take one client offline for 5 minutes, edit both sides, reconnect, and verify convergence.
- [ ] run a 5-node gossip cluster, partition 2 nodes, and observe convergence and false-positive failure detection as you tune timeouts.
