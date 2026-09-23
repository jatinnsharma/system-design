# Labs — Level 0 — Prerequisites & Mental Models

- [ ] write a benchmark that traverses 1 GB sequentially vs randomly, in RAM and on disk. Plot the four numbers. Keep them.
- [ ] compute the theoretical minimum latency for a user in Mumbai calling a service in Virginia, then measure it.
- [x] implement a Bloom filter and a Count-Min Sketch from scratch. Measure false-positive rate vs your formula. → [`0.3-bloom-countmin/`](0.3-bloom-countmin/) (C++)
- [ ] write a TCP echo server three ways — thread-per-connection, `epoll` event loop, async runtime. Load test all three to find the knee.
- [ ] build a bounded thread-safe queue; then deliberately create a deadlock and a livelock and diagnose both from a thread dump.
- [ ] estimate storage/bandwidth/servers for Instagram, WhatsApp, and Uber. Compare against published engineering blogs.
- [ ] design a URL shortener using the 7 steps in exactly 45 minutes, timed, on paper.
