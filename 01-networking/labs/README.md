# Labs — Level 1 — Networking & The Web

- [ ] capture a TCP handshake and a QUIC handshake in Wireshark. Then use `tc netem` to add 200 ms latency + 2% loss and observe throughput collapse.
- [ ] trace a lookup with `dig +trace`. Set up a domain with weighted + failover routing and measure real failover time.
- [ ] serve the same page over H1, H2, H3 and compare waterfall + p95 on a lossy link. Implement ETag-based optimistic concurrency on a PUT endpoint.
- [ ] stand up TLS with your own CA and enforce mTLS between two services. Then let a cert expire on purpose and watch the failure mode.
- [ ] put HAProxy or Envoy in front of 3 backends, kill one mid-load-test, and measure error count and recovery time under each algorithm.
- [ ] deploy static assets behind a CDN, measure hit ratio, then deliberately break `Cache-Control` and watch origin load explode.
- [ ] build a chat room with WebSockets that survives a rolling deploy without losing messages (hint: resume cursor + drain period).
