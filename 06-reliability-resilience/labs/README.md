# Labs — Level 6 — Reliability, Resilience & Operability

- [ ] wrap a flaky dependency with timeout + jittered retry + circuit breaker + bulkhead + fallback. Prove with a fault-injection test that a 100%-down dependency does not exhaust your threads or violate your SLO for the unaffected paths.
- [ ] write and execute a DR plan for a small app: measure real RPO/RTO by destroying the primary region and recovering. Record the gap between planned and actual.
- [ ] run one experiment per failure domain against your own service. Document one surprise per experiment. Then write a TLA+ spec for a small protocol (e.g. a lease-based lock) and let it find a bug.
- [ ] write a production-readiness checklist for a service you own, then run a postmortem on any recent real incident using a proper template.
