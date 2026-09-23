# Notes — Level 0 — Prerequisites & Mental Models
> Source: [`../ROADMAP.md`](../ROADMAP.md), started ______, passed exit gate ______

## 0.1 Computer architecture as a performance model

> Ye detailed Hinglish notes hain — technical terms (epoll, fsync, TLB, cache line, NUMA, SIMD, etc.) English mein hi rakhe hain kyunki interview aur docs mein wahi use hote hain, samjhaya gaya hai Hindi-English mix mein. Har topic ka structure fixed hai: **kya hai → kyun important hai → kaise kaam karta hai → real-world + backend/system-design example.**

### 1. CPU, cores, hardware threads, sockets, NUMA nodes

**Kya hai:** Ek server mein sirf ek "brain" nahi hota. Motherboard par ek ya zyada **sockets** hote hain — har socket ek alag physical CPU chip hai apne khud ke pins, apni khud ki memory controller ke saath. Har socket ke andar multiple **cores** hote hain — har core apna khud ka independent execution unit hai, apne registers, apna L1/L2 cache. Aur aajkal har core aksar **2 hardware threads** run kar sakta hai ek saath — isko Intel **hyperthreading** kehta hai aur generic term hai **SMT** (Simultaneous Multi-Threading). Ye 2 threads same physical execution units (ALU, FPU) share karte hain, bas kuch registers alag hote hain — matlab ye "2 free cores" nahi hain, ye ek core ka better utilization hai jab ek thread stall ho (cache miss ka wait kar raha ho) to doosra thread usi cycle ko use kar le. Aur sabse important piece: **NUMA** (Non-Uniform Memory Access) — modern multi-socket server mein RAM globally shared nahi hoti jaisa purane zamane mein hota tha. Har socket ke paas apni **local** RAM directly attached hoti hai (integrated memory controller ke through), aur doosre socket ki RAM access karne ke liye data ko inter-socket interconnect (Intel ka **UPI**, AMD ka **Infinity Fabric**) se guzarna padta hai — jo slower hai.

**Kyun important hai:** Agar ye mental model nahi hai, to intuition bolegi "jitne zyada cores, utna zyada throughput, linearly scale hoga." Reality mein aisa nahi hota. Ek 96-core, 2-socket machine actual mein **2 alag NUMA domains** hai jo aapas mein juda hua hai — agar application ka memory allocation aur thread scheduling in domains ko respect nahi karta, to bahut saara time sirf remote-memory access mein waste hota hai, aur wo box effectively ek chhote box jaisa perform karta hai. Ye exactly wo scenario hai jahan production mein "hum add more cores kiya but latency same/worse ho gayi" jaisa mystery bug milta hai — jiska root cause sirf NUMA topology samajhne se hi pakad mein aata hai.

**Kaise kaam karta hai:** OS scheduler (Linux CFS) har process/thread ko kisi bhi core par schedule kar sakta hai by default — matlab ek thread jo socket-0 ki memory allocate karke baithi thi, agla time-slice socket-1 ke core par migrate ho sakti hai, aur ab har memory access remote ho jayega (typically 1.5x–2x zyada latency local access ke comparison mein, aur bandwidth bhi kam). Linux `numactl` tool se aap kisi process ko specific NUMA node par **pin** kar sakte ho (CPU affinity + memory policy dono), taaki memory allocation aur thread execution same node par rahe. Kernel khud bhi NUMA-aware hai — default policy `local` hoti hai jahan process pehli baar page touch karta hai wahi node se allocate hota hai (`first-touch` policy), lekin explicit control ke bina scheduler ise migrate kar sakta hai.

**Real-world example aur backend/system-design connection:** Postgres jaisa database dual-socket, high-core-count machine par deploy karte waqt DBAs `numactl --interleave=all postgres ...` ya explicit node pinning use karte hain, kyunki Postgres ka shared buffer pool agar sirf ek NUMA node par allocate ho jaye aur backend processes doosre node ke cores par schedule ho jayein, to har buffer access remote ban jata hai — throughput 20-30% tak gir sakta hai bina kisi obvious config galti ke. Kubernetes mein bhi `CPU Manager` ka `static` policy is exact problem ko address karta hai — pods ko whole cores pin karta hai taaki noisy-neighbor aur cross-NUMA scheduling na ho, jo latency-sensitive services (payment gateway, matching engine) ke liye critical hai. Ye seedha backend decision hai: high-throughput DB ya cache node provision karte waqt sirf "cores kitne hain" nahi, "kitne sockets/NUMA nodes hain aur kya application NUMA-aware hai" bhi dekhna padta hai.

```bash
# System ki socket/core/thread/NUMA topology dekhna
lscpu
# Output mein dekho: "Socket(s)", "Core(s) per socket", "Thread(s) per core", "NUMA node(s)"

# NUMA nodes aur unki memory distances dekhna
numactl --hardware
# node distances table dikhata hai — local access ~10, remote access ~20-21 (units relative hain)

# Ek process ko NUMA node 0 par pin karke chalana (CPU + memory dono)
numactl --cpunodebind=0 --membind=0 ./my_server
```

**Quick summary:** Server = multiple sockets × cores × hardware threads, aur RAM per-socket local hoti hai (NUMA) — is topology ko ignore karke naive "more cores = more speed" sochna production mein silent throughput loss deta hai.

**Practice tasks:**
- Apne machine ya kisi cloud VM par `lscpu` aur `numactl --hardware` chalao aur output ko kisi ko explain karo ki kitne sockets, cores, NUMA nodes hain.
- Zor se explain karo: "hyperthreading do full cores kyun nahi hain" — apne shabdon mein.

---

### 2. Memory hierarchy: registers → L1/L2/L3 cache → RAM → SSD → HDD → network → tape/cold storage

**Kya hai:** CPU se data jitna "door" hota hai, use fetch karne mein utna zyada time lagta hai — aur ye difference linear nahi, **orders-of-magnitude** ka hota hai. Layer-by-layer, roughly:

| Layer | Approx latency | Kitna bada hota hai (typical) |
|---|---|---|
| CPU Register | < 1 ns | bytes |
| L1 cache | ~1 ns | 32–64 KB per core |
| L2 cache | ~3–4 ns | 256 KB–1 MB per core |
| L3 cache | ~10–20 ns | few MB, shared per socket |
| RAM (DRAM) | ~80–120 ns | GBs |
| SSD (NVMe) | ~50–150 µs | TBs |
| HDD | ~5–10 ms (seek) | TBs |
| Network (same datacenter) | ~0.3–0.5 ms | — |
| Network (cross-region) | ~50–150 ms | — |
| Tape / cold storage (Glacier) | minutes–hours | PBs |

**Kyun important hai:** Ye table software architecture ke har layer mein caching kyun exist karti hai iska seedha proof hai. Jab pata chal jaye ki RAM, network round-trip se ~1000–100000x fast hai, aur L1 cache RAM se ~100x fast hai, to "cache laga do" sirf ek buzzword nahi reh jata — ye ek measurable, predictable, napkin-math se justify hone wala engineering decision ban jata hai. Har system design interview ka latency budget isi table se derive hota hai.

**Kaise kaam karta hai:** Har layer ka trade-off same hai — jitna chhota aur CPU ke paas, utna fast lekin utna hi mehenga (per byte) aur kam capacity. Registers CPU silicon ke andar hi hain (zero wire delay). SRAM-based caches (L1/L2/L3) bhi on-chip hain lekin bade transistor count ki wajah se bade nahi ban sakte. DRAM (RAM) off-chip hai, ek alag chip, memory bus ke through access hota hai — physically door hone ki wajah se hi latency badhti hai. SSD/HDD block devices hain jo OS ke I/O stack, driver, aur (HDD ke case mein) mechanical head movement se guzarte hain — HDD mein seek time hi sabse bada contributor hai (physically head ko sahi track par le jaana). Network add karta hai serialization, routing, aur speed-of-light delay — jo cross-region hone par dominant factor ban jata hai.

**Real-world example aur backend/system-design connection:** User profile fetch karne ke do designs compare karo — (a) Redis se directly (in-memory, ~1ms including network round-trip) vs (b) Postgres se disk se read (agar page cache mein nahi hai to SSD I/O + query planning, ~10–50ms). Ye 10-50x ka difference hai jo aap build karne se pehle hi is table se predict kar sakte ho — isi liye high-QPS read paths mein caching layer (Redis, Memcached) itni common hoti hai. System design interviews mein latency budgets (jaise "P99 200ms ke andar rehna hai") isi hierarchy ko explicitly worked-out karke justify kiye jate hain.

```text
# Worked example: ek API request jo 3 layers touch karta hai

Step 1: App server memory se config read (RAM)         ~=      100 ns
Step 2: Redis cache hit, same datacenter (network)      ~=  500,000 ns   (0.5 ms)
Step 3: Cache miss hone par Postgres disk read (SSD)     ~=  100,000 ns   (0.1 ms)
                                                          -----------------
Total worst case (cache miss + DB hit)                   ~=  600,100 ns  ~= 0.6 ms

Agar Redis bhi miss ho aur cross-region DB call lage:
Step 3': Cross-region network round trip                ~= 100,000,000 ns (100 ms)
Total                                                     ~= 100.5 ms   <- 150x zyada!
```

**Quick summary:** Har memory/storage layer CPU se door hone ke saath orders-of-magnitude slow hoti jaati hai (register → L1 → RAM → SSD → HDD → network) — isi progression ne saari caching architecture (app cache, CDN, DB buffer pool) ko justify kiya hai.

**Practice tasks:**
- Kisi remote server ko `ping` karke actual network latency dekho, aur usko upar ke table ke numbers se compare karo.
- Apne current project ka ek read path lo aur uske har hop (cache/DB/network) ka worked-out latency estimate likho, jaise upar diya gaya hai.

---

### 3. Cache lines (64B), spatial and temporal locality, cache misses

**Kya hai:** CPU RAM se ek-ek byte fetch nahi karta — har baar RAM se ek pura **cache line** (typically **64 bytes**) fetch karke cache mein le aata hai. Do related ideas: **spatial locality** — agar ek memory address use hua hai, to uske paas-paas ke addresses bhi jaldi use honge (isliye pura 64B block fetch karna sense banata hai — "free mein" pados ka data bhi mil jata hai). **Temporal locality** — jo data abhi use hua hai, wo dobara jaldi use hoga (isi liye recently accessed data cache mein rakha jata hai). Jab CPU ko chahiye wala data kisi bhi cache level (L1/L2/L3) mein nahi milta, use **cache miss** kehte hain — tab CPU ko RAM tak jaana padta hai jo (jaisa upar table mein dekha) ~100ns lagta hai, jo CPU cycles ke hisaab se sainkadon cycles ka stall hai.

**Kyun important hai:** Ye batata hai ki data structure ka memory layout algorithm ke Big-O jitna hi real-world performance ko affect karta hai. Same asymptotic complexity ke do implementations real wall-clock time mein 5-10x tak alag ho sakte hain sirf is wajah se ki ek cache-friendly hai aur doosra nahi. Interviews aur system design dono mein ye "Big-O poori kahani nahi batata" wala critical nuance hai.

**Kaise kaam karta hai:** Jab CPU ek address maangta hai, hardware us address ko cache line boundary (64B-aligned block) mein round karke poora block fetch karta hai — is process mein ek **prefetcher** bhi kaam karta hai jo sequential access pattern detect karke agle blocks ko pehle hi speculatively fetch kar leta hai, bina CPU ne maanga hi. Cache multiple levels mein organize hota hai — L1 sabse chhota/fastest per-core, L2 bada per-core, L3 sabse bada aur socket ke saare cores mein shared hota hai. Jab multiple cores same cache line ko modify karte hain, cache-coherence protocol (jaise **MESI** — Modified/Exclusive/Shared/Invalid) us line ko cores ke beech sync rakhta hai, jo cost add karta hai (isi se agle subtopic mein "false sharing" nikalta hai).

**Real-world example aur backend/system-design connection:** 2D array/matrix ko **row-major** order mein traverse karna (jaise C/C++ mein memory mein actually stored hota hai) sequential cache lines hit karta hai — fast. Wahi matrix ko **column-major** order mein traverse karna har access par memory mein door jump karta hai — almost har access ek naya cache miss — same `O(n²)` complexity hone ke bawajood real time mein kayi guna slow. Ye exact reasoning columnar databases (ClickHouse, Apache Parquet, DuckDB) ke peeche hai — jab query ko sirf 2 columns chahiye 50 mein se, columnar layout sirf wahi 2 columns ke contiguous bytes padhta hai (cache- aur I/O-efficient), jabki row-oriented storage (Postgres heap tuples) har row ka pura data touch karta hai chahe zyada columns na bhi chahiye.

```c
#include <stdio.h>
#include <time.h>

#define N 4096
static int matrix[N][N];

int main(void) {
    struct timespec t0, t1;

    // Row-major traversal: memory mein jaisa stored hai waisa hi order — cache-friendly
    clock_gettime(CLOCK_MONOTONIC, &t0);
    long sum = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++)
            sum += matrix[i][j];          // sequential 64B cache lines
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("row-major: %ld ns\n",
           (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec));

    // Column-major traversal: same data, opposite order — cache-hostile
    clock_gettime(CLOCK_MONOTONIC, &t0);
    sum = 0;
    for (int j = 0; j < N; j++)
        for (int i = 0; i < N; i++)
            sum += matrix[i][j];          // har access ek naya cache line (stride = N*4 bytes)
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("column-major: %ld ns\n",
           (t1.tv_sec - t0.tv_sec) * 1000000000L + (t1.tv_nsec - t0.tv_nsec));
    return 0;
}
// Typical result: column-major 5-10x slower, same O(n^2) work
```

**Quick summary:** CPU 64-byte cache lines fetch karta hai, na ki single bytes — isi wajah se memory layout ka locality (spatial/temporal) real performance decide karta hai, aksar algorithm ke Big-O se bhi zyada.

**Practice tasks:**
- Upar diya C code compile karke (`gcc -O2 file.c -o bench && ./bench`) khud row-major vs column-major ka actual time difference measure karo apne machine par.
- Zor se explain karo ki "same Big-O ke bawajood ek code doosre se fast kyun hai" — cache line ke reference ke saath.

---

### 4. Instruction pipelining, branch prediction, speculative execution, SIMD

**Kya hai:** Modern CPU ek instruction poora complete karke doosri start nahi karta — wo **pipelining** use karta hai: instruction ko stages mein todta hai (fetch → decode → execute → memory → writeback) aur ek time par kai instructions ko alag-alag stages mein overlap karke process karta hai, jaise ek factory assembly line. `if` conditions (branches) is pipeline ke liye problem hain — CPU ko pata nahi hota branch kis taraf jayega jab tak condition evaluate na ho, lekin pipeline ko khali nahi rakhna chahta. Isliye CPU **branch prediction** karta hai (history-based heuristics, saturating counters, Branch Target Buffer) aur predicted path par **speculatively** aage execute karna shuru kar deta hai — agar prediction sahi nikla to time bacha, agar galat nikla (**misprediction**) to pura speculative kaam discard karke pipeline **flush** karna padta hai (typically 15-20 cycles ka waste). **SIMD** (Single Instruction, Multiple Data — SSE/AVX/AVX-512 jaise instruction sets) ek alag optimization hai: ek hi instruction se multiple data elements (jaise 4, 8, ya 16 numbers) ek saath process karna, wide registers (128/256/512-bit) use karke.

**Kyun important hai:** Ye samjhata hai ki **unpredictable** branches (jaise random data par `if (x > threshold)`) predictable branches se dramatically slow kyun hote hain — pipeline baar-baar flush hota hai. Aur numeric/data-heavy workloads (image processing, ML inference, compression, database query engines) SIMD-friendly loops likhne se kyun 4-16x tak fast ho sakte hain — compiler auto-vectorization ya manual intrinsics dono is samajh par based hain.

**Kaise kaam karta hai:** Modern CPUs **superscalar** aur **out-of-order** hote hain — matlab wo ek saath multiple instructions issue kar sakte hain aur unka execution order program order se alag ho sakta hai (dependencies allow karein to), phir ek **reorder buffer** unhe sahi order mein "retire" (commit) karta hai taaki program ka observable behavior correct rahe. Branch predictor typically 2-bit saturating counter jaisi scheme se har branch ka history track karta hai — agar same branch baar-baar same direction leta hai (jaise ek loop condition), prediction accuracy 95%+ tak ho sakti hai. Misprediction hone par CPU ko speculative results discard karke sahi path se dobara start karna padta hai — ye penalty pipeline depth ke barabar cycles ki hoti hai. SIMD instructions compiler auto-vectorization (`-O3 -march=native` jaise flags) se ya explicit intrinsics (`_mm256_add_ps` jaise) se generate hote hain, aur ek CPU cycle mein poori wide register (jaise AVX2 ka 256-bit = 8 floats) par operation apply karte hain.

**Real-world example aur backend/system-design connection:** Ek classic benchmark — bade array ko pehle **sort** karke phir filter loop (`if (x > threshold) count++`) chalana, unsorted array par same filter chalane se dramatically fast hota hai — sorted data mein branch predictor ko long runs of true/true/true...false/false milte hain (easy to predict), unsorted mein har iteration essentially coin-flip hai (branch predictor fail karta hai, pipeline baar-baar flush hoti hai). Production side par: modern analytical databases jaise **ClickHouse** aur query engines jaise **DuckDB** apne filter/aggregate operators ko explicitly SIMD-vectorized likhte hain (batch of rows par ek saath operate karte hain) — isi wajah se columnar OLAP engines row-at-a-time traditional engines se 10-50x fast query execution de paate hain. Speculative execution ka ek security angle bhi backend/system-design decision banta hai — **Spectre/Meltdown** vulnerabilities isi speculative execution ki side-channel leakage se aayi thi, jisne cloud providers (AWS, GCP) ko multi-tenant isolation (VM vs container boundary, hypervisor patches) ke baare mein seedha re-think karwaya.

```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N (1 << 20)

int main(void) {
    int *data = malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) data[i] = rand() % 256;

    struct timespec t0, t1;
    long count = 0;

    // Pass 1: unsorted data par branchy filter — branch predictor ke liye almost random
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++)
        if (data[i] >= 128) count++;      // ~50% true, ~50% false -> bahut mispredicts
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("unsorted filter: %ld ns, count=%ld\n",
           (t1.tv_sec-t0.tv_sec)*1000000000L + (t1.tv_nsec-t0.tv_nsec), count);

    // Ab sort kar do
    int cmp(const void *a, const void *b) { return (*(int*)a - *(int*)b); }
    qsort(data, N, sizeof(int), cmp);

    // Pass 2: same filter, sorted data par — predictor ko long true/false runs milte hain
    count = 0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < N; i++)
        if (data[i] >= 128) count++;      // ab ek hi "flip point" hai -> almost no mispredicts
    clock_gettime(CLOCK_MONOTONIC, &t1);
    printf("sorted filter:   %ld ns, count=%ld\n",
           (t1.tv_sec-t0.tv_sec)*1000000000L + (t1.tv_nsec-t0.tv_nsec), count);
    return 0;
}
// Typical result: sorted filter kaafi zyada fast hota hai, same comparisons ke bawajood
```

**Quick summary:** Pipelining CPU ko multiple instructions overlap karne deta hai, branch prediction usko busy rakhta hai `if` ke around — galat predict hone par flush penalty lagti hai; SIMD ek hi instruction se multiple data elements process karke numeric workloads ko dramatically fast karta hai.

**Practice tasks:**
- Upar wala sorted-vs-unsorted filter benchmark khud compile-run karke number dekho apne machine par.
- Apne shabdon mein explain karo ki "branch misprediction" aur "cache miss" dono hi "pipeline ko rokte hain" lekin alag reasons se kaise.

---

### Subtopics — practical aha points

**Array traversal linked-list se fast hai, same Big-O hone ke bawajood:** Dono traversal `O(n)` hain complexity mein, lekin array memory mein **contiguous** hota hai — sequential access prefetcher ke through cache-friendly hai. Linked list ke nodes memory mein (heap allocator ke hisaab se) bikhre hote hain — har `node->next` pointer dereference ek naya, unpredictable memory location hai, jo likely ek fresh cache miss hai. *Fayda:* ye samajh aata hai ki Big-O poori kahani nahi hai — real-world constant factors (memory layout) typical production data sizes ke liye Big-O se bhi zyada matter kar sakte hain, isi liye `ArrayList`/slice ko `LinkedList` par default choice mana jata hai jab tak specific insert/delete pattern na chahiye ho.

**False sharing:** Jab do alag threads **alag-alag** independent variables likhte hain jo memory mein by coincidence **same 64-byte cache line** par land ho jate hain, to har thread ka write us line ko cache-coherence protocol (MESI) ke through invalidate kar deta hai doosre core ke liye — even though data logically unrelated hai. Result: dono threads baar-baar RAM se re-fetch karte hain jaise wo ek hi shared variable par contend kar rahe hon. *Fayda:* isi se wo counter-intuitive multi-threaded slowdown samajh aata hai jahan zyada threads add karne se cheezein slow ho jati hain. Fix: struct ko explicitly pad karke hot variables ko alag cache lines par force karna.

```go
// Go: false sharing ka example — do counters same struct mein adjacent hain
type Counters struct {
    a int64 // thread 1 ye likhta hai
    b int64 // thread 2 ye likhta hai — same 64B cache line mein hone ki wajah se contend karega
}

// Fix: padding daal ke dono ko alag cache lines par force karo
type PaddedCounters struct {
    a int64
    _ [56]byte // padding taaki 'a' apni poori 64B line le le
    b int64
}
```

**Sequential vs random access SSD vs HDD par:** Sequential reads dono SSD aur HDD par fast hain (predictable pattern, prefetch-friendly). Random reads SSD par bhi fast hain (no moving parts), lekin HDD par bahut slow (~5-10ms per seek — physically read head ko disk ke sahi track par move karna padta hai). *Fayda:* isi single fact ne database storage engines ka pura design decide kiya hai — B-trees aur LSM-trees dono random disk I/O minimize karne ke liye design kiye gaye hain (yehi Level 3 ka core foundation hai). Postgres ka `random_page_cost` config parameter literally isi trade-off ko encode karta hai.

**Page size (4KB), page cache, `mmap`, dirty pages, write-back vs write-through:** OS disk ko fixed-size **pages** (typically 4KB) mein manage karta hai, aur RAM mein transparently recently-used disk pages ko cache karta hai — isko **page cache** kehte hain. `mmap()` syscall se ek file ko directly process ke address space mein map kiya ja sakta hai, taaki file access normal memory access jaisa dikhe (kernel background mein pages load karta hai). Jab koi page RAM mein modify hoti hai but disk par abhi flush nahi hui, use **dirty page** kehte hain. **Write-back** caching (dirty page ko baad mein, batched tareeke se flush karna — fast but risk of loss on crash) vs **write-through** (har write turant disk tak jata hai — safe but slower) dono OS aur database level dono par exist karte hain. *Fayda:* samajh aata hai ki server "disk se" data fast kaise serve karta hai (actually zyada tar RAM-backed page cache se hi ho raha hota hai), aur power loss recent writes kyun lose kar sakta hai jo sirf dirty page cache mein the, jab tak `fsync()` explicitly call na ho.

```bash
# Linux mein page cache ka size dekhna
free -h
# "buff/cache" column mein page cache included hota hai

# Kisi process ka dirty pages flush force karna (durability guarantee)
sync
# ya application code mein: fsync(fd) call karke ek specific file ke dirty pages disk tak force karna
```

**DMA, zero-copy (`sendfile`, `splice`) — Kafka aur Nginx ka throughput:** Naive tareeke se disk se network tak data bhejne ke liye 4 copies lagti hain: disk → kernel buffer → user-space app buffer → kernel socket buffer → NIC. **DMA** (Direct Memory Access) CPU ko in copies ke beech bypass karne deta hai — disk controller aur NIC directly RAM se data move kar sakte hain CPU ko har byte ke liye involve kiye bina. **Zero-copy** syscalls jaise `sendfile()` aur `splice()` kernel ko file data ko directly socket tak bhejne dete hain, bina user-space mein copy kiye. *Fayda:* Kafka ka broker jab consumer ko log segment serve karta hai, aur Nginx jab static file serve karta hai, dono `sendfile()`-style zero-copy use karte hain — unnecessary CPU cycles aur memory bandwidth bachate hain, jo high-throughput (GB/s range) systems ke liye critical differentiator hai.

---

### Advanced concepts

**NUMA-aware allocation aur thread pinning:** Jaisa Topic 1 mein dekha, 96-core dual-socket box actually 2 alag NUMA-local "sub-machines" hain jo ek interconnect se jude hain. Advanced level par is samajh se aage badhkar — memory allocator (jaise `jemalloc`, `tcmalloc`) ko bhi NUMA-aware banaya ja sakta hai taaki thread jis node par run kar raha hai, memory usi node se allocate ho (`numa_alloc_local()` jaisi APIs). *Fayda:* high-performance systems (Cassandra JVM tuning guides, Postgres, ScyllaDB — jo explicitly per-core shard architecture rakhta hai) NUMA nodes par processes/shards pin karne ki recommendation isi wajah se dete hain — cross-node memory traffic ko architecturally eliminate karna.

**Mechanical sympathy — struct-of-arrays vs array-of-structs:** Agar ek bade struct mein se query pattern ko sirf ek-do fields baar-baar chahiye (jaise sirf `price` field 1 million `Order` structs mein se), to un fields ko **separate parallel arrays** mein store karna (**struct-of-arrays**, SoA) ek array of full structs (**array-of-structs**, AoS) se zyada cache-efficient hai — kyunki tab CPU sirf relevant column ke contiguous bytes fetch karta hai, irrelevant fields ko cache line mein "waste" kiye bina. *Fayda:* yahi core idea hai jiski wajah se columnar databases (ClickHouse, Apache Parquet-based engines) analytics workloads (aggregate over 2 columns of a 50-column table) ke liye row-oriented databases se dramatically fast hote hain.

```c
// Array-of-Structs (AoS) — traditional, row-oriented
struct Order { int id; double price; char status[16]; double tax; };
struct Order orders[1000000];
// Agar sirf 'price' ka sum chahiye, phir bhi poora struct (id+status+tax) cache mein load hota hai

// Struct-of-Arrays (SoA) — columnar
struct Orders {
    int    id[1000000];
    double price[1000000];   // sirf ye array touch hota hai sum ke liye — fully cache-friendly
    char   status[1000000][16];
    double tax[1000000];
};
```

**Hardware failure statistics — bit rot, silent data corruption, AFR, checksums:** Disks (HDD aur SSD dono) kabhi-kabhi **silently** fail hote hain — koi error signal nahi milta, bas ek bit flip ho jata hai (cosmic ray, manufacturing defect, degraded media ki wajah se) — isko **bit rot** ya **silent data corruption** kehte hain. **AFR** (Annualized Failure Rate) — ek population of disks mein per-year expected failure percentage — datacenter-scale par ye ek statistical certainty hai (Google/Backblaze ke published disk reliability studies isi ko quantify karte hain, typically 1-3% AFR range mein). *Fayda:* samajh aata hai ki har serious storage system (**ZFS**, **Cassandra**, **Amazon S3**) end-to-end checksums kyun use karta hai — "disk ne error return nahi kiya" iska matlab "data correct hai" nahi hota, ye sirf ek weak signal hai. Checksums (aur replication) hi actual guarantee dete hain.

**Modern storage — NVMe queue depth, write amplification, SSD wear levelling, TRIM, persistent memory:** SSDs flash memory ke physical constraint ki wajah se **in-place overwrite** nahi kar sakte — purana data erase karke naya likhna padta hai, aur erase block-granularity (bada, jaise 256KB-4MB) mein hota hai jabki writes chhote (4KB) hote hain — isse **write amplification** hota hai (ek logical 4KB write, actual mein bahut zyada physical bytes likh/erase kar sakta hai). SSD controller **wear levelling** karta hai — writes ko physical blocks mein evenly spread karta hai taaki koi ek block jaldi na ghisay (flash cells ki limited erase-cycle life hoti hai). **TRIM** command OS SSD ko batata hai ki konse blocks ab logically free hain, taaki controller unhe proactively erase/reclaim kar sake background mein. **NVMe** protocol (PCIe-based, SATA AHCI ka successor) **high queue depth** support karta hai — ek saath 64K queues, har ek 64K commands deep, jisse massive I/O parallelism milta hai jo purana SATA/AHCI (single queue, 32 deep) nahi de sakta tha. **Persistent memory** (jaise Intel Optane, ab largely discontinued but concept relevant) RAM jaisi latency ke paas but persistent (non-volatile) storage deta hai — memory aur storage ke beech ki line blur karta hai. *Fayda:* samajh aata hai ki SSDs fill hone ke saath (kam free blocks bachne par) slow kyun ho jate hain, aur log-structured/sequential-write-favoring databases (LSM-tree based — Cassandra, RocksDB) SSD-friendly kyun hain jabki naive random-write-heavy patterns SSD life aur throughput dono ko hurt karte hain.

---

### Important terms

- **Socket:** Physical CPU chip slot motherboard par; har socket apni memory controller ke saath aata hai.
- **Core:** Socket ke andar ek independent execution unit — apne registers aur L1/L2 cache ke saath.
- **Hardware thread / SMT / Hyperthreading:** Ek core ke execution units ko 2 logical threads ke beech time-share karna, taaki ek thread ke stall hone par doosra progress kare.
- **NUMA (Non-Uniform Memory Access):** Multi-socket system architecture jahan har socket ki apni local RAM hoti hai, aur doosre socket ki RAM access karna slower hota hai (remote access).
- **Cache line:** Fixed-size (typically 64B) block jo CPU ek unit mein RAM se cache tak fetch karta hai.
- **Spatial locality:** Ek memory address use hone ke baad uske aas-paas ke addresses bhi jaldi use hone ki tendency.
- **Temporal locality:** Ek memory address dobara jaldi use hone ki tendency.
- **Cache miss:** Jab chahiye wala data kisi cache level mein nahi milta aur RAM tak jaana padta hai.
- **MESI protocol:** Cache-coherence protocol (Modified/Exclusive/Shared/Invalid states) jo multi-core cache lines ko sync rakhta hai.
- **False sharing:** Do independent variables same cache line par hone ki wajah se unrelated threads ke beech unnecessary cache invalidation.
- **Pipelining:** CPU ka instructions ko overlapping stages mein process karna, ek assembly-line ki tarah.
- **Branch prediction:** CPU ka heuristic guess ki ek conditional branch kis direction jayega, pipeline ko busy rakhne ke liye.
- **Speculative execution:** Predicted path par pehle se instructions execute karna, misprediction hone par discard karna.
- **Pipeline flush:** Misprediction ke baad speculative results discard karke pipeline restart karna (cost: ~15-20 cycles).
- **SIMD (Single Instruction, Multiple Data):** Ek instruction se multiple data elements par parallel operation (SSE/AVX/AVX-512).
- **Page (memory page):** OS-level fixed-size (typically 4KB) memory management unit.
- **Page cache:** OS ka RAM-based cache jo disk pages ko transparently store karta hai.
- **Dirty page:** RAM mein modified page jo abhi disk par flush nahi hui.
- **fsync:** Syscall jo kisi file descriptor ke dirty pages ko forcibly disk tak flush karta hai (durability guarantee).
- **mmap:** Syscall jo ek file ko process ke address space mein directly map karta hai.
- **DMA (Direct Memory Access):** Hardware capability jisse devices CPU ko bypass karke directly RAM se data move kar sakte hain.
- **Zero-copy:** Techniques (`sendfile`, `splice`) jo data ko user-space copy kiye bina kernel ke andar hi move karte hain.
- **Write amplification:** SSD par ek logical write ke corresponding hone wali extra physical erase/write activity.
- **Wear levelling:** SSD controller ki strategy jo writes ko evenly distribute karti hai taaki flash cells uniformly ghisein.
- **TRIM:** Command jisse OS SSD ko batata hai ki kaunse blocks logically free hain, reclaim ke liye.
- **AFR (Annualized Failure Rate):** Ek disk population ka expected per-year failure percentage.
- **Bit rot / silent data corruption:** Bina kisi error signal ke storage medium par data ka corrupt ho jana.
- **Struct-of-Arrays (SoA) vs Array-of-Structs (AoS):** Data layout strategies — fields ko separate arrays mein (columnar) vs ek combined struct ke array mein (row-oriented).

---

### Common mistakes

- Sirf core count dekh kar capacity planning karna, NUMA topology ignore karke — resulting mein "more cores lekin same/worse latency" wala production surprise.
- Linked list ko "O(n) hai to array jaisa hi fast hoga" soch kar bade datasets ke liye choose karna, cache-miss cost ignore karke.
- Multi-threaded counters/flags ko ek hi struct mein adjacent fields ke roop mein rakhna bina padding ke — false sharing se silently scaling na hona.
- `fsync()` na karna aur assume kar lena ki data "written" hai kyunki `write()` call succeed ho gaya — jabki wo abhi sirf dirty page cache mein hai, crash par lost ho sakta hai.
- Random-write-heavy workload ko naively SSD par likhna bina samjhe ki write amplification aur wear levelling long-term throughput/lifespan ko kaise degrade karte hain.
- Column-major traversal jaisa cache-hostile access pattern likhna (jaise numpy/matrix code mein galat loop order) aur phir wonder karna ki "same algorithm slow kyun hai."
- Checksums/data-integrity verification skip karna storage layer mein ye assume karke ki "disk kabhi silently corrupt nahi hoga."
- Branch-heavy hot-path code likhna unpredictable conditions ke saath (jaise random data par filters) bina sorting/batching ke alternative consider kiye, jabki predictable branch pattern se bada speedup mil sakta tha.

---

### Interview questions

1. Ek 64-core machine add karne ke baad bhi throughput expected se kam hai — kya possible reasons ho sakte hain? *(Hint: NUMA cross-node memory access, thread scheduling migration, socket-level memory controller bottleneck.)*
2. Cache line kya hota hai aur wo "false sharing" kaise create karta hai? *(Hint: 64B granularity + independent variables same line par + MESI invalidation ping-pong.)*
3. Same `O(n)` complexity ke bawajood array traversal linked list se fast kyun hota hai? *(Hint: contiguous memory = spatial locality = prefetcher-friendly vs scattered heap nodes.)*
4. Branch misprediction ka actual cost kya hota hai aur ise kaise minimize karte hain? *(Hint: pipeline flush ~15-20 cycles; sorting/batching data se predictable pattern banake reduce karo.)*
5. SIMD kya hai aur ye analytics databases (jaise ClickHouse) ko traditional row-stores se fast kyun banata hai? *(Hint: ek instruction, multiple data elements — vectorized filter/aggregate ops per cycle.)*
6. Dirty page aur `fsync` ka relationship samjhao — power loss ke case mein kya ho sakta hai? *(Hint: write-back caching, data RAM mein hai disk par nahi, crash before fsync = data loss.)*
7. Zero-copy (`sendfile`/`splice`) normal file-serving se better kyun perform karta hai? *(Hint: 4-copy path (disk→kernel→user→socket) ko eliminate karke DMA se direct kernel-to-NIC move.)*
8. Struct-of-Arrays data layout kab prefer karoge array-of-structs ke upar, aur kyun? *(Hint: jab access pattern sirf kuch columns touch karta hai baar-baar — columnar analytics workloads.)*
9. SSD write amplification kya hai aur ye database design (jaise LSM-trees) ko kaise influence karta hai? *(Hint: erase-before-write granularity mismatch; sequential/log-structured writes amplification minimize karte hain.)*
10. Silent data corruption / bit rot se protect karne ke liye storage systems kya karte hain, aur "disk ne error nahi diya" kyun kaafi nahi hai? *(Hint: end-to-end checksums, replication, scrubbing — hardware-level silence ek weak signal hai, not a guarantee.)*

---

### Hands-on lab

> 1 GB data ko sequentially vs randomly traverse karne ka benchmark likho — RAM mein bhi aur disk par bhi. Chaaron numbers plot karo. Inhe sambhaal kar rakho.

**Ye specific lab kyun:** Ye is poore section ki sabse important cheez hai — sirf padhe hue facts ko khud measure kiye hue numbers mein badalta hai. Tumhe 4 concrete data points milenge (RAM-sequential, RAM-random, disk-sequential, disk-random) jo orders-of-magnitude alag honge, aur yehi gap caching, indexing, aur poore storage-engine design ke exist karne ki wajah hai. Level 3 (databases/storage) aage isi numbers ko baar-baar reference karega, isliye inhe likh kar rakhna zaroori hai.

- [ ] Mera result: RAM-sequential = ______, RAM-random = ______, Disk-sequential = ______, Disk-random = ______

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.2 Latency numbers every engineer must know

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har topic neeche isi order mein chalega: **kya hai → kyun important hai (fayda) → kaise kaam karta hai (mechanism) → real-world aur backend/system-design example.**

### 1. The canonical latency table (memorize the orders of magnitude, not the digits)

**Kya hai:** Ye ek "cheat sheet" hai jo batata hai ki computer ke har operation mein roughly kitna time lagta hai — L1 cache se lekar cross-continent network call tak. Isko **Jeff Dean ki "Numbers Every Programmer Should Know"** table bhi kaha jata hai (originally Google ke andar circulate hoti thi, ab publicly famous hai). Important baat ye hai — tumhe exact "17 ns" ya "0.5 ms" yaad rakhne ki zarurat nahi hai. Tumhe sirf ye yaad rakhna hai ki **har row previous row se roughly 10x–1000x slow hai**, aur ye gaps kabhi change nahi hote, chahe hardware saal-dar-saal fast hota rahe.

**Kyun important hai:** Ye table tumhara **back-of-envelope estimation tool** hai. Production mein jab koi bologa "ye API 200ms le rahi hai, normal hai kya?" — tumhe benchmark chalaye bina hi pata chal jana chahiye ki 200ms na to "memory se data laana" hai (100 ns range), na "same-datacenter call" hai (~0.5 ms range) — ye scale hi "cross-region network call" ya "disk seek + queueing" jaisi cheez ka signature hai. Ye table hi wo mental model hai jisse system design interviews mein "estimate the latency of this design" jaise sawaal solve hote hain, aur production mein "kya slow hai" ka pehla guess milta hai bina kisi profiler ke.

**Kaise kaam karta hai:** Har layer ka time physically alag reason se aata hai:
- **L1/L2 cache reference** (~1–4 ns): CPU ke andar hi, electrical signal ko chip ke andar chhoti distance travel karni padti hai.
- **Branch mispredict** (~3 ns): CPU ne wrong instruction path speculatively execute kar li thi, ab pipeline flush karke sahi path se restart karna padta hai.
- **Mutex lock/unlock uncontended** (~17 ns): kernel involve nahi hota (userspace futex fast path), bas ek atomic CPU instruction (`CMPXCHG` jaisa kuch) chalta hai.
- **Main memory (RAM)** (~100 ns): CPU se memory controller tak signal jaake, DRAM row activate hoke data wapas aana — physically door hai chip ke bahar.
- **SSD random read (NVMe)** (~50–150 µs): PCIe bus ke through command jata hai, flash controller internally translate karta hai, flash cell se data aata hai — sab electronic hai, mechanical nahi, isiliye HDD se ~100x fast.
- **Round trip same datacenter** (~0.5 ms): NIC → switch → NIC, kernel network stack ke through, physically metres ki distance.
- **HDD seek** (~5–10 ms): ye **mechanical** hai — disk head ko physically move karke sahi track par pahunchna padta hai, phir platter ghoom kar sahi sector aane tak wait karna padta hai.
- **Cross-region/cross-continent RTT** (60–200 ms): speed of light in fibre ka hard limit — isko koi bhi software optimize nahi kar sakta (dekho Subtopic #1 neeche).

**Real-world aur backend/system-design example:** Socho ek engineer complain kar raha hai ki "Redis cache hit hone ke baad bhi response 80ms le raha hai, cache to RAM hai na, 1ms se kam lagna chahiye!" Is table se turant pata chalta hai — Redis khud RAM se data ~microseconds mein deta hai, lekin 80ms matlab kahin **network round trip + queueing** ho raha hai — maybe Redis ek doosre AWS availability zone mein hai (cross-AZ call ~1-2ms nahi, agar cross-region hai to 60-100ms+), ya connection pool exhaust ho gaya aur request queue mein wait kar rahi thi. Table ka use "kya normal hai" filter ki tarah hota hai — tumhe production dashboard dekhte hi pata chal jata hai ki number "cache-tier" hai ya "network-tier" hai ya "disk-tier" hai.

Poora reference table (2020s hardware, order-of-magnitude — exact digits nahi, **shape** yaad rakho):

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

"Human-scale" column ka trick simple hai — agar 1 ns ko 1 second treat kar do (roughly 1 billion ka multiplier), to poori table "insaan ki zindagi ke timescale" par map ho jati hai. Isi se instantly dikh jata hai ki L1 cache aur HDD seek ke beech ka gap utna hi bada hai jitna "1 second" aur "2-4 months" ke beech — ye visualization hi is table ka asli power hai.

```text
# "Human-scale" conversion — bas ek scaling factor hai (~1e9), koi magic nahi
scale_factor = 1_000_000_000   # 1 ns -> 1 s treat karo

for each row in latency_table:
    human_seconds = row.time_in_ns * scale_factor
    print(row.operation, "->", format_as_duration(human_seconds))

# Example hand-calc:
# main_memory_ns = 100
# human_seconds  = 100 * 1e9 ns = 1e11 ns = 100 seconds... wait, unit check:
# 100 ns * 1e9 (scale) = 1e11 ns = 100 s? No — scale_factor khud "1 ns -> 1 s" hai,
# to 100 ns ka matlab hai "100 s" ~= 1.7 minutes. Yehi table mein likha hai.
```

**Quick summary:** Ye table exact numbers ke liye nahi, **relative order-of-magnitude gaps** yaad rakhne ke liye hai — L1 cache se RAM tak ~100x, RAM se SSD tak ~1000x, SSD se HDD tak ~100x, aur same-datacenter se cross-continent tak ~100-1000x. Ye hi mental ruler hai jisse tum kisi bhi "ye latency normal hai kya" sawaal ka turant answer de sakte ho.

**Practice task:**
- Bina table dekhe, khud se bolke batao: "RAM se data lena SSD se kitna guna fast hai, roughly?" (answer: ~500-1000x). Fir check karo.
- Apne laptop par `ping localhost` aur `ping 8.8.8.8` chalao, dono ke numbers ko is table ke "same datacenter RTT" aur "cross-region RTT" rows se compare karo.

---

### 2. Converting latency into throughput and vice versa

**Kya hai:** **Latency** ka matlab hai — ek single request ko complete hone mein kitna time lagta hai (e.g. 50ms). **Throughput** ka matlab hai — system kitne requests per second handle kar sakta hai (e.g. 1000 req/s). Ye dono independent metrics nahi hain — inke beech ek precise mathematical relationship hai jise **Little's Law** kehte hain (queueing theory se aata hai, originally John Little ne 1961 mein prove kiya):

**L = λ × W**

Jahan **L** = system mein average concurrent items (in-flight requests / concurrency), **λ** (lambda) = arrival rate ya throughput (requests/sec), aur **W** = average time ek item system mein bitata hai (latency). Isko rearrange karke:

**Throughput (λ) = Concurrency (L) / Latency (W)**
**Concurrency (L) = Throughput (λ) × Latency (W)**

**Kyun important hai:** Ye formula batata hai ki throughput badhane ke sirf **do** tarike hain — ya to **latency kam karo** (har request jaldi complete karo), ya **concurrency badhao** (ek saath zyada requests parallel handle karo — zyada threads, zyada connections, zyada worker processes). Ye ek bahut common galti explain karta hai: engineers sochte hain "throughput kam hai, latency toh theek hai" — lekin agar concurrency (thread pool size, connection pool size) fixed/capped hai, to latency badhne se throughput **automatically** gir jayega, chahe CPU idle ho. Ye connection pool sizing, thread pool sizing, aur autoscaling policies design karne ka fundamental tool hai.

**Kaise kaam karta hai:** Ek analogy socho — ek toll booth par cars ki queue. Agar booth par ek saath 10 cars ho sakti hain (concurrency=10), aur har car ko process karne mein 2 seconds lagte hain (latency=2s), to throughput = 10/2 = 5 cars/sec. Ab agar kisi din booth operator slow ho jaye aur har car ko 4 seconds lage (latency double), lekin booth mein sirf 10 hi cars fit ho sakti hain (concurrency same rahi), to throughput gir kar 10/4 = 2.5 cars/sec ho jayega — **aadha**. Yehi backend server mein hota hai: agar ek downstream DB query slow ho jaye (latency badh gayi) aur tumhara HTTP server ka thread pool ya DB connection pool fixed size ka hai (concurrency capped), to poore system ka throughput automatically gir jayega — naye requests queue mein wait karenge, phir timeout honge.

**Real-world aur backend/system-design example:** **HikariCP** (Java ka popular connection pool) ki official sizing guide isi formula par based hai — wo bolti hai pool size ko bahut bada mat rakho, balki `connections = threads * (latency-based formula)` se calculate karo, kyunki zyada connections sirf context-switching overhead badhate hain, throughput nahi. **Nginx** ka `worker_connections` setting bhi isi tarah kaam karta hai — agar backend latency badh jaye (slow upstream), to fixed `worker_connections` ke saath throughput cap ho jata hai aur naye connections `502`/`504` errors dene lagte hain. Isi wajah se **Kubernetes HPA (Horizontal Pod Autoscaler)** ko design karte waqt engineers latency aur concurrency dono metrics dekhte hain, sirf CPU nahi — kyunki agar downstream latency spike ho jaye, existing pods "busy but low-CPU" ho sakte hain (waiting on I/O), aur naive CPU-based autoscaling scale hi nahi karega jab zarurat ho.

```text
# Worked example — Little's Law se capacity planning

# Target: 5000 requests/sec handle karne hain
# Average latency per request (normal case): 50 ms = 0.05 s

required_concurrency = throughput * latency
required_concurrency = 5000 req/s * 0.05 s
required_concurrency = 250   # ~250 concurrent in-flight requests chahiye
                              # (thread pool / DB connection pool >= 250)

# Ab socho DB query slow ho gayi (index missing, table scan) — latency 50ms se 200ms ho gayi
# lekin connection pool size 250 par hi fixed hai (nahi badla):

max_throughput_now = concurrency / latency
max_throughput_now = 250 / 0.2
max_throughput_now = 1250 req/s   # 5000 se seedha 1250 pe gir gaya — 75% capacity loss!

# Symptom production mein: "HikariPool-1 - Connection is not available,
# request timed out after 30000ms" ya Go mein http.Client "context deadline exceeded"
# — root cause dikhta hai "pool exhaustion", asli cause hai "downstream latency spike"
```

**Quick summary:** Throughput, concurrency, aur latency teeno ek triangle hain — kisi ek ko fix karke doosre do ka trade-off hota hai. Latency badhne se, fixed concurrency par, throughput seedha proportionally girta hai — ye sabse common "mysterious throughput drop" ka root cause hai production mein.

**Practice task:**
- Apni kisi API ka average latency aur target throughput nikaalo, Little's Law se calculate karo ki thread/connection pool kitna bada hona chahiye.
- Explain out loud: "agar ek downstream service ka p99 latency 10x badh jaye lekin humara connection pool size same rahe, to kya hoga?" — bina notes dekhe answer do.

---

### Subtopics — practical aha points

**Speed of light ek hard architectural constraint hai:** Fibre optic cable mein light ki speed vacuum ke ~2/3 hoti hai (refractive index ki wajah se), roughly **~200,000 km/s**, yani ~5 microseconds per km. Iska matlab hai — Mumbai se US-East (Virginia) tak ki ~13,500 km ki distance ke liye, sirf **one-way** signal travel karne mein hi ~67ms lagega, round trip ~135ms — aur ye ek **hard physics limit** hai, koi bhi software optimization, koi bhi bada server, koi bhi CDN isko cross nahi kar sakta. Isi wajah se system design mein bola jata hai "Sydney kabhi bhi US-East ke ek write ko 70ms se kam mein nahi dekh sakta" — chahe network perfect ho, zero congestion ho. Ye fact hi **CAP theorem** ki practical wajah hai, aur Google Spanner ka **TrueTime commit-wait** isi limit ko explicitly handle karne ke liye design hua tha. Backend decision: agar users globally distributed hain, to single global database rakhna latency-wise impossible hai — isiliye **regional read replicas**, **multi-region active-active setups** (DynamoDB Global Tables, CockroachDB), ya **edge caching** (CloudFront, Cloudflare) use hote hain — data ko user ke paas laana hi ek tareeka hai is physics limit ko "kaam around" karne ka.

```bash
# Real-world measurement — Mumbai se US-East AWS endpoint tak actual RTT dekho
ping -c 5 s3.us-east-1.amazonaws.com
# Expect: ~230-250ms actual (theoretical minimum ~135ms se kaafi zyada —
# routing hops, peering, undersea cable ka non-straight-line path sab add hota hai)
```

**Bandwidth vs latency vs throughput vs goodput vs IOPS — chaar alag bottlenecks:** Ye terms aksar confuse hote hain lekin har ek **alag cheez** measure karta hai. **Bandwidth** = link ki raw capacity (kitna data theoretically bhej sakte ho per second, jaise `1 Gbps`). **Latency** = ek single unit of data ko travel karne mein kitna time lagta hai (one-way ya round-trip). **Throughput** = actually kitna data per second successfully transfer hua (real-world, bandwidth se kam hota hai overhead ki wajah se). **Goodput** = throughput mein se sirf "useful application data" — protocol headers, retransmissions, ACKs minus karke (jaise TCP retransmit hua packet throughput mein count hota hai lekin goodput mein nahi). **IOPS** (I/O Operations Per Second) = storage-specific metric — kitne discrete read/write operations per second ho sakte hain, chahe har operation chhota ho (4KB) ya bada. Highway analogy: bandwidth = kitni lanes hain, latency = ek car ko cross karne mein kitna time, throughput = actually kitni cars/sec pass hui, goodput = usme se kitni cars mein "useful cargo" tha (na ki empty trucks/retries). Ek **geostationary satellite link** classic example hai — bandwidth bahut high ho sakta hai (Mbps-Gbps), lekin latency ~500-600ms hai (36,000 km up-down), jo high-bandwidth-high-latency combination naive intuition se ulta lagta hai.

```bash
# Bandwidth/throughput measure karne ke liye
iperf3 -c iperf.server.example.com          # network throughput test

# Storage IOPS measure karne ke liye (random 4K reads)
fio --name=randread --rw=randread --bs=4k --size=1G --numjobs=4 --runtime=30 --time_based
```

**Latency budgets — 200ms page budget ko allocate karna:** System design mein pura end-to-end latency budget ek "budget" ki tarah treat hota hai — jaise money budget mein har department ko ek slice milta hai. Agar target hai ki page 200ms mein load ho, to ye 200ms **DNS resolution** (~10-20ms, cold), **TLS handshake** (2 × RTT — agar RTT 30ms hai to ~60ms), **API gateway/load balancer routing**, **6 microservices ki internal calls** (kuch parallel, kuch sequential), **2 database queries**, aur **client-side render** — sabke beech baant diya jata hai. Agar koi ek layer apna slice overspend kare (jaise ek DB query jo expected 10ms ki jagah 80ms le rahi hai), to poora budget bust ho jata hai aur user ko slow page dikhta hai. Ye concept hi **distributed tracing** (Jaeger, Zipkin, AWS X-Ray, OpenTelemetry) ka poora purpose hai — waterfall diagram mein dikhana ki 200ms budget ka kaunsa hissa kahan gaya. Amazon ka famous internal finding tha ki **har 100ms extra latency ~1% sales revenue** kam kar deta hai — isi wajah se latency budgeting sirf engineering nahi, business-critical practice hai.

```bash
# curl se ek request ka timing breakdown dekho — DNS, connect, TLS, TTFB, total
curl -o /dev/null -s -w \
  "dns:%{time_namelookup}s connect:%{time_connect}s tls:%{time_appconnect}s ttfb:%{time_starttransfer}s total:%{time_total}s\n" \
  https://example.com
```

---

### Advanced concepts

**Latency ek *distribution* hai, kabhi mean nahi — p50/p90/p99/p99.9/p99.99:** Production mein latency ek single number nahi hota, wo ek **distribution** hota hai — kuch requests fast, kuch bahut slow. **p50 (median)** matlab 50% requests isse fast the. **p99** matlab sirf 1% requests isse slow the — yani agar tumhare paas 1 million requests/day hain, to p99 latency wo hai jo **10,000 requests** ko face karni padi (bilkul chhota fraction nahi!). **Mean/average** is distribution ko chhupa deta hai — agar 99 requests 10ms lein aur ek request 10 seconds le (GC pause, lock contention, page fault), mean sirf ~110ms dikhayega jabki asli problem (10-second outlier) invisible ho jata hai. Isi wajah se production SLOs **kabhi mean par set nahi hote**, hamesha p99/p99.9/p99.99 par. Tools jaise **HdrHistogram** (High Dynamic Range Histogram, Gil Tene dwara banaya gaya) is precision ke saath percentiles track karte hain bina memory explode kiye — Datadog, Prometheus histograms, Gatling, wrk2 sab isi concept par based hain.

```text
# Percentile calculate karne ka basic mechanism
latencies_ms = sorted([...collected samples...])   # sort karna zaroori hai
n = len(latencies_ms)

p50_index  = int(0.50 * n)
p99_index  = int(0.99 * n)
p999_index = int(0.999 * n)

p50  = latencies_ms[p50_index]
p99  = latencies_ms[p99_index]
p999 = latencies_ms[p999_index]

# Note: chhote sample sizes (n=100) par p99 essentially "max" hai — noisy!
# Reliable p99 ke liye kam se kam kuch hazaar samples chahiye.
```

**Fan-out amplification — tail-at-scale problem:** Jab ek request internally **N parallel sub-calls** karta hai (scatter-gather — jaise Elasticsearch multiple shards ko query karna, ya ek dashboard jo 100 microservices ko parallel call kare), to poore request ka latency us **sabse slowest sub-call** se decide hota hai. Agar har individual call ka p99 = 1% (yani 1% chance slow hone ki), aur N=100 parallel calls hain, to probability ki **kam se kam ek** call slow ho: **1 − (0.99)^100 ≈ 63%**. Matlab jo pehle sirf "1% requests ko affect karta tha," wo ab **63% requests** ko affect karta hai — tumhare overall request ka p99 ab ek single call ke **p63** jaisa behave karta hai! Ye phenomenon **"The Tail at Scale"** paper (Jeffrey Dean & Luiz André Barroso, Google, 2013) mein formally describe hua hai. Mitigations: **hedged/backup requests** (duplicate request bhejo agar original ek threshold time tak respond na kare, jo bhi pehle aaye use lo — Google search backend isi se apna tail latency control karta hai), fan-out width kam karna, per-sub-request timeouts + jittered retries, aur circuit breakers (**Envoy**, **Hystrix**-style patterns) taaki ek slow shard poore request ko block na kare.

```text
# Fan-out amplification ka arithmetic — N badhne se probability kaise badhti hai
single_call_p99 = 0.01   # 1% chance ek call slow hone ka

for N in [1, 10, 50, 100, 1000]:
    prob_at_least_one_slow = 1 - (1 - single_call_p99) ** N
    print(f"N={N}: at least one slow call probability = {prob_at_least_one_slow:.2%}")

# Output roughly:
# N=1:    1.00%
# N=10:   9.56%
# N=50:   39.50%
# N=100:  63.40%   <- classic "tail at scale" number
# N=1000: 99.99%   <- almost guaranteed ek call slow hoga
```

**Coordinated omission — tumhara load test ka p99 jhooth bol raha hai:** Ye term **Gil Tene** ne coin kiya tha. Zyadatar simple load-testing tools (jaise plain `ab` ya default JMeter thread-group mode) **closed-loop** hote hain — wo agla request tabhi bhejte hain jab pichhle request ka response aa jaye. Problem ye hai: agar server 5 seconds ke liye stall ho jaye (GC pause, deploy, DB failover), to closed-loop tool bhi 5 seconds ke liye **naye requests bhejna band** kar deta hai — jabki real production mein **real users** us 5-second window mein bhi naye requests bhejte rehte hain (unhe pata nahi ki server stall hai), aur wo sab requests bahut lambi queueing delay experience karte hain jo tumhare test mein **kabhi measure hi nahi hui**. Result: tumhara measured p99 optimistic (fake-good) hota hai, real users ka experience usse kaafi bura hota hai. Fix: **open-loop load generators** use karo jo fixed schedule par requests bhejte hain chahe pichla response aaya ho ya nahi — jaise **wrk2** (wrk ka fork jo isi problem ko fix karta hai), **k6** ka `arrival-rate` executor, ya **Gatling** ka open injection profile — ye tools "intended send time" vs "actual completion time" dono track karte hain taaki queueing delay bhi latency mein count ho.

```bash
# wrk (closed-loop, coordinated omission ka shikar) —
# agla request tabhi jaata hai jab connection free ho
wrk -t4 -c100 -d30s http://api.example.com/endpoint

# wrk2 (open-loop, fixed throughput schedule — coordinated omission se safe) —
# --rate fixed requests/sec try karta hai, chahe responses slow aa rahe hon
wrk2 -t4 -c100 -d30s -R2000 http://api.example.com/endpoint
```

---

### Important terms

- **RTT (Round-Trip Time):** Ek request bhejne aur uska response wapas aane tak ka total time — one-way latency ka double (roughly).
- **Latency:** Ek single operation/request ko complete hone mein lagne wala time.
- **Throughput:** Per unit time kitna kaam (requests, bytes, transactions) complete hua — jaise req/sec.
- **Bandwidth:** Link/channel ki raw theoretical data-carrying capacity.
- **Goodput:** Throughput minus overhead (retransmissions, protocol headers) — sirf "useful" data ka rate.
- **IOPS:** I/O Operations Per Second — storage devices ke liye discrete read/write operation rate.
- **Little's Law:** `Concurrency = Throughput × Latency` — queueing theory ka fundamental relationship.
- **Percentile (p50/p90/p99/p99.9):** Distribution mein wo value jisse X% samples chhote/fast the.
- **Tail latency:** Distribution ke extreme slow end ke latencies (p99, p99.9 wagera).
- **Mean vs median:** Mean outliers se easily skew ho jata hai; median (p50) zyada robust hai skewed distributions ke liye.
- **Fan-out:** Ek incoming request ko multiple parallel downstream calls mein split karna (scatter-gather).
- **Tail-at-scale:** Jab fan-out badhta hai, poore request ka latency single sub-call ke tail se decide hota hai.
- **Hedged/backup request:** Duplicate request bhejna agar original threshold time tak respond na kare, taaki tail latency control ho.
- **Coordinated omission:** Closed-loop load-testing tools ka measurement bias jo slow periods ke during naye requests bhejna band kar dete hain, real queueing delay ko under-report karte hain.
- **Closed-loop vs open-loop load testing:** Closed-loop = agla request pichle response ke baad; open-loop = fixed schedule par requests, response ka wait nahi karta.
- **HdrHistogram:** Ek data structure jo bahut wide latency range (ns se seconds tak) ke percentiles accurately, low-memory footprint mein track karta hai.
- **Refractive index:** Material property jo batata hai light us material (jaise fibre-optic glass) mein vacuum se kitni slow travel karega — isi wajah se fibre mein light ~200,000 km/s hai, na ki ~300,000 km/s.
- **Latency budget:** Ek end-to-end target latency (jaise 200ms) ko individual components/hops ke beech pre-allocate karna.
- **Distributed tracing:** Ek request ka poora path multiple services ke through track karna (Jaeger, Zipkin, OpenTelemetry) taaki latency budget breakdown dikh sake.
- **Jitter:** Latency mein variability/inconsistency — do requests same operation ke liye bhi alag-alag time le sakte hain.

---

### Common mistakes

- Dashboards aur SLAs mein **mean/average latency** report karna instead of p99/p99.9 — ek outlier-heavy distribution mein mean hamesha "sab theek hai" dikhata hai jabki real users tail latency face kar rahe hote hain.
- **Straight-line distance** se RTT estimate karna, actual fibre routing (undersea cables, peering points, ISP hops) ignore karke — real RTT theoretical minimum se aksar 1.5-2x zyada hota hai.
- Load testing ke liye **closed-loop tools** (default `ab`, naive JMeter setup) use karna aur unke p99 numbers ko production SLA ke liye trust kar lena — coordinated omission ki wajah se ye numbers optimistic hote hain.
- Scatter-gather/fan-out design karte waqt fan-out width badhate jaana bina ye socha ki tail latency probability exponentially worse hoti jaati hai (N=100 par 1% single-call tail = 63% request-level tail).
- **TLS handshake aur DNS resolution time** ko latency budget mein completely bhool jaana — assume kar lena ki request = "1 RTT," jabki naya HTTPS connection kam se kam 3-4 RTT le sakta hai (DNS + TCP handshake + TLS 2-RTT).
- Throughput drop dekh kar seedha "zyada servers add karo" bolna, bina ye check kiye ki concurrency (thread pool/connection pool) already capped hai — Little's Law se pehle diagnose karna chahiye ki bottleneck latency hai ya concurrency limit.
- High-latency cross-region links par **bandwidth-delay product** ignore karna — TCP window size chhota hone se, bandwidth available hone ke bawajood actual throughput bahut kam milta hai (fix: window scaling, parallel streams, ya protocols jaise QUIC).
- Chhote sample size (jaise sirf 100 requests) ke saath p99 report karna — statistically ye almost "max" ke barabar hai, bahut noisy aur misleading, reliable p99 ke liye hazaron samples chahiye.

---

### Interview questions

1. Latency table ke exact numbers yaad rakhne ki jagah "order of magnitude" kyun important hai? — *Hardware har saal badalta hai but relative gaps (RAM vs SSD vs network) stable rehte hain; yehi gaps back-of-envelope estimation ka base hain.*
2. Do datacenters X km door hain — theoretical minimum RTT kaise calculate karoge, aur real RTT usse zyada kyun hoga? — *Distance / (~200,000 km/s fibre speed) × 2 for round trip; real routing straight-line nahi hota, hops/peering/congestion add hota hai.*
3. Little's Law derive/explain karo aur ek connection pool size calculate karo given throughput aur latency. — *Concurrency = Throughput × Latency; formula seedha apply karo, phir bolo latency badhne se fixed pool par throughput kaise girta hai.*
4. p99 latency mean se zyada important kyun hai SLAs mein? — *Mean outliers ko average kar deta hai; p99 batata hai worst-affected users ka real experience, jo business-critical hota hai (heavy users repeatedly tail hit karte hain).*
5. Tail-at-scale/fan-out amplification kya hai? 100 parallel calls ke saath single-call p99 request-level kya ban jata hai, aur kaise? — *1-(0.99)^100 ≈ 63% — mitigations: hedged requests, reduce fan-out, per-call timeouts.*
6. Coordinated omission kya hai, aur wrk vs wrk2 mein kya fark hai? — *Closed-loop tools slow period mein naye requests bhejna rok dete hain, real queueing delay miss ho jaata hai; wrk2/open-loop fixed schedule maintain karta hai.*
7. Bandwidth, latency, throughput, goodput, aur IOPS mein fark batao, har ek ka ek real scenario do. — *Satellite link: high bandwidth, high latency; retransmission-heavy TCP: throughput high but goodput low; SSD: high IOPS.*
8. Ek 200ms API latency budget banao — DNS, TLS, downstream services, DB, render — kis order mein cut karoge agar over-budget ho? — *Pehle sabse bada/avoidable slice dekho (extra DB round trips, sequential-instead-of-parallel service calls), phir TLS resumption/keep-alive jaisi cheap wins.*
9. Sydney US-East ka write 70ms se pehle kyun nahi dekh sakta? Ye CAP theorem se kaise juda hai? — *Speed of light hard limit hai; strong consistency chahiye to wait karna padega ya eventual consistency accept karni padegi — CAP mein Partition tolerance ke saath Consistency vs Availability trade-off isi physics se emerge hota hai.*
10. Jitter aur latency mein kya fark hai, aur real-time systems (video call, gaming) mein jitter kyun latency se bhi zyada matter karta hai? — *Latency = kitna time laga; jitter = time mein variability — consistent 100ms better hai than fluctuating 20-200ms, kyunki real-time buffers ko worst-case ke liye size karna padta hai.*

---

### Hands-on lab

> Compute the theoretical minimum latency for a user in Mumbai calling a service in Virginia, then measure it.

**Ye specific lab kyun:** Ye lab Topic 1 aur Subtopic #1 (speed of light constraint) ko seedha empirical bana deta hai. Tumhe pehle ek **theoretical floor** calculate karna hai (great-circle distance ÷ speed of light in fibre × 2 for round trip), aur phir ek **real measurement** (`ping`/`curl`) lekar dekhna hai ki actual RTT theoretical minimum se kitna zyada hai — ye gap hi batata hai ki real-world routing, peering, aur congestion overhead kitna add karte hain. Ye ek chhota exercise hai lekin iska result tumhare pure career mein baar-baar reference banega — jab bhi koi multi-region architecture design karoge, "kam se kam ye latency lagegi hi" ka baseline yehi calculation dega.

- [ ] Mera result: Theoretical minimum RTT (calculated) = ______ ms, Actual measured RTT = ______ ms, Difference/overhead = ______ ms (______ %)

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.3 Data structures & algorithms as design tools

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har topic ke saath: **kya hai → kyun important hai (fayda) → kaise kaam karta hai (mechanism) → real-world/backend example.**

### 1. Arrays, dynamic arrays, linked lists, stacks, queues, deques, ring buffers

**Kya hai:** Ye sabse basic data structures hain, lekin inka "kya hai" sirf textbook definition nahi — inka memory layout hi inki real personality hai. **Array** ek contiguous (lagatar) block hai memory mein — fixed size, index se O(1) access. **Dynamic array** (Go ka `slice`, Java ka `ArrayList`, C++ ka `std::vector`, Python ka `list`) array ke upar ek growable wrapper hai — jab capacity khatam ho jati hai, ek naya bada block allocate hota hai (usually 1.5x ya 2x size) aur purana data copy ho jata hai. **Linked list** har element ko alag se heap par allocate karta hai aur pointer se connect karta hai — memory mein bikhra hua (scattered). **Stack** (LIFO) aur **queue** (FIFO) access-pattern constraints hain, kisi bhi underlying structure (array ya linked list) par implement ho sakte hain. **Deque** (double-ended queue) dono end se O(1) push/pop deta hai. **Ring buffer** (circular buffer) ek fixed-size array hai jisme do pointers — `head` aur `tail` — ghoomte rehte hain, wraparound ke saath.

**Kyun important hai:** Interview mein "array vs linked list" sirf Big-O ka sawaal nahi hai — production mein ye decision cache behavior, GC pressure, aur throughput directly decide karta hai. Ring buffers to har high-throughput system (network stacks, message queues, audio/video pipelines, log buffers) ke core mein baithe hote hain kyunki unme **no allocation** hoti hai steady state mein — jo GC-heavy languages (Java, Go) mein latency spikes avoid karne ke liye critical hai.

**Kaise kaam karta hai:** Dynamic array ka growth **amortized doubling strategy** use karta hai — jab full ho jaye to naya array size × 2 allocate karo, purana copy karo, purana free karo. Isse average insert cost O(1) rehta hai even though kabhi-kabhi ek insert O(n) copy trigger karta hai (ye "amortized vs worst-case" ka core idea hai — subtopics mein aage detail hai). Ring buffer mein `head` write position hai, `tail` read position — jab `head == tail` to buffer empty ya full hai (disambiguate karne ke liye ek extra flag ya ek slot khaali rakha jata hai). Producer `head` badhata hai, consumer `tail` badhata hai — dono modulo capacity wrap karte hain.

**Real-world example aur backend/system-design connection:** Kafka ka producer-side batching buffer aur Linux kernel ke network device drivers ke RX/TX rings dono ring buffers hain — fixed memory, zero runtime allocation, predictable latency. Backend design mein: jab aap ek in-memory rate limiter ya recent-events buffer bana rahe ho ("last 1000 requests ka log rakho"), ring buffer ek natural choice hai kyunki tumhe bounded memory chahiye aur allocation-free hot path chahiye — jabki ek naive `ArrayList`/`slice` jo hamesha `append` karta rahe (bina bound ke) memory leak aur unpredictable GC pause ka risk banata hai.

```c
// Simple fixed-capacity ring buffer (SPSC-style), C
#define CAP 1024
typedef struct {
    int buf[CAP];
    size_t head; // next write index
    size_t tail; // next read index
} RingBuffer;

int rb_push(RingBuffer *rb, int val) {
    size_t next_head = (rb->head + 1) % CAP;
    if (next_head == rb->tail) return -1; // full
    rb->buf[rb->head] = val;
    rb->head = next_head;
    return 0;
}

int rb_pop(RingBuffer *rb, int *out) {
    if (rb->tail == rb->head) return -1; // empty
    *out = rb->buf[rb->tail];
    rb->tail = (rb->tail + 1) % CAP;
    return 0;
}
```

**Quick summary:** Array = contiguous + cache-friendly; linked list = flexible but scattered; ring buffer = bounded, allocation-free, high-throughput ka pasandida structure.

**Practice task(s):**
- Apne alfaazon mein explain karo (bolke): kyun ek 10-million-element array ko traverse karna usi size ki linked list se kai guna fast hota hai, jabki dono O(n) hain.
- Ek chhota experiment: apni language mein ek dynamic array mein 1 million elements `append` karo aur time measure karo; phir capacity ko pehle hi `reserve`/pre-allocate karke same test chalao. Dono times compare karo.

---

### 2. Hash tables: hashing, collisions (chaining vs open addressing), load factor, resizing

**Kya hai:** Hash table ek key → value mapping hai jo average O(1) lookup deta hai. Ek **hash function** key ko ek integer (hash code) mein convert karti hai, jo phir `mod tableSize` karke ek **bucket index** deta hai. **Collision** tab hoti hai jab do alag keys same bucket mein map ho jaayein. Do main collision-resolution strategies hain: **chaining** (har bucket ek linked list/small array hoti hai jisme sab colliding entries pad jaati hain) aur **open addressing** (agar bucket full hai to ek fixed probing sequence — linear, quadratic, ya double hashing — follow karke agla khaali slot dhoondo, sab kuch same array ke andar). **Load factor** = `n / tableSize` (kitne elements bhare hain table ke against capacity) — jab ye ek threshold (jaise 0.75) cross karta hai, table **resize** (usually double) hoti hai aur saari entries **rehash** hoke naye table mein daali jaati hain.

**Kyun important hai:** Hash tables har jagah hain — language-level maps (`dict`, `map`, `HashMap`), database indexes (hash indexes), caches (LRU cache internally ek hashmap + doubly linked list hoti hai), aur distributed systems (partitioning). Lekin naive resize implementation ek production landmine hai: jab table resize hoti hai, wo ek **stop-the-world pause** create kar sakti hai jahan puri table rehash ho rahi ho — ye tail latency (p99/p999) ko directly hit karta hai, average latency ko nahi (average acceptable rehta hai kyunki resize rare hai, lekin jab hota hai tab spike bada hota hai).

**Kaise kaam karta hai:** Chaining mein har lookup: `hash(key) % tableSize` → bucket → us bucket ki chhoti list ko linearly scan karo key match ke liye. Worst case (sab keys same bucket mein — adversarial hash collision attack) O(n) ho sakta hai, isliye production hash tables **randomized hash seed** use karte hain (jaise Python, Go, aur many languages "hash flooding" DoS attacks se bachne ke liye per-process random seed rakhte hain). Open addressing mein deletion tricky hota hai — slot ko simply empty mark nahi kar sakte (kyunki probing sequence break ho jayegi), isliye ek **tombstone** marker use karte hain. Resize: naya array (double size) allocate karo, purani saari entries ko naye hash function (naya modulo) se re-insert karo — ye O(n) operation hai jo poore table ko touch karta hai. Advanced implementations (Redis ka `dict`, Java's `ConcurrentHashMap`) **incremental rehashing** karte hain — ek hi resize ko turant complete karne ke bajaye, har subsequent operation ke saath thoda-thoda rehash karte hain, taaki ek hi call mein bada pause na aaye.

**Real-world example aur backend/system-design connection:** Redis internally apne main keyspace ke liye ek hash table use karta hai, aur bilkul isi wajah se **incremental rehashing** implement karta hai — do hash tables (`ht[0]` aur `ht[1]`) simultaneously rakhta hai resize ke dauran, aur har command ke saath thoda data migrate karta hai, taaki Redis (jo single-threaded event loop hai) ek bade rehash ki wajah se sab clients ko block na kar de. Backend design mein: agar tum apna khud ka in-memory cache likh rahe ho high-QPS service ke liye, "resize threshold" aur "resize cost" dono ka andaza hona chahiye — warna ek innocent-looking `map[key]value` production mein periodic latency spikes de sakta hai jab wo grow karta hai.

```c
// Open addressing with linear probing — simplified insert
#define CAP 16
typedef struct { char *key; int used; int tombstone; } Slot;
Slot table[CAP];

unsigned hash(const char *key) {
    unsigned h = 2166136261u;
    for (; *key; key++) { h ^= (unsigned char)*key; h *= 16777619u; }
    return h;
}

void insert(const char *key) {
    unsigned idx = hash(key) % CAP;
    for (int i = 0; i < CAP; i++) {
        unsigned probe = (idx + i) % CAP; // linear probing
        if (!table[probe].used || table[probe].tombstone) {
            table[probe].key = strdup(key);
            table[probe].used = 1;
            table[probe].tombstone = 0;
            return;
        }
    }
    // table full — caller should resize before this happens
}
```

**Quick summary:** Hash tables amortized O(1) hain, lekin resize/rehash ek chhupa hua O(n) tail-latency cost hai — isi wajah se production systems incremental rehashing karte hain.

**Practice task(s):**
- Bolke explain karo: chaining aur open addressing mein deletion ka handling alag kyun hota hai, aur tombstone ki zaroorat kyun padti hai.
- `strace` (ya apni language ka profiler) se ek chhoti script chalao jo ek map mein lakhon entries insert kare, aur dekho ki insert time ek smooth line hai ya periodic spikes dikhata hai (resize points par).

---

### 3. Trees: BST, balanced trees (AVL/red-black), B-tree/B+tree, tries, heaps, skip lists

**Kya hai:** **Binary Search Tree (BST)** ek tree hai jahan har node ke left subtree mein chhoti values aur right subtree mein badi values hoti hain — lookup, insert, delete average O(log n) hain lekin agar tree skewed ho jaye (jaise sorted data insert karne se) to worst-case O(n) ban jata hai. **Balanced trees** (AVL, Red-Black) automatically rotations karke height ko O(log n) guarantee karte hain chahe insertion order kuch bhi ho. **B-tree/B+tree** ek generalization hai jahan har node do nahi, balki **many children** rakh sakta hai (high fanout) — disk-oriented databases ke liye design kiya gaya. **Trie** (prefix tree) strings ko character-by-character path ke roop mein store karta hai — common prefixes share hote hain. **Heap** (binary heap) ek array-based complete tree hai jo hamesha min (ya max) ko root par rakhta hai — O(log n) insert/extract. **Skip list** ek linked-list-based probabilistic structure hai jisme "express lanes" hoti hain multiple levels par, jo O(log n) expected search deta hai bina rotations ke.

**Kyun important hai:** Ye structures database internals, in-memory caches, schedulers, aur autocomplete jaise features ke andar chhupe hote hain — inhe samajhna matlab hai "database index slow kyun hai" ya "priority queue kis se implement karoon" jaise real decisions le paana. B+tree ka fanout samajhna specifically Level 3 (storage engines) ke liye foundational hai.

**Kaise kaam karta hai:** AVL tree har insert/delete ke baad node heights check karta hai — agar left aur right subtree heights ka difference 1 se zyada ho jaye, to **rotation** (single ya double) karke tree ko rebalance karta hai; Red-Black tree similar guarantee deta hai lekin looser balancing (colors: red/black nodes) ke through — kam rotations lagte hain isliye writes thodi cheaper hoti hain AVL se, reads thodi slower (AVL zyada strictly balanced hota hai). B+tree mein saari actual data values sirf **leaf nodes** mein hoti hain, aur leaves ek linked list ki tarah connected hoti hain (range scans ke liye) — internal nodes sirf routing keys rakhte hain, jisse ek hi node mein sainkadon children fit ho jaate hain (disk page = node, high fanout = kam height = kam disk seeks). Heap array mein store hota hai (`parent = i/2`, `children = 2i, 2i+1`), insert par naye element ko end mein daal ke **sift-up** karte hain (parent se compare karke swap), extract-min par root ko last element se replace karke **sift-down** karte hain.

**Real-world example aur backend/system-design connection:** Postgres ka default index type **B-tree** hai (`CREATE INDEX`), aur MySQL/InnoDB ka clustered index bhi B+tree hai jahan leaf nodes mein actual row data hoti hai. Linux kernel scheduler mein Completely Fair Scheduler (CFS) task selection ke liye red-black tree use karta hai (runnable tasks ko virtual-runtime se ordered rakhta hai). Job scheduler ya timer-wheel implementations aksar binary heap use karte hain "next event kab hai" jaldi nikaalne ke liye — Go ke `time.Timer` runtime ka internal implementation aur Kubernetes ka scheduler queue dono heap-jaisi priority structures use karte hain.

```c
// Binary min-heap: sift-up on insert (array-based)
void sift_up(int *heap, int i) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (heap[parent] <= heap[i]) break;
        int tmp = heap[parent]; heap[parent] = heap[i]; heap[i] = tmp;
        i = parent;
    }
}

void heap_insert(int *heap, int *size, int val) {
    heap[*size] = val;
    sift_up(heap, *size);
    (*size)++;
}
```

**Quick summary:** BST/AVL/Red-Black in-memory ordered structures hain; B+tree disk-optimized wide-fanout tree hai; heap priority extraction ke liye; trie/skip list apne specific use-cases (prefix match, probabilistic ordering) ke liye.

**Practice task(s):**
- Bolke explain karo: B+tree ka fanout zyada kyun rakha jata hai (ek node mein sainkadon children), jabki in-memory BST mein sirf 2 children hote hain.
- Ek chhoti coding exercise: apni pasandida language mein ek min-heap implement karo (insert + extract-min), aur usse ek "top-K smallest elements" problem solve karo.

---

### 4. Graphs: representations, BFS/DFS, topological sort, shortest path

**Kya hai:** Graph ek structure hai jo **nodes (vertices)** aur unke beech **edges (relationships)** represent karta hai — directed ya undirected, weighted ya unweighted. Do common representations: **adjacency list** (har node ke liye uske neighbors ki list — sparse graphs ke liye memory-efficient) aur **adjacency matrix** (n×n matrix, dense graphs ya O(1) edge-existence check ke liye). **BFS** (Breadth-First Search) level-by-level explore karta hai queue use karke — shortest path (unweighted graph mein, edge-count ke hisaab se) guarantee karta hai. **DFS** (Depth-First Search) ek path ko end tak jaata hai phir backtrack karta hai, stack (ya recursion) use karke. **Topological sort** ek DAG (Directed Acyclic Graph) ke nodes ko ek aisi linear order mein arrange karta hai ki har edge `u → v` mein `u` order mein `v` se pehle aaye. **Shortest path** algorithms (Dijkstra weighted non-negative edges ke liye, Bellman-Ford negative edges ke liye, BFS unweighted ke liye) minimum-cost path dhoondte hain.

**Kyun important hai:** Distributed systems mein bahut kuch literally graph hota hai — service dependency graphs, task DAGs (CI/CD pipelines, data pipelines), social network connections, network routing. "Deployment order kya hoga" ya "circular dependency hai ya nahi" jaise real production sawaal directly topological sort/cycle-detection ke sawaal hain.

**Kaise kaam karta hai:** BFS ek queue maintain karta hai — start node ko queue mein daalo, mark visited, phir loop: queue se node nikaalo, uske saare unvisited neighbors ko visited mark karke queue mein daalo. Isse guarantee milta hai ki jab tak level `k` ke saare nodes process nahi ho jaate, level `k+1` ka koi node process nahi hoga — isi wajah se BFS unweighted shortest path deta hai. Topological sort do tareekon se hota hai: **Kahn's algorithm** (in-degree 0 wale nodes ko ek queue mein daalo, process karo, unke neighbors ka in-degree ghataate jao, jo 0 ho jaaye use queue mein daalo — agar end mein saare nodes process nahi hue to cycle hai) ya **DFS-based** (post-order traversal ko reverse karo).

**Real-world example aur backend/system-design connection:** Kubernetes/Terraform jaise tools resource dependencies ko ek DAG maante hain aur apply/destroy order topological sort se decide karte hain. Build systems (Make, Bazel, npm/yarn dependency resolution) bhi topological sort use karte hain ye decide karne ke liye ki kaunsa module pehle build/install hoga. Distributed tracing (Jaeger/Zipkin) mein service call graph banane ke liye graph traversal use hota hai — kis service ne kisko call kiya, latency kahan accumulate hui. Network routing protocols (OSPF) shortest-path algorithms (Dijkstra-family) use karte hain.

```go
// BFS + simple cycle-aware topological sort (Kahn's algorithm), Go-flavoured pseudocode
func topoSort(graph map[string][]string) ([]string, bool) {
    inDegree := map[string]int{}
    for node := range graph { inDegree[node] = 0 }
    for _, neighbors := range graph {
        for _, n := range neighbors { inDegree[n]++ }
    }

    queue := []string{}
    for node, deg := range inDegree {
        if deg == 0 { queue = append(queue, node) }
    }

    order := []string{}
    for len(queue) > 0 {
        node := queue[0]; queue = queue[1:]
        order = append(order, node)
        for _, n := range graph[node] {
            inDegree[n]--
            if inDegree[n] == 0 { queue = append(queue, n) }
        }
    }

    hasCycle := len(order) != len(graph)
    return order, !hasCycle
}
```

**Quick summary:** Graphs relationships model karte hain; BFS unweighted shortest path deta hai, topological sort dependency ordering deta hai — dono real deployment/build systems ke core mein hain.

**Practice task(s):**
- Bolke explain karo: circular dependency detect karne ke liye topological sort kaise use hoga (Kahn's algorithm ka in-degree-zero logic).
- Ek chhoti coding exercise: 5-6 services ka ek fake dependency graph banao (map se) aur unka deployment order nikaalo topological sort se; ek cycle add karke dekho ki tumhara code use detect karta hai ya nahi.

---

### 5. Sorting & searching, external sorting (merge sort on disk)

**Kya hai:** Sorting algorithms (`quicksort` average O(n log n), `mergesort` guaranteed O(n log n) stable, `heapsort` in-place O(n log n)) data ko order mein arrange karte hain. Searching mein **binary search** O(log n) mein ek sorted array mein element dhoondta hai. **External sorting** tab chahiye jab data RAM mein fit hi na ho — jaise 100 GB file ko sort karna jab RAM sirf 4 GB hai. Iska classic approach hai **external merge sort**: data ko RAM-size ke chunks mein todo, har chunk ko in-memory sort karo aur disk par ek temporary "run" ke roop mein likho, phir saare sorted runs ko ek **k-way merge** se combine karo.

**Kyun important hai:** Har database jo `ORDER BY` par bade dataset handle karti hai (jab index available na ho, ya `GROUP BY`/hash-join ke intermediate steps mein), aur har big-data batch system (Hadoop MapReduce ka shuffle-sort phase, Spark) internally external sorting ka koi variant use karta hai. Ye samajhna batata hai ki "query slow kyun hai jab `work_mem`/sort buffer chhota hai" — Postgres literally log mein "External sort" keyword dikhata hai jab sort RAM mein fit nahi hota.

**Kaise kaam karta hai:** Phase 1 — **run generation**: input file ko RAM-fit chunks mein padho, har chunk ko in-memory sort (quicksort/mergesort) karo, disk par ek sorted "run" file likh do. Agar 100 GB data hai aur RAM 4 GB hai, to ~25 sorted runs banenge. Phase 2 — **k-way merge**: saare runs ko ek saath open karo, har run se sabse chhota unread element compare karo (ek min-heap of size k use karke efficient banate hain), sabse chhota output mein likho, us run se agla element load karo. Isse ek single sequential pass mein saara data merge ho jata hai — disk par bas sequential reads/writes hote hain (random seeks nahi), jo HDD ke liye critical hai (Topic 0.1 ki memory-hierarchy lesson yahin apply hoti hai).

**Real-world example aur backend/system-design connection:** Postgres query planner jab `ORDER BY` ya hash-aggregate `work_mem` se bada data handle karta hai, to `EXPLAIN ANALYZE` mein `"Sort Method: external merge Disk: XXXX kB"` dikhata hai — is line ko dekhkar hi engineers `work_mem` tune karte hain taaki sort in-memory ho jaye (kaafi tez). Hadoop MapReduce ka **shuffle phase** (mappers ke output ko reducers tak pahunchane se pehle) essentially external merge sort hai key ke hisaab se. Linux ka `sort` command-line utility bhi bade files ke liye automatically external merge sort mein switch ho jata hai (`sort -S <buffer-size>` flag se memory budget control hota hai).

```
# Estimation: sorting 100 GB with only 4 GB RAM available for sort buffer
Chunk size = 4 GB  →  number of sorted runs = 100 GB / 4 GB = 25 runs
If merge fan-in k = 25 (merge all at once): 1 merge pass needed
If only k = 10 files can be open at once: ceil(log_10(25)) = 2 merge passes
Each pass = 1 full sequential read + write of ~100 GB
So total I/O ≈ (number_of_passes + 1) × 100 GB sequential — still far cheaper
than 100 GB of RANDOM disk I/O would be.
```

```bash
# Real tool: GNU sort switching to external sort automatically for large files
sort -S 4G -T /tmp -o sorted_output.txt huge_100gb_input.txt
```

**Quick summary:** Jab data RAM mein fit nahi hota, sorting "chunk → sort → sequential k-way merge" ban jata hai — poora design sequential disk I/O maximize karne ke liye optimized hai.

**Practice task(s):**
- Bolke explain karo: external merge sort mein "run generation" aur "k-way merge" phases ka kaam kya-kya hai, aur random I/O kyun avoid hota hai poore process mein.
- `EXPLAIN ANALYZE` ke saath ek Postgres query chalao jisme `work_mem` ko jaanbujhkar bahut chhota set karo (`SET work_mem = '64kB'`) aur ek bada `ORDER BY` chalao — dekho "external merge" line log mein kaise dikhti hai.

---

### Subtopics - practical aha points

**Amortized vs worst-case complexity; why p99 cares about worst-case (rehash pauses, GC pauses):** "Amortized O(1)" ka matlab hai average cost N operations ke over, lekin ek individual operation kabhi-kabhi bahut mehenga ho sakta hai (jaise dynamic array ka resize, ya hash table ka rehash). Average-case metrics (mean latency) is spike ko "smooth" kar dete hain kyunki wo rare hai — lekin **p99/p999 latency** exactly wahi spikes capture karta hai, kyunki ek user jiski request usi moment pe aayi jab resize ho raha tha, uska experience genuinely slow hoga. *Fayda:* ye samjhata hai ki Java ka `HashMap` resize, Go ka `map` growth, ya ek naive cache jo periodically bada allocation karta hai — sab tail-latency spikes ke potential sources hain, chahe unka "average" case fine lage. Isi wajah se production-grade structures (Redis `dict`, `ConcurrentHashMap`) **incremental** resize/rehash karte hain — cost ko many small operations mein spread karke, ek bade pause ko avoid karte hain. `p99` dashboards mein achanak periodic spikes dikhna aksar isi cheez ka symptom hote hain.

**Why B+trees dominate on-disk indexes: fanout, height 3–4 for billions of rows, node = page:** Disk (ya SSD) ek block device hai — chhota data padhna bhi ek pura **page** (typically 4 KB ya 8 KB) padhne jitna hi mehenga hai, aur random seeks (specially HDD par) bahut slow hote hain. B+tree design ye exploit karta hai: har tree **node** exactly ek disk **page** ke barabar hota hai, aur uske andar jitne zyada keys/pointers fit ho sakein, **fanout** utna zyada rakha jata hai. High fanout ka matlab hai tree ki **height** bahut kam rehti hai — matlab ek lookup mein kam se kam disk pages padhne padte hain. *Fayda:* isi wajah se billions of rows ke liye bhi B+tree ki height sirf 3-4 hoti hai — matlab sirf 3-4 disk reads (jinme se top levels aksar RAM cache mein already hote hain) mein koi bhi row mil jaati hai.

```
# Worked arithmetic: B+tree fanout & height for 1 billion rows
Page size = 8 KB
Key size = 8 bytes (bigint), pointer size = 8 bytes
Entry size per key+pointer ≈ 16 bytes
Fanout ≈ 8192 / 16 ≈ 500 children per node

Height needed to index 1,000,000,000 rows with fanout 500:
  500^1 = 500
  500^2 = 250,000
  500^3 = 125,000,000
  500^4 = 62,500,000,000   ← already covers 1 billion rows

So height = 4 is enough for a billion rows — meaning a lookup touches
at most ~4 pages, and the top 1-2 levels almost always sit in RAM/page cache,
so real disk reads per lookup are often just 1-2.
```

**LSM-tree structure: memtable, SSTables, levels, compaction (contrast with B-tree in Level 3):** LSM-tree (Log-Structured Merge-tree) B-tree se bilkul alag philosophy follow karta hai: writes ko in-place disk par update karne ke bajaye, pehle ek in-memory sorted structure — **memtable** (aksar ek skip list ya balanced tree) — mein buffer karta hai. Jab memtable ek threshold size tak pahunch jaata hai, use ek immutable, sorted file ke roop mein disk par flush kar diya jaata hai — isse **SSTable** (Sorted String Table) kehte hain. Time ke saath kai SSTables ban jaate hain multiple **levels** mein (L0, L1, L2...), aur background mein ek **compaction** process chalta rehta hai jo overlapping/duplicate/deleted keys ko merge-clean karta hai. *Fayda:* writes bahut fast hote hain (sirf sequential append, no random disk write), jabki B-tree writes random in-place updates hote hain — ye trade-off hai jise Level 3 mein detail se explore kiya jayega, lekin abhi itna samajhna kaafi hai: LSM = write-optimized (sequential writes, deferred/background cleanup), B-tree = read-optimized (direct in-place lookups, no compaction overhead). RocksDB, Cassandra, aur LevelDB is design ko production mein use karte hain.

**Priority queues for schedulers, timers, rate limiters, delayed queues:** Priority queue (heap se implement hoti hai) "next most urgent item kya hai" ye O(log n) mein deti hai — insert bhi O(log n), extract-min/max bhi O(log n). *Fayda:* ye exact pattern hai jo har scheduler, timer, aur delayed-job system ke peeche hota hai — "agla event kab fire hoga" ye jaldi nikaalna hai bina saari list scan kiye. Linux kernel timers, OS process schedulers (priority-based), rate limiters (token bucket refill timing), aur delayed message queues sab isi idea par based hain. Redis mein ek common pattern hai sorted set (`ZADD queue <unix_timestamp> job_id`) ko delayed queue ki tarah use karna — `ZRANGEBYSCORE queue -inf <now>` chalake "jinka time aa chuka hai" wo jobs nikaal lo; internally Redis sorted sets ek skip list + hash table se implemented hote hain, jisse ye range queries O(log n + k) mein hoti hain.

```
# Redis-based delayed-job pattern using a sorted set as a priority queue
ZADD delayed_jobs 1732450000 "job:send-email:42"
ZADD delayed_jobs 1732450300 "job:cleanup:17"

# Worker polls: fetch every job whose scheduled time has passed
ZRANGEBYSCORE delayed_jobs -inf <current_unix_timestamp>
```

---

### Advanced concepts

**Bloom filter:** Ek **probabilistic set-membership** structure hai — "definitely not present" ya "probably present" bata sakta hai, kabhi "definitely present" nahi (false positives possible hain, false negatives kabhi nahi). Internally ek bit array hota hai aur `k` independent hash functions — insert par har hash function ek bit set karta hai, lookup par saare `k` bits check karta hai (sab set hain to "maybe present", koi bhi 0 hai to "definitely absent"). Optimal size formula: `m = -n·ln(p)/(ln2)²` (`m` = bits, `n` = expected elements, `p` = target false-positive rate). *Fayda:* LSM-tree-based databases (Cassandra, RocksDB, LevelDB) har SSTable ke saath ek Bloom filter rakhte hain — read ke waqt pehle Bloom filter check karte hain, agar wo "definitely absent" bole to us SSTable ko disk se padhna hi skip kar dete hain, jisse read amplification drastically kam ho jaata hai. Chrome browser bhi malicious-URL checking ke liye kabhi Bloom filters use karta tha.

```c
// Minimal Bloom filter: 2 hash functions combined via double hashing
#define M 1000000 // bits
unsigned char bits[M / 8];

void set_bit(unsigned idx) { bits[idx / 8] |= (1 << (idx % 8)); }
int  get_bit(unsigned idx) { return bits[idx / 8] & (1 << (idx % 8)); }

void bloom_add(unsigned h1, unsigned h2, int k) {
    for (int i = 0; i < k; i++) set_bit((h1 + i * h2) % M);
}

int bloom_maybe_contains(unsigned h1, unsigned h2, int k) {
    for (int i = 0; i < k; i++)
        if (!get_bit((h1 + i * h2) % M)) return 0; // definitely absent
    return 1; // maybe present
}
```

**Cuckoo filter / Quotient filter:** Bloom filter ki ek limitation hai — usme se elements **delete** nahi kar sakte (bits shared hote hain multiple elements ke beech, ek bit clear karna kisi aur element ko galat "absent" bana sakta hai). Cuckoo filter isi problem ko solve karta hai — ye har item ka ek chhota "fingerprint" store karta hai bucket slots mein (cuckoo hashing ki tarah — collision par existing entry ko displace/relocate karta hai dusri possible location par), jisse deletion safe ho jaati hai (bas fingerprint hata do). Quotient filter similar guarantee deta hai ek alag internal layout (single array, no separate buckets) ke saath jo cache-friendlier hai. *Fayda:* jab tumhe ek changing/mutable set chahiye (jaise "currently active sessions", jisme add bhi ho aur remove bhi) probabilistic membership ke saath, Bloom filter kaam nahi aayega — Cuckoo/Quotient filter chahiye hoga.

**HyperLogLog:** Cardinality estimation ke liye ek probabilistic structure hai — "kitne **distinct** elements dekhe hain" (jaise unique visitors) bata sakta hai bahut kam memory (~1.5 KB) mein, chahe underlying stream mein billions of elements ho, sirf ~2% error ke saath. *Kaise:* ye is observation par based hai ki agar tum random hash values ke leading zero bits ka **maximum** dekho, to jitne zyada distinct elements honge, utna zyada wo maximum grow karega (probability se related) — HyperLogLog is idea ko many small "registers" mein split karke (statistical variance kam karne ke liye) average leta hai. *Fayda:* Redis ke `PFADD`/`PFCOUNT` commands isi structure par based hain — "kitne unique users ne aaj login kiya" jaise metrics ko billions of events ke liye bhi constant memory mein track karna possible banata hai, jabki ek exact `SET` use karne se memory linearly grow karti (millions of user IDs = megabytes se gigabytes).

**Count-Min Sketch:** Frequency estimation ke liye ek probabilistic structure hai — "ye item kitni baar dekha gaya hai" (approximate count) batata hai, hamesha **overestimate** kar sakta hai lekin kabhi underestimate nahi. Internally ye ek 2D array (`d` rows × `w` columns of counters) aur `d` independent hash functions rakhta hai — increment par har row mein ek counter (uski hash function se decided column par) badhaya jata hai; query par saare `d` rows ka minimum value liya jaata hai (isliye naam "Count-Min"). *Fayda:* "heavy hitters" / hot-key detection ke liye ye perfect hai — jaise ek CDN/cache layer mein "kaunsi keys sabse zyada request ho rahi hain" ye track karna bina har unique key ke liye exact counter allocate kiye. Twitter/X aur many streaming systems trending-topics detection ke liye Count-Min Sketch variants use karte hain.

```c
// Count-Min Sketch: increment and query
#define D 4    // number of hash rows
#define W 2000 // width per row
unsigned counters[D][W];

void cms_increment(unsigned hashes[D]) {
    for (int i = 0; i < D; i++) counters[i][hashes[i] % W]++;
}

unsigned cms_estimate(unsigned hashes[D]) {
    unsigned minVal = 0xFFFFFFFF;
    for (int i = 0; i < D; i++) {
        unsigned v = counters[i][hashes[i] % W];
        if (v < minVal) minVal = v;
    }
    return minVal; // always >= true count, never less
}
```

**t-digest / HdrHistogram:** Metrics systems mein percentile (p50, p95, p99) calculate karna tricky hai jab data distributed multiple machines par ho aur exact values store karna expensive ho. **t-digest** aur **HdrHistogram** dono compact, **mergeable** structures hain jo approximate percentiles deti hain — data ko buckets/clusters mein summarize karte hain, extremes (tails) ke paas zyada resolution rakhte hain kyunki wahi percentiles ke liye zyada matter karta hai. *Fayda:* Prometheus, Datadog jaise monitoring systems mein jab tumhe multiple hosts ke latency histograms ko ek combined "global p99" mein merge karna ho, tumhe aisi structure chahiye jo mergeable ho — raw values store karna (aur global sort karna) scale nahi karta, lekin t-digest/HdrHistogram summaries ko directly merge kiya ja sakta hai bina raw data ke.

**MinHash / SimHash / LSH:** In teeno ka common goal hai — do items kitne **similar** hain ye jaldi estimate karna bina unhe pura compare kiye. **MinHash** sets ki similarity (Jaccard similarity) estimate karta hai — har set par multiple hash functions apply karke unka minimum value ek "signature" banata hai; do sets jitne similar honge unke signatures utne hi match karenge. **SimHash** documents/text ke liye similar idea hai lekin weighted-feature-based hashing se ek compact fingerprint banata hai jahan similar documents ke fingerprints mein Hamming distance chhoti hoti hai. **LSH** (Locality-Sensitive Hashing) ek broader technique hai jo similar items ko high probability se same "bucket" mein hash karti hai, taaki near-duplicate search O(1)-ish ho jaye bajaye sab pairs compare karne ke (jo O(n²) hota). *Fayda:* Google jaise search engines near-duplicate web pages detect karne ke liye SimHash use karte hain (crawl budget bachane ke liye), aur recommendation systems/plagiarism-detection tools MinHash+LSH use karte hain "similar items/documents" jaldi dhoondne ke liye lakhon items ke beech se.

**Merkle trees:** Ek binary tree hai jahan har leaf ek data block ka hash hai, aur har internal node apne children ke hashes ka hash hai — root par ek single hash poore dataset ko represent karta hai. *Kaise:* agar do replicas ke Merkle tree roots match karte hain, to unka poora data identical hai (bina compare kiye har byte). Agar roots mismatch karein, to tree ko top-down traverse karke exactly wo subtree/leaf dhoondhi ja sakti hai jahan difference hai — bina poora dataset transfer kiye. *Fayda:* Cassandra aur DynamoDB **anti-entropy repair** ke liye Merkle trees use karte hain — replicas ke beech data sync karte waqt, sirf mismatched branches ka data transfer hota hai, poora dataset nahi. Git bhi internally commits/trees/blobs ko content-addressed hash tree ki tarah store karta hai (isi wajah se `git diff` do commits ke beech itni jaldi common ancestry figure kar leta hai). Blockchains (Bitcoin) transactions ko ek block ke andar Merkle tree mein organize karte hain taaki koi bhi transaction efficiently verify ho sake bina poora block download kiye (Merkle proof).

**Consistent hashing & rendezvous (HRW) hashing:** Jab tumhe keys ko `N` servers/shards ke beech distribute karna ho, naive approach (`hash(key) % N`) ek problem create karta hai — agar `N` change ho (server add/remove), to **almost saari keys** remap ho jaati hain (massive cache invalidation/data movement). **Consistent hashing** is problem ko solve karta hai — servers aur keys dono ko ek **hash ring** (0 se `2^32-1`) par place karte hain, har key apne clockwise-nearest server ko jaati hai; server add/remove hone par sirf uske paas wali keys affect hoti hain, baaki sab wahi rehti hain. **Virtual nodes** (ek physical server ko ring par multiple points par represent karna) load ko zyada evenly distribute karte hain. **Rendezvous (HRW - Highest Random Weight) hashing** ek alag approach hai — har `(key, server)` pair ke liye ek weight compute karo, jis server ka weight sabse zyada ho wahi key ka owner hai; isme bhi server add/remove hone par minimal remapping hoti hai, aur ye ring maintain kiye bina hi kaam karta hai. *Fayda:* Cassandra, DynamoDB, aur memcached client-side sharding libraries consistent hashing use karte hain taaki horizontal scaling (node add/remove) minimal data reshuffling ke saath ho sake.

```c
// Consistent hashing: place servers on a ring, find owner via next-clockwise
// (conceptual — real implementations use virtual nodes for even distribution)
unsigned ring_positions[NUM_SERVERS]; // sorted hash positions of servers

int find_owner(unsigned key_hash) {
    for (int i = 0; i < NUM_SERVERS; i++) {
        if (key_hash <= ring_positions[i]) return i; // first server clockwise
    }
    return 0; // wrap around to first server
}
```

**Inverted index:** Search engines ka core data structure — ek mapping hai "term → list of documents (postings list) jinme wo term hai", forward index (document → terms) ke bilkul opposite. Postings list mein aksar document IDs sorted rakhe jaate hain, aur **skip pointers** add kiye jaate hain taaki `AND` queries (do terms ka intersection nikaalna) mein poori list linearly scan na karni pade — kuch entries "skip" kiye ja sakte hain agar target unse aage hai. *Fayda:* Elasticsearch/Lucene, aur Google jaise search engines isi structure par based hain — "word X kaunse documents mein hai" ye query O(matching docs) mein answer hoti hai bajaye O(all docs) scan kiye.

```
# Simplified inverted index
"postgres" → [doc12, doc45, doc890, doc1200]
"index"    → [doc3, doc12, doc77, doc890, doc1200]

Query "postgres AND index" → intersect both postings lists → [doc12, doc890, doc1200]
```

**Trie / FST:** Trie (prefix tree) strings ko character-by-character node-path ke roop mein store karta hai, jahan common prefixes automatically share hote hain — "cat", "car", "card" ek hi `c → a` path share karenge phir branch honge. **FST** (Finite State Transducer) ek aur compressed version hai jo prefixes **aur** suffixes dono share karta hai (trie sirf prefixes share karta hai), jisse memory footprint bahut kam ho jaata hai bade dictionaries ke liye. *Fayda:* autocomplete/typeahead features (jab tum "goo" type karte ho aur "google", "good", "goofy" suggestions turant aate hain) trie/FST se backed hote hain — prefix match O(length of prefix) mein ho jaata hai. Lucene/Elasticsearch apne term dictionary ko FST se store karte hain (extremely memory-compact given millions of unique terms). Network routers IP routing tables ke liye trie-based structures (longest prefix match) use karte hain — kis subnet ko packet route karna hai ye decide karne ke liye.

```c
// Trie node — 26 children for lowercase a-z, plus end-of-word marker
typedef struct TrieNode {
    struct TrieNode *children[26];
    int is_end_of_word;
} TrieNode;

void trie_insert(TrieNode *root, const char *word) {
    TrieNode *node = root;
    for (; *word; word++) {
        int idx = *word - 'a';
        if (!node->children[idx]) node->children[idx] = calloc(1, sizeof(TrieNode));
        node = node->children[idx];
    }
    node->is_end_of_word = 1;
}
```

**Roaring bitmaps:** Ek compressed bitmap representation hai jo set-membership/set-operations (union, intersection) ko bahut fast aur memory-efficient banata hai bade integer sets ke liye. *Kaise:* 32-bit integers ko high 16 bits (chunk key) aur low 16 bits (value within chunk) mein split karta hai — har chunk apne data density ke hisaab se automatically best representation choose karta hai (**array container** sparse chunks ke liye, **bitmap container** dense chunks ke liye, **run-length container** consecutive-run-heavy chunks ke liye). *Fayda:* analytics databases (ClickHouse) aur search engines (Lucene/Elasticsearch) filters ko roaring bitmaps se represent karte hain — "status = active AND region = APAC" jaise multi-condition filters ko bitmap AND operation se milliseconds mein combine kar dete hain, chahe underlying data mein crores rows ho.

**Ring buffer / disruptor pattern:** Basic ring buffer (Topic 1 mein cover hua) ka ek advanced production pattern hai **LMAX Disruptor** — ek lock-free, single-producer/single-consumer (ya multi-producer/multi-consumer variants) queue jo mutex/lock use nahi karta, balki atomic **sequence counters** (CAS - Compare-And-Swap operations) use karta hai producer aur consumer ke beech coordination ke liye. *Kaise:* producer apna write-sequence atomically increment karta hai, consumer apna read-sequence track karta hai aur sirf tab aage badhta hai jab producer ka sequence usse aage ho gaya ho — is tarah dono bina kisi lock/blocking ke, cache-friendly (pre-allocated ring, no GC pressure) tareeke se coordinate karte hain. *Fayda:* ye pattern extreme-low-latency trading systems (jahan se ye originally aaya — LMAX exchange) aur high-throughput messaging systems (Aeron) mein use hota hai jahan traditional lock-based queues (mutex contention) ki latency bhi unacceptable hoti hai.

---

### Important terms

- **Amortized complexity:** Average cost per operation over a sequence, even though individual operations kabhi mehenge ho sakte hain (jaise dynamic array resize).
- **Load factor:** Hash table mein `n / tableSize` ratio — kab resize trigger hoga ye decide karta hai.
- **Incremental rehashing:** Poori hash table ko ek baar mein rehash karne ke bajaye, operations ke saath thoda-thoda migrate karna (Redis `dict`).
- **Fanout:** Ek tree node ke kitne children ho sakte hain — B+tree mein high (100s), BST mein sirf 2.
- **Memtable:** LSM-tree ka in-memory write buffer, flush hone se pehle.
- **SSTable:** Sorted String Table — LSM-tree ka immutable, disk par sorted output file.
- **Compaction:** LSM-tree ka background process jo multiple SSTables ko merge/clean karta hai (deleted/duplicate keys hatana).
- **False positive rate (p):** Bloom filter mein probability ki galat se "present" bol de jab actually absent ho.
- **Cardinality:** Kisi set mein distinct elements ki sankhya — HyperLogLog isi ka estimate deta hai.
- **Heavy hitters:** Wo items jo stream mein sabse zyada frequency se occur karte hain — Count-Min Sketch inhe detect karta hai.
- **Jaccard similarity:** Do sets ki similarity measure — intersection size / union size — MinHash isi ko approximate karta hai.
- **Anti-entropy repair:** Distributed databases mein replicas ke beech data ko sync/consistent rakhne ka background process — Merkle trees isse efficient banate hain.
- **Hash ring:** Consistent hashing mein servers aur keys ko ek circular hash-space par place karne ka concept.
- **Virtual nodes:** Ek physical server ko hash ring par multiple positions par represent karna, taaki load evenly distribute ho.
- **Postings list:** Inverted index mein ek term ke saath associated document IDs ki list.
- **FST (Finite State Transducer):** Ek compressed trie-jaisi structure jo prefixes aur suffixes dono share karti hai.
- **Write amplification (context: LSM):** Ek logical write ki wajah se compaction ke through hone wala extra physical write I/O.
- **External sort:** Aisi sorting jahan data RAM mein fit nahi hoti — disk par chunks sort karke phir merge kiya jata hai.
- **k-way merge:** Multiple pehle-se-sorted runs ko ek single sorted output mein combine karna, typically ek min-heap ke through.
- **Tombstone:** Open-addressing hash table ya LSM-tree mein ek "deleted" marker jo actual removal se alag hota hai.

---

### Common mistakes

- Hash table/dynamic array ka expected size approx pata hone ke bawajood pre-allocate/`reserve` na karna — resulting mein multiple unnecessary resizes aur copies hoti hain jo avoid ho sakti thi.
- B+tree index ke fanout aur height ka mental model na hone ki wajah se ye maan lena ki "index lagaya hai to fast hi hoga" — jabki wrong column order (composite index mein), ya index bloat, ya bahut zyada dead tuples (Postgres mein) effective height/pages badha sakte hain.
- Bloom filter use karte waqt uska false-positive rate check hi na karna ya `m`/`k` galat calculate karna, jisse effectively har lookup "maybe present" bolke fallback ki zaroorat create kar de — poora benefit khatam.
- Naive `hash(key) % N` se sharding karna production system mein, phir server count change hone par surprise hona ki "almost saara cache cold ho gaya" — consistent hashing na use karne ka direct consequence.
- Count-Min Sketch ko galat samajhna aur use "exact count" ki tarah treat karna — ye hamesha overestimate karta hai (kabhi exact ya kam nahi), is guarantee ko na samajhna galat alerting/decisions ki wajah ban sakta hai.
- External sort ke context mein `work_mem`/sort buffer ko itna bada set kar dena ki multiple concurrent queries milke saara server RAM khatam kar dein (har connection apna khud ka `work_mem` use karta hai Postgres mein) — global memory budget calculate na karna.
- Graph traversal mein visited-set track na karna aur cyclic graph par infinite loop mein fall ho jaana (especially DFS mein without proper visited marking).
- LSM-tree wale system mein compaction ko ignore/misconfigure karna, jisse read amplification (bahut saari overlapping SSTables scan karni padti hain) silently badh jaata hai over time.

---

### Interview questions

1. Dynamic array resize amortized O(1) kyun hai jabki individual resize operation O(n) hai? Doubling strategy ka role kya hai?
   *Hint:* Total cost across N inserts sum karo geometric series se — total copies ≈ 2N, isliye per-insert average O(1) hai.

2. Hash table mein chaining aur open addressing ka trade-off kya hai — load factor high hone par dono kaise perform karte hain?
   *Hint:* Chaining gracefully degrade karta hai (list length badhti hai); open addressing high load factor par clustering aur probing length badhne se sharply degrade hota hai — isi wajah se open addressing lower load factor threshold rakhta hai.

3. B+tree ka B-tree se kya farq hai, aur database indexes B+tree kyun prefer karte hain?
   *Hint:* B+tree mein data sirf leaves mein hota hai, leaves linked list hoti hain (range scans ke liye efficient); B-tree ke internal nodes mein bhi data hota hai jo range scan ko complicate karta hai.

4. LSM-tree writes B-tree se fast kyun hote hain, aur is trade-off mein reads ka kya hota hai?
   *Hint:* LSM sequential append (memtable → SSTable flush) karta hai, random in-place update nahi; reads ko multiple SSTable levels check karne pad sakte hain (read amplification), isliye Bloom filters saath use kiye jaate hain.

5. Bloom filter false positive de sakta hai lekin false negative kabhi nahi — kyun? Isse ek real system (jaise LSM read path) mein kaise use karte hain?
   *Hint:* Bits sirf set hote hain kabhi clear nahi (insert-only), isliye "not present" ka answer hamesha reliable hai; "maybe present" ko verify karne ke liye actual data check karna padta hai.

6. Consistent hashing normal `hash % N` se better kyun hai jab servers add/remove hote hain?
   *Hint:* Ring-based placement se sirf neighboring server ki keys affect hoti hain remap mein; `% N` se almost saari keys ka mapping change ho jaata hai N change hone par.

7. Count-Min Sketch mein `d` rows aur `w` columns ka trade-off kya hai accuracy aur memory ke beech?
   *Hint:* Zyada `w` → kam collision per row → better accuracy; zyada `d` → minimum lene se error probability aur kam hoti hai, dono memory cost badhate hain.

8. Amortized complexity ka production p99 latency par kya asar padta hai, aur isse kaise mitigate karte hain?
   *Hint:* Rare-but-expensive operations (resize/rehash/GC) tail latency spikes create karte hain; incremental rehashing, pre-allocation, ya background compaction se mitigate karte hain.

9. Topological sort kab possible nahi hota, aur ye production mein kis real problem ko detect karne ke liye use hota hai?
   *Hint:* Graph mein cycle ho to topological sort impossible hai; deployment/build-dependency circular reference detect karne ke liye use hota hai.

10. External merge sort mein disk I/O pattern (random vs sequential) kis tarah design kiya gaya hai, aur ye Level 0.1 ki memory-hierarchy lesson se kaise connect hota hai?
    *Hint:* Poora design sequential reads/writes maximize karta hai (chunk sort + linear k-way merge) kyunki random disk I/O (especially HDD) orders of magnitude slower hai — yahi memory-hierarchy ka practical application hai.

---

### Hands-on lab

> Implement a Bloom filter and a Count-Min Sketch from scratch. Measure false-positive rate vs your formula.

**Ye specific lab kyun:** Ye lab tumhe probabilistic data structures ko sirf "formula yaad rakhna" se "khud verify kiya hua understanding" mein badal deta hai. Jab tum apna khud ka Bloom filter banake usse formula-predicted false-positive rate ke against measure karte ho, tab pata chalta hai ki `m = -n·ln(p)/(ln2)²` sirf ek theoretical equation nahi hai — real bits, real hash functions ke saath ye kaise behave karta hai. Yahi samajh Level 3 aur real production systems (jahan Bloom filters aur sketches literally har jagah — caches, databases, network systems mein — chhupe hote hain) mein directly kaam aayegi.

- [x] Mera result: C++ implementation, linked as [labs/0.3-bloom-countmin/](labs/0.3-bloom-countmin/)
  - Bloom filter: measured false-positive rate = 0.00972 (formula predicted 0.01, n=100000)
  - Count-Min Sketch: max observed error = 1092, theoretical bound = 2000 (eps=0.001, delta=0.01) — no underestimate was found, as expected.

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.4 Operating systems for system designers

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har topic ke saath: **kya hai → kyun important hai → kaise kaam karta hai → real-world aur backend/system-design example.**

### 1. Processes vs threads vs coroutines; the scheduler; context switch cost (~1–5 µs)

**Kya hai (What):** **Process** ek independent execution unit hai jiska apna virtual address space, apna page table, apna file descriptor table hota hai — OS ke liye ye ek fully isolated "sandbox" hai. **Thread** process ke andar chalta hai aur usi process ke address space, heap, open files ko share karta hai, lekin apna stack, apne registers, apna program counter rakhta hai. **Coroutine** (ya "green thread" — Go mein **goroutine**, Kotlin mein coroutine, Python mein `asyncio` task) ek **user-space** execution unit hai jise kernel jaanta hi nahi — ek language runtime khud multiple coroutines ko kam OS threads par multiplex (schedule) karta hai. OS ka **scheduler** (Linux par default **CFS** — Completely Fair Scheduler) decide karta hai kaunsa thread kaunse core par kab chalega — per-CPU run queues, priorities (`nice` value), aur timer-interrupt-based preemption (kernel periodically ek "tick" interrupt leta hai taaki lambe chalne wale thread ko rok kar doosre ko mauka de sake).

**Kyun important hai (Why):** Ye samajhna seedha batata hai ki "jitne zyada threads utna zyada concurrency" galat soch hai. Har OS thread costly hai — memory mein stack (Linux default ~8MB virtual, Java classic ~512KB-1MB), aur har **context switch** (ek thread ko rok kar doosra chalana) ~1–5 µs leta hai kyunki kernel ko registers save/restore karne hote hain, kabhi-kabhi page table switch karna padta hai (agar process badal raha hai, to TLB entries bhi invalid ho jaati hain — naya process apna translation cache warm karega scratch se), aur CPU caches (L1/L2) "cold" ho jaate hain naye thread ke liye. 10,000 OS threads spawn karke 10,000 concurrent connections handle karne ki koshish is wajah se scale nahi karti — scheduler khud hi overhead ban jaata hai.

**Kaise kaam karta hai (How):** Process banta hai `fork()` (ya `clone()` bina `CLONE_VM`) se — poora naya address space (ya copy-on-write copy). Thread banta hai `pthread_create()` (jo internally `clone(CLONE_VM | CLONE_FS | CLONE_FILES | ...)` call karta hai) se — address space share hota hai, sirf stack alag hota hai. Kernel scheduler ka kaam: har CPU core ki apni run queue hoti hai, CFS "virtual runtime" track karta hai taaki fairness maintain ho, aur blocking syscall (jaise disk read) par thread khud hi voluntarily CPU chhod deta hai (reschedule trigger hota hai). Goroutines ka model alag hai — Go runtime **M:N scheduling** use karta hai: M (OS threads) par N (goroutines, jinka initial stack sirf ~2KB hota hai aur grow hota hai) ko **P** (logical processor, GOMAXPROCS ke barabar) ke through schedule kiya jaata hai. Goroutine switch mein kernel involve nahi hota — bas kuch registers aur stack pointer userspace mein save/restore hote hain, isliye ye ~tens of nanoseconds mein ho jaata hai, kernel context switch se ~100x sasta.

**Real-world example aur backend/system-design connection:** Classic Java Tomcat/Apache "thread-per-request" model ~5,000-10,000 concurrent connections ke baad struggle karta hai — memory aur context-switch overhead dono badh jaate hain. Isi problem ko solve karne ke liye Nginx ek fixed number of worker processes (typically = CPU core count) rakhta hai, har worker ek **event loop** (epoll based) se hazaaron connections handle karta hai bina thread-per-connection ke. Go mein `net/http` server har incoming connection ke liye ek naya goroutine spawn karta hai — ye "thread-per-connection" jaisa lagta hai code mein, lekin runtime ke andar M:N scheduling ki wajah se practically event-loop jaisi efficiency deta hai. System design mein ye decision — thread pool size, event-loop, ya goroutine model — directly decide karta hai ki ek backend service kitne concurrent connections ek machine par handle kar sakti hai.

```go
// Go: goroutine-per-connection — cheap kyunki M:N scheduling hai
func handleConn(conn net.Conn) {
    defer conn.Close()
    // ye goroutine ek OS thread block NAHI karta jab blocking I/O karta hai —
    // Go runtime is goroutine ko "park" karke us OS thread ko kisi aur
    // ready goroutine ko de deta hai (netpoller epoll ke upar based hai)
    buf := make([]byte, 4096)
    for {
        n, err := conn.Read(buf) // blocking-looking, par non-blocking internally
        if err != nil {
            return
        }
        conn.Write(buf[:n])
    }
}

func main() {
    ln, _ := net.Listen("tcp", ":9000")
    for {
        conn, _ := ln.Accept()
        go handleConn(conn) // ~2KB stack, sasta context switch
    }
}
```

```bash
# Context switch cost khud measure karo Linux par:
perf bench sched pipe
# Output kuch aisa dikhata hai: "X usecs/op" — typically 1-5 usec range,
# dependent on CPU, kernel version, aur mitigations (KPTI etc.) enabled hain ya nahi.
```

**Quick summary:** Process = isolated (own address space), thread = shared-memory concurrency within a process, coroutine/goroutine = cheap user-space concurrency multiplexed by a runtime. Context switch (~1-5 µs) OS threads ko "expensive currency" banata hai — isi wajah se high-concurrency backend systems event loops ya M:N runtimes use karte hain.

**Practice task(s):**
- Apne alfaazon mein bolke explain karo: ek OS thread context switch ke waqt kernel exactly kya-kya save/restore karta hai, aur goroutine switch usse kyun sasta hai.
- `perf bench sched pipe` (ya `taskset` ke saath do processes pipe se ping-pong karte hue) chala kar apni machine par actual context-switch latency measure karo.
- Ek chhoti Go script likho jo 100,000 goroutines spawn kare (kuch bhi halka kaam karte hue) aur memory/time note karo — phir socho ki 100,000 OS threads spawn karne par kya hota.

---

### 2. Virtual memory, page tables, TLB, swap, OOM killer

**Kya hai (What):** Har process apna khud ka **virtual address space** dekhta hai (jaise usi ke paas poori RAM ho) — asal mein CPU ka **MMU** (Memory Management Unit) har virtual address ko physical address mein translate karta hai ek **page table** ke through (x86-64 par 4-level hierarchy: PML4 → PDPT → PD → PT). Ye translation baar-baar memory access karne ke barabar mehenga hai, isliye CPU ke paas ek chhota, super-fast cache hota hai jise **TLB** (Translation Lookaside Buffer) kehte hain — recently used virtual→physical mappings yahan cached rehti hain. Jab physical RAM kam pad jaaye, kernel kuch inactive pages ko disk par **swap** kar deta hai taaki RAM free ho. Jab bilkul bhi memory free nahi ho paa rahi (swap bhi khatam), Linux ka **OOM killer** (Out-Of-Memory killer) kisi process ko forcefully kill kar deta hai based on a heuristic score.

**Kyun important hai (Why):** Ye concept batata hai ki "memory" ek illusion hai jo kernel maintain karta hai — aur is illusion ki cost samajhna zaroori hai. Bade heaps (jaise ek JVM ka 32GB heap) ke saath TLB miss rate badh jaata hai kyunki TLB mein limited entries hoti hain (typically kuch sau se kuch hazaar) — isse latency-sensitive apps slow ho sakte hain (isi ki wajah se huge pages exist karte hain, Advanced section dekho). Swap ka production databases ke liye matlab hai disaster — agar DB ka buffer pool swap ho jaaye, to "RAM se fast read" ka assumption tut jaata hai, latency milliseconds se seconds tak jump kar sakti hai. Aur OOM killer ka trigger hona — especially container ke andar — ek confusing production incident hota hai jab tak aap iska mechanism na samjho.

**Kaise kaam karta hai (How):** Jab CPU ek virtual address access karta hai, TLB mein check hota hai — hit hone par instant translation milta hai. Miss hone par kernel/hardware page table walk karta hai (multiple memory accesses, ~100ns+ ka overhead). Agar wo page physical RAM mein hi nahi hai, to **page fault** hota hai — do types: **minor fault** (page kahin memory mein hai, bas is process ke page table mein map nahi hai — jaise copy-on-write ya lazily-allocated `mmap` page — sasta hai, sirf ek mapping update), aur **major fault** (page disk par hai — swap ya file-backed — kernel ko disk se read karna padega, expensive, milliseconds tak lag sakta hai). Swap: kernel ke paas ek **page reclaim** algorithm hai (roughly LRU-ish) jo kam-use hone wale pages ko evict karta hai; agar working set physical RAM se bada ho to system **thrash** karne lagta hai — CPU zyada time swap-in/swap-out mein hi bitane lagta hai. OOM killer: jab allocation fail ho rahi ho aur reclaim se bhi kuch na bache, kernel har process ka `oom_score` calculate karta hai (memory usage, `oom_score_adj` jaise factors se) aur sabse "worst offender" ko `SIGKILL` bhej deta hai.

**Real-world example aur backend/system-design connection:** Postgres aur MySQL production tuning guides explicitly `vm.swappiness=1` (ya `0`) set karne ki recommend karte hain — kyunki agar kernel database ke buffer pool ko swap kar de, to queries jo "RAM se fast" expect kar rahi thi achanak disk-latency slow ho jaati hain, aur ye symptom monitoring mein confusing "random slowness" jaisa dikhta hai (bilkul NUMA wale mystery jaisa). Kubernetes mein jab ek pod apni `memory.limit` cross karta hai, cgroup ka OOM killer us container ke process ko kill karta hai — yahi wajah hai `kubectl describe pod` mein `OOMKilled` status dikhta hai; is samay production engineers `oom_score_adj` aur proper memory `requests`/`limits` sizing karke is se bachte hain.

```bash
# Ek process ki actual memory footprint check karo:
grep -E 'VmRSS|VmSwap|VmSize' /proc/<pid>/status
# VmSize = virtual (illusion, bahut bada ho sakta hai mmap ki wajah se)
# VmRSS  = actually RAM mein resident
# VmSwap = kitna is process ka swap ho chuka hai

# Swappiness dekho/tune karo (0 = swap avoid karo jitna ho sake):
sysctl vm.swappiness

# OOM killer ka history dekho:
dmesg -T | grep -i "Killed process"
# ya systemd wale system par:
journalctl -k | grep -i "out of memory"
```

**Quick summary:** Virtual memory = per-process illusion, page tables + TLB isse fast banate hain (TLB miss = costly page walk). Swap RAM khatam hone par disk ka use karta hai (slow), aur agar wahan bhi jagah na bache to OOM killer kisi process ko force-kill karta hai.

**Practice task(s):**
- Apne alfaazon mein bolke explain karo: minor page fault aur major page fault mein kya fark hai, aur ek `mmap`-based lazy allocation minor fault kyun trigger karega.
- `/proc/self/status` padh kar apne khud ke shell process ka VmRSS aur VmSize compare karo — samjho ye do numbers itne alag kyun hain.
- `vm.swappiness` apni machine par check karo aur socho: agar tum ek Redis server deploy kar rahe ho, to ye value kya honi chahiye aur kyun.

---

### 3. File systems: inodes, page cache, `fsync`, journaling, durability guarantees

**Kya hai (What):** **Inode** ek data structure hai jo ek file ke metadata ko store karta hai — size, permissions, timestamps, aur disk blocks ke pointers — lekin file ka **naam** inode mein nahi hota, wo ek separate structure (**dentry**, directory entry) mein hota hai jo naam ko inode number se map karta hai (isi wajah se ek hi file ke multiple hard links ho sakte hain — sab same inode point karte hain). **Page cache** OS ka wo mechanism hai jo disk files ke pages ko transparently RAM mein cache karta hai — read tez ho jaate hain kyunki dobara disk tak nahi jaana padta, aur write bhi pehle sirf page cache mein hoti hai (page **dirty** mark ho jaati hai) — asal disk write baad mein async hoti hai kernel ke writeback threads se. `fsync(fd)` ek syscall hai jo explicitly kehta hai: "is file ke saare dirty pages ko **abhi** durable storage tak pahuncha do, aur wapas mujhe tabhi batana jab confirm ho jaaye." **Journaling** filesystems (ext4, XFS) crash-consistency ke liye metadata changes (kabhi data bhi) pehle ek sequential **journal log** mein likhte hain, phir asal jagah — taaki crash ke beech mein bhi filesystem structure corrupt na ho.

**Kyun important hai (Why):** Ye sabse zyada misunderstood durability concept hai: `write()` ka successfully return karna iska matlab data **safe** hai, ye galat assumption hai — data abhi sirf RAM (page cache) mein hai. Agar isi waqt power chali jaaye ya kernel crash ho jaaye, wo data **gayab** ho sakta hai — kyunki wo kabhi disk tak pahunchi hi nahi. Ye samajhna hi ACID ke "D" (Durability) ko real banata hai — har serious database engine isi wajah se apne commit path mein explicitly `fsync`/`fdatasync` call karta hai. Journaling batata hai ki crash ke baad filesystem `fsck` ke bina bhi consistent kyun mil jaata hai — lekin ye tumhara **application data** durable hone ki guarantee nahi deta, sirf filesystem ke apne internal structures ki deta hai.

**Kaise kaam karta hai (How):** Jab app `write(fd, buf, n)` call karta hai, kernel user buffer se data page cache mein copy kar deta hai aur turant return kar deta hai — bahut fast, kyunki disk I/O involve hi nahi hui. Kernel background mein (writeback threads, `dirty_ratio`/`dirty_expire_centisecs` jaise tunables ke hisaab se) dirty pages ko periodically disk par flush karta hai. `fsync(fd)` explicitly us file ke saare dirty pages ko flush karwata hai **aur** storage device ko ek "flush your own cache" command (FLUSH CACHE / FUA — Force Unit Access) bhejta hai, taaki disk ka apna volatile write cache bhi bypass ho jaaye. Journaling ka mechanism: filesystem pehle ek transaction ko journal (sequential, append-only area) mein likhta hai, journal entry ko commit karta hai (khud ek barrier/flush ke saath), phir hi asal blocks update karta hai — agar crash beech mein ho jaaye, reboot par journal replay ho jaata hai aur filesystem consistent state mein aa jaata hai.

**Real-world example aur backend/system-design connection:** Postgres ka **WAL** (Write-Ahead Log) isi principle par bana hai — jab tum `COMMIT` karte ho, Postgres WAL record likhta hai aur (`synchronous_commit=on` hone par) `fsync` call karke wait karta hai confirm hone tak, tabhi client ko "commit successful" bolta hai — yahi durability ki real guarantee hai. MySQL InnoDB mein `innodb_flush_log_at_trx_commit=1` isi behavior ko enforce karta hai (har commit par fsync), jabki `=0`/`=2` performance ke liye durability trade-off karte hain. Interesting contrast: Kafka apne throughput ke liye per-message `fsync` **nahi** karta — wo durability ka bhaar OS page cache + **replication** (multiple brokers par copy) par daalta hai, ye ek explicit design tradeoff hai (fsync-per-write vs replicate-and-batch-flush).

```c
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("wal.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    const char *record = "COMMIT txn=42 amount=500\n";

    // Ye sirf page cache mein jaata hai — FAST, par crash hua to data gaya
    write(fd, record, 26);

    // Ye explicitly kernel + disk se confirm leta hai ki data durable hai
    // — SLOW (disk I/O ka wait), par crash-safe
    fsync(fd);

    close(fd);
    return 0;
}
```

**Quick summary:** `write()` sirf page cache tak pahunchata hai (fast, non-durable); `fsync` disk tak force karta hai (slow, durable). Journaling filesystem ki apni consistency ke liye hai, tumhare application data ki durability ke liye tumhe khud `fsync` chahiye.

**Practice task(s):**
- Apne alfaazon mein bolke explain karo: agar `write()` return ho gaya lekin `fsync` nahi hua tha, aur usi waqt power chali gayi, to kya hoga — aur ye Postgres ke commit guarantee se kaise related hai.
- `strace -e trace=write,fsync <koi command jo file likhta ho>` chala kar dekho `write` aur `fsync` alag syscalls ke roop mein kaise dikhte hain.
- Ek chhoti script likho jo ek file mein 1000 baar write kare — ek version `fsync` ke saath har write ke baad, doosra bina — time difference note karo.

---

### 4. I/O models: blocking, non-blocking, I/O multiplexing (`select`/`poll`/`epoll`/`kqueue`), async (`io_uring`), signal-driven

**Kya hai (What):** **Blocking I/O** — thread ek syscall (jaise `read()`) call karta hai aur jab tak data available na ho, wo thread bilkul rukk jaata hai (CPU se hata diya jaata hai). **Non-blocking I/O** — syscall turant return karta hai, agar data ready nahi hai to `EAGAIN`/`EWOULDBLOCK` error de deta hai, caller ko khud dobara try karna padta hai (polling). **I/O multiplexing** ek single thread ko ek saath hazaaron file descriptors ko "watch" karne deta hai aur sirf tab kaam karta hai jab koi ek ready ho: `select()` (purana, max ~1024 fds, O(n) scan har call mein), `poll()` (fd limit nahi, phir bhi O(n) scan), `epoll` (Linux-specific, kernel ek interest list maintain karta hai aur sirf "ready" fds ka list deta hai — practically O(1) amortized), `kqueue` (BSD/macOS ka epoll-equivalent). **Async I/O** (`io_uring`, Linux ka newest model) syscall-per-operation ka overhead bhi hata deta hai — do shared ring buffers (Submission Queue aur Completion Queue) user aur kernel ke beech mmap hote hain, app operations submit karta hai bina syscall ke (batch mein), kernel unhe complete karke completion queue mein daal deta hai. **Signal-driven I/O** (`SIGIO`) kernel ko bolta hai "jab fd ready ho, mujhe signal bhejo" — theory mein useful, practice mein rarely use hota hai kyunki signal handling complex/fragile hai.

**Kyun important hai (Why):** Ye poora mechanism hi wo asli jawab hai jo **C10K problem** (10,000 concurrent connections ek server par) ko solve karta hai (agla subtopic dekho). Isse pehle "ek thread per connection" model hi tha — jo memory aur context-switch cost ki wajah se hazaaron connections par gir jaata tha. I/O multiplexing ne "**ek** thread, hazaaron connections" possible banaya. `io_uring` isse ek kadam aur aage le jaata hai — jahan `epoll` bhi ab bhi ek syscall-per-notification model hai, `io_uring` syscall overhead ko bhi minimize karta hai, jo bahut high IOPS (lakhs/sec) systems ke liye matter karta hai.

**Kaise kaam karta hai (How):** `epoll` teen syscalls ka set hai: `epoll_create1()` ek kernel-side object banata hai, `epoll_ctl()` usme fds register/modify/remove karta hai (kaunse events chahiye — `EPOLLIN`, `EPOLLOUT`), aur `epoll_wait()` block karta hai jab tak koi registered fd ready na ho, phir sirf ready fds ka list return karta hai. Internally kernel registered fds ko ek red-black tree mein rakhta hai aur ek separate "ready list" maintain karta hai jo tab update hoti hai jab hardware/network event aata hai — isliye `epoll_wait` ko sirf ready fds check karne padte hain, saare registered fds nahi (yahi `select`/`poll` se fundamental difference hai). **Level-triggered** mode mein jab tak buffer mein data hai, `epoll_wait` baar-baar ready bolega; **edge-triggered** mode mein sirf ek baar batayega jab state change ho — app ko tab tak read karte rehna hoga jab tak `EAGAIN` na mile. `io_uring` mein app ek submission queue entry (SQE) likh kar kernel ko batata hai (bina syscall ke — shared memory ring), kernel operation complete karke ek completion queue entry (CQE) daal deta hai — app periodically (ya ek `io_uring_enter` call se, jo batch mein multiple ops submit/reap kar sakta hai) CQ check karta hai.

**Real-world example aur backend/system-design connection:** Redis single-threaded hai command-execution ke liye, lekin uska event loop (`ae.c` module) `epoll` (Linux par) use karke ek saath hazaaron client connections multiplex karta hai — yahi wajah hai Redis ko apne core commands ke liye locks nahi chahiye (ek waqt mein ek hi command chal raha hota hai). Nginx ka har worker process similarly `epoll`-based event loop chalata hai — chhoti si fixed number of workers (typically = core count) lakhs of concurrent connections handle kar lete hain reverse proxy ke roop mein. Node.js ka `libuv` OS ke hisaab se `epoll` (Linux), `kqueue` (macOS/BSD), ya IOCP (Windows) use karta hai. Newer high-performance systems (ScyllaDB apna khud ka async I/O engine, aur Postgres/MySQL ke experimental `io_uring` patches) `io_uring` ki taraf move kar rahe hain jahan syscall overhead khud bottleneck ban jaata hai.

```c
#include <sys/epoll.h>
#include <unistd.h>
#include <stdio.h>

#define MAX_EVENTS 64

int main() {
    int epfd = epoll_create1(0);
    struct epoll_event ev, events[MAX_EVENTS];

    // Maan lo listen_fd ek already-bound, listening, non-blocking socket hai
    int listen_fd = 0; // placeholder
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

    for (;;) {
        int n = epoll_wait(epfd, events, MAX_EVENTS, -1); // block until ready
        for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                // naya connection accept() karo, non-blocking set karo,
                // epoll_ctl(EPOLL_CTL_ADD) se register karo
            } else {
                // fd par read()/write() karo — ye ready hai, block nahi karega
            }
        }
    }
}
```

**Quick summary:** `select`/`poll` O(n) scan karte hain (aur `select` mein 1024 fd limit); `epoll` kernel-maintained ready-list se O(1)-ish scale karta hai; `io_uring` syscall-per-op overhead bhi hata deta hai. Yahi mechanism single-thread event-loop servers (Redis, Nginx, Node.js) ko hazaaron connections handle karne deta hai.

**Practice task(s):**
- Apne alfaazon mein bolke explain karo: `epoll` `select` se fundamentally fast kyun hai (sirf "naya hai" nahi — actual data-structure level reason bolo).
- Level-triggered aur edge-triggered `epoll` mode ka fark ek chhote diagram/example ke saath khud ko explain karo.
- Upar wale `epoll` skeleton ko complete karke ek chhota echo server banao — ye is section ke hands-on lab ka pehla version hoga.

---

### 5. System calls, user vs kernel space, the cost of crossing the boundary

**Kya hai (What):** CPU hardware-level pe do (ya zyada) **privilege modes/rings** support karta hai — **user mode** (x86 par ring 3) jahan normal applications chalte hain, restricted — direct hardware access nahi, aur **kernel mode** (ring 0) jahan OS chalta hai, full hardware control ke saath. **System call** wo controlled, well-defined entry point hai jisse ek user-space program kernel se kuch mangta hai — file I/O, memory allocation, process creation, networking — kyunki ye sab cheezein sirf kernel hi kar sakta hai. x86-64 par ye ek special CPU instruction (`syscall`) se hota hai (purane systems mein `int 0x80` interrupt se), jisme ek register (RAX) mein syscall number daal kar CPU ko trap karaya jaata hai.

**Kyun important hai (Why):** Syscalls **free nahi hain** — har ek mein mode switch (user→kernel→user) hota hai, jo cache aur TLB ko disturb karta hai, aur security mitigations ki wajah se aur bhi costly ho gaya hai: Meltdown/Spectre ke baad Linux kernel mein **KPTI** (Kernel Page Table Isolation) aaya, jo har user↔kernel transition par TLB ka ek bada hissa flush kar deta hai taaki speculative-execution side-channel attacks na ho sakein — is wajah se syscall-heavy workloads ~5-30% tak slow ho sakte hain KPTI enabled hone par. Ye batata hai ki "buffered I/O" jaisi cheezein sirf convenience nahi hain — wo directly syscall count kam karke real CPU time bachaati hain, aur ye samajhna hai ki `io_uring` jaisi technologies kyun exist karti hain (syscall-per-operation cost ko minimize karne ke liye).

**Kaise kaam karta hai (How):** Jab app `syscall` instruction execute karta hai, CPU trap leta hai — privilege level ring 3 se ring 0 mein switch hoti hai, execution kernel ke ek fixed entry point par jump karta hai jo RAX register mein diye gaye syscall number ko ek **syscall table** mein lookup karke sahi kernel function call karta hai. Kernel apna kaam karta hai (privileged instructions, hardware access), result register mein daalta hai, aur `sysret`/`iret` instruction se wapas user mode mein return karta hai. Har cheez ke liye full trap mehenga hai — isliye kuch bahut-frequently-called, read-only cheezein (jaise `gettimeofday()`, `clock_gettime()`) ke liye kernel ek **vDSO** (virtual Dynamic Shared Object) provide karta hai — kernel ka code hi process ke address space mein map kar diya jaata hai, taaki wo function call user space mein hi resolve ho jaaye, bina trap ke.

**Real-world example aur backend/system-design connection:** Ek simple "hello world" C program bhi `strace` mein dekho to dozens syscalls dikhte hain (`execve`, `mmap`, `openat`, `write`, `exit` — dynamic linker load karne ke liye bhi). Glibc ki buffered `stdio` (`fwrite`, `printf`) internally ek user-space buffer maintain karti hai aur usse tabhi flush karti hai jab wo bhar jaaye ya explicitly flush ho — isliye ek loop mein `fwrite()` calls raw unbuffered `write()` syscalls se kahin zyada fast hote hain (ek syscall vs hazaaron). High-performance systems (Envoy proxy, Cassandra, Kafka) explicitly batch sizes tune karte hain taaki per-request/per-message syscall count minimize ho — ye ek direct backend performance lever hai, sirf theory nahi.

```bash
# Kisi command ke saare syscalls count aur time ke saath dekho:
strace -c ls /tmp
# Output batata hai: kitni baar kaunsa syscall call hua, aur usme total time kitna gaya —
# production debugging mein "kahan CPU/time ja raha hai" samajhne ka powerful tool hai.

# vDSO functions dekhna (ye normal syscalls ki tarah trap nahi karte):
strace ./a.out 2>&1 | grep -c clock_gettime
# Agar vDSO use ho raha hai, to clock_gettime shayad strace ke output mein
# bilkul nahi dikhega — kyunki wo kernel trap kabhi hua hi nahi!
```

**Quick summary:** Syscall = controlled, costly user→kernel→user mode switch; KPTI jaisi security mitigations ne is cost ko aur badhaya hai. Buffered I/O aur `io_uring` dono isi cost ko amortize/avoid karne ki strategies hain.

**Practice task(s):**
- Apne alfaazon mein bolke explain karo: `syscall` instruction ke chalne par CPU/kernel mein step-by-step kya hota hai jab tak control wapas user program ko nahi milta.
- `strace -c` kisi bhi apne locally chal rahe program (ya `curl` ek URL par) par chala kar dekho sabse zyada time kaunse syscalls mein ja raha hai.
- Ek chhota experiment likho: same file 100,000 baar 1-byte `write()` se likhna vs buffered writer (jaise Go ka `bufio.Writer` ya C ka `fwrite`) se likhna — time compare karo.

---

### Subtopics — practical aha points

**Thread-per-request vs event loop vs async runtimes vs goroutines/green threads:** Ye chaar models same problem (concurrency) ke chaar alag tradeoffs hain. Thread-per-request (classic Java Tomcat) simplest hai code likhne mein (synchronous-looking code) par memory + context-switch cost ki wajah se ~5-10k connections ke baad struggle karta hai. Event loop (Nginx, Redis, `libuv`/Node.js) ek single thread par `epoll` se hazaaron connections multiplex karta hai — bahut scalable I/O ke liye, par agar koi ek callback CPU-bound kaam kare (jaise heavy JSON parsing without yielding), to poora loop block ho jaata hai, sab connections ke liye. Async runtimes (Python `asyncio`, JS `async`/`await`, Rust `tokio`) event loop ke upar ek syntax layer daalte hain taaki code synchronous jaisa dikhe. Goroutines (Go) **M:N scheduling** se best of both deti hain — synchronous-looking code, par kam OS threads par multiplexed, aur runtime khud blocking syscalls ko handle kar leta hai (`GOMAXPROCS` OS threads, unlimited goroutines). System design mein ye choice seedha batati hai ki ek service kitne concurrent clients handle kar sakti hai per instance, aur CPU-bound vs I/O-bound workload ke liye kaunsa model sahi hai.

**The C10K → C10M problem aur usko asal mein kisne solve kiya:** 1999 mein Dan Kegel ne "C10K problem" likha — ek server 10,000 concurrent connections kyun handle nahi kar pa raha tha? Root cause tha thread-per-connection model (memory + context switch) aur `select()`'s O(n) scan + 1024-fd limit. Solution tha **`epoll`** (Linux, 2002 ke aas-paas) — O(1)-ish multiplexing jisne C10K ko routine bana diya (aaj koi bhi Nginx box aasaani se 10k+ connections handle karta hai). **C10M** (10 million connections, ek hi box par) ek alag league hai — isme sirf multiplexing kaafi nahi, poora networking stack rethink karna padta hai: kernel bypass (`DPDK`), zero-copy, NUMA-aware design, per-core sharding, interrupt coalescing. Practically, zyadatar backend systems ko kabhi C10M tak pahunchne ki zaroorat nahi padti — horizontal scaling (zyada machines) usually saster hai — C10M sirf extreme cases (CDN edge, trading systems, DDoS mitigation boxes) mein matter karta hai.

**`ulimit`, file descriptors as a hard scaling limit, ephemeral port exhaustion (~28k per tuple):** Linux mein har open socket, file, pipe ek **file descriptor** consume karta hai, aur har process ka ek max limit hota hai (`ulimit -n`, default aksar sirf `1024` hota hai — production mein `65535` ya usse zyada set karna common practice hai). Ye limit hit hone par app ko `EMFILE`/"too many open files" error milta hai — ye ek bahut real, bahut common production outage cause hai. **Ephemeral port exhaustion** ek related lekin alag limit hai: jab tumhari service ek destination (jaise ek DB) ko outbound TCP connections banati hai, har connection ek unique (`source_ip, source_port, dest_ip, dest_port`) tuple leta hai — default ephemeral port range (`/proc/sys/net/ipv4/ip_local_port_range`, typically ~32768-60999) ~28,000 ports deta hai. Agar bahut saare short-lived connections banate rehte ho (bina connection pooling ke) ek hi destination ko, to ye ~28k limit hit ho sakti hai — isliye connection pooling (jaise Postgres ke liye `PgBouncer`, ya HTTP keep-alive) sirf performance optimization nahi, ek **hard scaling necessity** hai.

**cgroups & namespaces — containers ke peeche ka asli mechanism:** "Container" naam ka koi single kernel object nahi hota — Docker/containerd jaise tools do independent Linux kernel primitives ko combine karte hain. **cgroups** (control groups) resource **limiting aur accounting** karte hain — kitna CPU, memory, disk I/O, PIDs ek group of processes use kar sakta hai. **Namespaces** isolation karte hain — process ko **dikhta** kya hai — PID namespace (apna PID 1 dikhta hai), network namespace (apna network stack/interfaces), mount namespace (apna filesystem view), UTS namespace (apna hostname), user namespace (UID mapping — container ke andar "root" host par non-root ho sakta hai). In dono ke upar ek union filesystem (`overlayfs`) images ke layers ko combine karta hai. `docker run` karte waqt asal mein `clone()` ko kai `CLONE_NEW*` flags ke saath call kiya jaata hai — ye samajhna "containers lightweight VMs hain" jaisi galat mental model ko theek karta hai.

**CPU throttling in containers (CFS quota) aur ye p99 latency kyun destroy karta hai:** Kubernetes ek container ki CPU **limit** (jaise `cpu: "2"`) ko Linux ke **CFS bandwidth control** se enforce karta hai — har `cpu.cfs_period_us` (typically 100ms) window mein container ko `cpu.cfs_quota_us` microseconds ka CPU time milta hai (2 CPUs ka limit = 200ms quota per 100ms period). Problem ye hai: agar container apni saari quota window ke pehle hi hissa mein burst kar ke use kar le (jaise ek GC pause, ya ek momentary spike jahan bahut saare goroutines/threads parallel chal rahe the), to wo **poore bache hue period ke liye completely throttled/frozen** ho jaata hai — chahe node par doosre cores idle hi kyun na baithe hon. Ye ek well-documented real-world p99 latency killer hai (kai companies ke engineering blogs isse discover karke likh chuke hain) — average CPU usage graph mein sab normal (jaise 30%) dikhega, par requests periodically 80-100ms tak freeze ho rahi hongi. Fix: limits hata do ya generous rakho, ya modern kernels/cgroup v2 ke behavior ko samjho.

---

### Advanced concepts

**`fsync` semantics, write barriers, disk cache lies, `O_DIRECT`, and how databases achieve durability:** `fsync` ki guarantee tabhi asli hai jab underlying disk **honest** ho — bahut saare consumer-grade SSDs/HDDs apne onboard **volatile write cache** mein data rakh kar OS ko bol dete hain "flush ho gaya" (benchmarks mein fast dikhne ke liye), jabki asal mein wo data ab bhi disk ke apne RAM mein hai aur power-loss mein gayab ho sakta hai — is wajah se enterprise storage/databases battery-backed write cache ya properly-honored `FLUSH`/`FUA` commands par depend karte hain. **Write barriers** filesystem/block-layer ka mechanism hai jo guarantee karta hai ki barrier se pehle ki writes barrier ke baad ki writes se pehle hi disk tak pahunchengi (journaling correctness ke liye zaroori). `O_DIRECT` flag page cache ko bilkul bypass kar deta hai — app apna khud ka buffering/caching manage karta hai (Oracle default isi ka use karta hai, Postgres experimental support rakhta hai) — fayda: double-buffering avoid hoti hai (data page cache **aur** DB buffer pool dono mein cached na ho), aur app ko write timing par zyada control milta hai; cost: alignment requirements (typically 512B/4KB) manually handle karne padte hain. Real databases durability isi combination se achieve karte hain: WAL + `fsync`/`fdatasync` (kabhi `O_DIRECT`/`O_DSYNC` ke saath) — theory se production tak ka poora path.

**Copy-on-write, `fork` semantics, aur Redis `BGSAVE` memory double kyun kar sakta hai:** `fork()` jab ek naya child process banata hai, to poora parent memory turant copy **nahi** hoti — dono processes same physical pages ko **read-only, shared** mode mein point karte hain (**copy-on-write**, COW). Jab koi bhi process (parent ya child) us memory mein **write** karta hai, tabhi ek page fault trigger hoti hai aur kernel sirf **us ek page** ko actually copy karta hai. Redis ka `BGSAVE` isi trick se ek pura huge in-memory dataset ka consistent snapshot bina duplicate memory allocate kiye instantly le leta hai — `fork()` fast hai chahe dataset 50GB ho. Lekin agar parent process (jo abhi bhi client requests serve kar raha hai) `BGSAVE` chalte-chalte bahut sara data **write/modify** kare, to har modified page ko COW-copy karna padta hai (taaki child ka "frozen" view sahi rahe) — high write-rate workload par ye worst case mein **memory usage ko nearly double** kar sakta hai, aur agar system already memory-constrained hai to OOM ka risk create karta hai. Isi wajah se production Redis deployments `BGSAVE`/AOF rewrite ke time memory headroom explicitly plan karte hain.

**Huge pages, transparent huge pages, aur unke latency side effects:** Default page size 4KB hai — bade memory regions (jaise ek 32GB JVM heap) ke liye millions of pages honge, aur TLB mein sirf limited entries fit hoti hain, is wajah se TLB miss rate badh jaata hai. **Huge pages** (2MB ya 1GB size) ek hi TLB entry se bahut zyada memory cover karte hain — TLB misses drastically kam ho jaate hain, large-memory apps (JVMs, in-memory databases) ke liye significant speedup. **Transparent Huge Pages (THP)** Linux ka feature hai jo automatically, bina app-level changes ke, regular pages ko huge pages mein promote karta hai — lekin iska background process `khugepaged` jo memory **compact/defragment** karta hai (contiguous huge pages banane ke liye) unpredictable, sometimes-multi-millisecond latency spikes create kar sakta hai bilkul random waqt par. Isi wajah se Redis, MongoDB, aur Postgres ki official production documentation explicitly THP disable karne ki recommend karti hai (`echo never > /sys/kernel/mm/transparent_hugepage/enabled`) — predictable latency, manual huge-page allocation se behtar hai unpredictable "automatic" optimization se.

**Kernel bypass: DPDK, SR-IOV, user-space TCP; ye kab matter karta hai (trading, CDNs):** Normal networking path mein ek incoming packet NIC se aata hai, ek hardware interrupt fire hoti hai, kernel packet ko apne network stack mein process karta hai (`sk_buff` allocation, routing decisions, netfilter/iptables rules, socket buffer copy), tab jaake user-space app usse `read()` kar paata hai — har packet par ye poora overhead lagta hai, aur bahut high packet rates (millions/sec) par interrupts khud hi CPU ko overwhelm kar dete hain. **DPDK** (Data Plane Development Kit) is poore kernel network stack ko bypass kar deta hai — ek user-space poll-mode driver (via UIO/VFIO) directly NIC se packets kheenchta hai, koi interrupt nahi (continuously polling), koi syscall nahi, aur app apna khud ka lightweight packet-processing/TCP implement karta hai — massively higher throughput, aur predictable, bahut low latency. **SR-IOV** (Single Root I/O Virtualization) physical NIC ko multiple virtual PCIe functions mein split karta hai jo directly VMs ko assign ho sakte hain, hypervisor ke virtual switch ko bypass karke near-native networking performance dete hain. Ye tab matter karta hai jab microseconds count hote hain — high-frequency trading systems, CDN edge/load-balancer nodes jo millions packets/sec handle karte hain (jaise kuch AWS Nitro-based instances SR-IOV use karte hain), aur NFV middleboxes. Ek normal CRUD backend service ke liye ye **bilkul zaroori nahi** hai — massive complexity cost ke against bahut kam fayda, is baat ko samajhna bhi utna hi important hai jitna ye samajhna ki ye kaise kaam karta hai.

---

### Important terms

- **Process:** Apna independent virtual address space rakhne wala execution unit; isolation ki base unit hai OS mein.
- **Thread:** Process ke andar ek execution unit jo address space/heap share karta hai par apna stack/registers rakhta hai.
- **Coroutine / green thread / goroutine:** User-space (runtime-managed) execution unit jise kernel directly nahi jaanta — kam OS threads par multiplex hoti hai.
- **Scheduler / CFS:** Kernel component jo decide karta hai kaunsa thread kab kaunse core par chalega; Linux default CFS (Completely Fair Scheduler) hai.
- **Context switch:** Ek thread/process ko rok kar doosre ko chalana — registers save/restore, kabhi page table switch, ~1-5 µs cost.
- **Virtual memory:** Per-process memory ki illusion jo MMU + page tables ke through physical RAM par map hoti hai.
- **Page table / TLB:** Page table virtual→physical mapping rakhta hai (multi-level); TLB uska fast hardware cache hai.
- **Page fault (minor/major):** Minor = mapping missing par page RAM mein hai (sasta); major = page disk se laana padega (mehenga).
- **Swap:** RAM kam padne par kuch pages ko disk par move karna — latency ke liye bahut costly.
- **OOM killer:** Jab memory bilkul reclaim na ho paaye, kernel ka mechanism jo kisi process ko force-kill karta hai.
- **Inode:** File ka metadata structure (size, permissions, block pointers) — naam se independent.
- **Dentry:** Directory entry jo filename ko inode se map karta hai.
- **Page cache:** OS ka transparent in-RAM cache disk file pages ka; reads/writes ko fast banata hai.
- **Dirty page:** Page cache mein modify hui par abhi disk par flush nahi hui memory page.
- **`fsync`/`fdatasync`:** Syscall jo dirty pages ko forcefully durable storage tak flush karwata hai.
- **Journaling:** Filesystem technique jahan changes pehle ek sequential log mein likhi jaati hain crash-consistency ke liye.
- **WAL (Write-Ahead Log):** Database ka apna journal — commit se pehle change log mein likha jaata hai, durability ke liye.
- **Write barrier / FUA:** Ordering guarantee jo ensure karta hai ki writes sahi sequence mein durable storage tak pahunchein.
- **`O_DIRECT`:** File open flag jo page cache bypass karta hai — app apna buffering khud manage karta hai.
- **Blocking / non-blocking I/O:** Blocking = thread ruk jaata hai data ke liye; non-blocking = turant return, caller retry karta hai.
- **`select`/`poll`/`epoll`/`kqueue`:** I/O multiplexing mechanisms — ek thread se multiple fds monitor karna; `epoll`/`kqueue` scale karte hain, `select`/`poll` nahi.
- **Level-triggered vs edge-triggered:** `epoll` ke modes — level-triggered baar-baar ready bolta hai jab tak data hai; edge-triggered sirf state-change par ek baar.
- **`io_uring`:** Linux ka async I/O interface jo shared ring buffers se syscall-per-operation overhead bhi minimize karta hai.
- **SQ / CQ (Submission/Completion Queue):** `io_uring` ke do ring buffers jahan operations submit hoti hain aur results milte hain.
- **Signal-driven I/O (`SIGIO`):** Kernel fd-ready hone par ek signal bhejta hai — rarely used in practice.
- **System call (syscall):** User-space se kernel-space mein controlled entry point kisi privileged operation ke liye.
- **Ring 0 / Ring 3:** CPU privilege levels — ring 0 kernel mode (full access), ring 3 user mode (restricted).
- **vDSO:** Kernel-provided functions jo process ke address space mein directly mapped hote hain, full trap avoid karne ke liye.
- **KPTI (Kernel Page Table Isolation):** Meltdown mitigation jo har user↔kernel transition par TLB flush karta hai — syscall cost badhata hai.
- **C10K / C10M:** Ek server par 10,000 / 10,000,000 concurrent connections handle karne ki classic scaling problems.
- **`ulimit`:** Per-process resource limits (jaise open file descriptors ka max count).
- **Ephemeral port:** Outbound connections ke liye kernel-assigned temporary source port (typically ~28k range).
- **cgroups:** Kernel mechanism jo processes ke resource usage (CPU, memory, I/O) ko limit/measure karta hai.
- **Namespaces:** Kernel mechanism jo processes ko isolated view deta hai (PID, network, mount, UTS, user, etc.).
- **CFS quota/period:** Container CPU limit ka mechanism — per time-window (period) mein allowed CPU time (quota).
- **CPU throttling:** Jab container apni CFS quota use kar leta hai aur period ke bache hue time ke liye pause ho jaata hai.
- **Copy-on-write (COW):** Memory-sharing optimization — pages tabhi copy hote hain jab unpar write ho.
- **`fork`:** Syscall jo ek naya child process banata hai, COW semantics ke saath.
- **`BGSAVE`:** Redis ka background snapshot mechanism jo `fork` + COW use karta hai.
- **Huge pages / THP:** Bade page sizes (2MB/1GB) jo TLB miss rate kam karte hain; THP automatic version hai jiske latency side effects hote hain.
- **`khugepaged`:** Kernel thread jo background mein memory compact karke huge pages banata hai — latency spikes ka source.
- **DPDK:** User-space kernel-bypass networking toolkit, poll-mode NIC drivers ke saath.
- **SR-IOV:** Hardware-level NIC virtualization jo VMs ko near-native network performance deta hai.
- **Kernel bypass:** Networking/storage stack ko user-space se seedha access karna, kernel ka overhead avoid karke.

---

### Common mistakes

- Har connection ke liye ek naya OS thread spawn karna (classic thread-per-request model) aur phir 5-10k concurrent users par server "achanak" slow/crash hone par confuse hona — root cause context-switch + per-thread stack memory hai.
- `write()` ka successfully return hona hi "data safe hai" samajh lena, `fsync` ya framework ki durability guarantee explicitly verify kiye bina — crash/power-loss par "acknowledged" data gayab ho jaata hai.
- Ek disk/RAID controller par blindly trust karna ki `fsync` = durable, jabki us disk ka volatile write cache honest flush signal ko honor hi nahi karta — "hamne fsync call kiya" false sense of security deta hai.
- Production mein default `ulimit -n 1024` chhod dena aur traffic spike ke waqt achanak "too many open files" errors milna — ek bahut common first-deployment mistake.
- Kubernetes mein CPU `limits` set karte waqt CFS quota granularity (per-100ms window) ko na samajhna, phir average CPU usage graph "normal" dikhne ke bawajood p99 latency spikes se confuse hona.
- Transparent Huge Pages ko production latency-sensitive datastore (Redis, MongoDB) par default-enabled chhod dena aur `khugepaged`-driven random latency spikes ko "network issue" samajh lena.
- Bahut saari app instances ek hi NAT gateway/single source IP ke peeche se ek hi downstream service ko short-lived connections banate rehna, bina pooling ke — burst traffic mein ephemeral port exhaustion (~28k) hit karna, jo sirf load ke under hi dikhta hai.
- Monitoring/alerting mein VSZ (virtual size) aur RSS (actual resident memory) ko confuse karna — `mmap`-heavy process ka VSZ bahut bada dikh sakta hai bina actual RAM zyada use kiye, jisse galat capacity decisions ya false OOM alarms milte hain.

---

### Interview questions

1. Process, thread, aur coroutine mein kya fundamental fark hai, aur kis scenario mein kaunsa choose karoge?
   *Hint: address-space sharing, creation cost, aur scheduling kisne (kernel vs runtime) kiya — Go goroutines ka M:N model specifically mention karo.*
2. Context switch ke dauran exactly kya hota hai, aur iski cost ~1-5 µs kyun hai?
   *Hint: register save/restore, page table switch (agar process change), TLB/cache cold-hone ka effect — ye sab layers hint mein bolo.*
3. `epoll` `select`/`poll` se scale karne mein kyun behtar hai?
   *Hint: O(n) full scan har call vs kernel-maintained ready-list + interest tree, aur `select` ka 1024-fd limit.*
4. Minor aur major page fault mein fark batao, aur OOM killer kab trigger hota hai?
   *Hint: minor = mapping missing par page RAM mein hai; major = disk se laana; OOM = reclaim ke baad bhi memory na milne par heuristic-based kill.*
5. `fsync` call karne ke baad bhi data loss kaise ho sakta hai kuch hardware par?
   *Hint: disk ka apna volatile write cache jo FLUSH/FUA command ko honestly honor nahi karta — "disk ne jhooth bola" scenario.*
6. cgroups aur namespaces mein kya fark hai, aur ye dono milkar "container" kaise banate hain?
   *Hint: cgroups = resource limiting/accounting; namespaces = isolation/visibility; Docker in dono + overlayfs ko combine karta hai.*
7. Ek Kubernetes pod jiski CPU limit 2 hai, wo node par CPU idle hone ke bawajood throttle kyun ho sakta hai?
   *Hint: CFS bandwidth control — per-100ms period mein fixed quota, jaldi burst karne par bacha hua period completely frozen.*
8. Redis ka `BGSAVE` memory usage kabhi-kabhi double kyun kar sakta hai?
   *Hint: `fork()` + copy-on-write, high write rate during snapshot par har modified page copy honi padti hai.*
9. C10K problem kya tha aur ise kaise solve kiya gaya — aur C10M ke liye extra kya chahiye?
   *Hint: thread-per-connection + `select` ki limits vs `epoll`; C10M ke liye kernel bypass, zero-copy, per-core sharding.*
10. Ephemeral port exhaustion kya hai aur backend architecture mein ise kaise mitigate karte ho?
    *Hint: source-port range ~28k limit ek destination ke liye; connection pooling (PgBouncer, HTTP keep-alive), multiple source IPs/NAT scaling.*

---

### Hands-on lab

> Write a TCP echo server three ways — thread-per-connection, `epoll` event loop, async runtime. Load test all three to find the knee.

**Ye specific lab kyun:** Ye poori section ka theory-to-practice bridge hai — teeno concurrency models (thread-per-connection, `epoll`-based event loop, aur ek async runtime jaisa Go ka goroutine model ya Python `asyncio`) same kaam (TCP echo) same machine par karenge, aur load test se tumhe apni khud ki "knee point" (wo concurrency level jahan latency/throughput achanak degrade hone lagta hai) milegi har model ke liye. Ye number khud dekhna — sirf padhna nahi — hi context-switch cost, file-descriptor limits, aur scheduler overhead ko abstract facts se real, measured engineering knowledge mein badal deta hai. Aage ke levels (concurrency patterns, networking, aur scaling decisions) isi lab ke numbers ko implicitly reference karenge.

- [ ] Mera result: Thread-per-connection knee = ______ connections (latency/throughput: ______), `epoll` event loop knee = ______ connections (latency/throughput: ______), Async runtime knee = ______ connections (latency/throughput: ______)

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.5 Concurrency & parallelism

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har topic ke saath: **kya hai (what) → kyun important hai (why) → kaise kaam karta hai (how) → real-world aur backend/system-design example.**

### 1. Concurrency vs parallelism; Amdahl's law; Gustafson's law

**Kya hai (What):** **Concurrency** ka matlab hai multiple tasks ko *structure* karna taaki wo overlapping time-window mein progress karein — zaroori nahi ki wo ek hi exact instant par chal rahe hon. **Parallelism** ka matlab hai literally ek hi instant par, alag-alag physical CPU cores par, multiple tasks ka *simultaneous* execution. Ek single-core CPU bhi concurrency de sakta hai (OS scheduler bahut tezi se context-switch karke ek illusion banata hai ki sab kuch saath chal raha hai), lekin true parallelism ke liye multiple cores chahiye hi hote hain. Isliye "concurrent" aur "parallel" synonyms nahi hain — concurrency ek **design property** hai (kaise tasks decompose kiye gaye), parallelism ek **runtime property** hai (kya sach mein hardware par ek saath execute ho raha hai). **Amdahl's law** batata hai ki agar tumhare program ka fraction `P` parallelize ho sakta hai aur `(1-P)` strictly serial hai, to `N` cores ke saath max speedup hai `S(N) = 1 / ((1-P) + P/N)` — aur `N → ∞` par ye ek hard ceiling `1/(1-P)` par saturate ho jata hai. **Gustafson's law** iska optimistic mirror hai: agar problem size bhi cores ke saath badhti jaaye (real-world mein data volume aksar aisa hi hota hai), to speedup `S(N) = N - α(N-1)` hota hai, jaha `α` serial overhead ka fraction hai — aur ye near-linear scale karta hai.

**Kyun important hai (Why):** Ye samajh na hona sabse common capacity-planning mistake ki jad hai: "bas zyada cores/workers/machines laga do, throughput badh jayega." Amdahl's law batata hai ki agar tumhara workload ek chhoti si serial cheez (jaise ek global mutex, ek single-threaded aggregation step, ek shared connection) par bottleneck hai, to 4 se 400 cores tak jaane par bhi returns severely diminish ho jate hain — aur kabhi-kabhi contention ki wajah se performance *girti* bhi hai. Gustafson's law explain karta hai ki bade data/HPC workloads (Spark jobs, distributed training) mein cores add karne se near-linear scaling kyun milti hai — kyunki wahaan problem size bhi saath scale hoti hai.

**Kaise kaam karta hai (How):** Practically, pehle apne workload ka serial fraction identify karo — locks, single-threaded I/O waits, initialization code, shared-state aggregation steps. Phir Amdahl's formula se plug-in karke dekho ki adding cores ka marginal benefit kab flatten ho raha hai. Production mein ye typically profiling se nikalta hai (flame graphs mein "lock wait" ya "single-threaded section" time dikhta hai), theoretical calculation se nahi.

**Real-world example aur backend/system-design connection:** Ek REST API jisme request handling 95% parallelizable hai (independent per-request work) lekin sabhi requests ek shared, single-threaded audit-logger ko block karke likhte hain — ye 5% serial fraction Amdahl's ceiling ko `1/0.05 = 20x` par cap kar deta hai, chahe tum 200 worker threads laga do. Isi wajah se high-throughput systems (Kafka producers, Nginx worker processes) shared mutable state ko jitna ho sake avoid karte hain ya use per-worker/per-shard bana dete hain. System design mein, database read-replica fan-out ka scaling Gustafson-jaisa hota hai (zyada replicas = zyada parallel reads, data bhi usi scale par badh raha hai), lekin agar final aggregation ek single node par ho rahi hai, wo Amdahl ka bottleneck ban jata hai.

```
Amdahl's Law:  S(N) = 1 / ( (1 - P) + P/N )

Workload: P = 0.9 (90% parallelizable), (1-P) = 0.1 (10% strictly serial)

  N = 2 cores:   S = 1 / (0.10 + 0.90/2)   = 1 / 0.550   = 1.82x
  N = 4 cores:   S = 1 / (0.10 + 0.90/4)   = 1 / 0.325   = 3.08x
  N = 16 cores:  S = 1 / (0.10 + 0.90/16)  = 1 / 0.15625 = 6.40x
  N = infinity:  S -> 1 / (1 - P) = 1 / 0.1 = 10x   <-- hard ceiling, chahe N = 1000 ho

Gustafson's Law:  S(N) = N - alpha * (N - 1)   [problem size scales with N]

Workload: alpha = 0.05 (5% serial overhead, roughly constant as data grows)

  N = 16 cores:  S = 16 - 0.05 * 15 = 16 - 0.75 = 15.25x   <-- near-linear!
```

**Quick summary:** Concurrency = structure/design, parallelism = actual simultaneous execution. Amdahl's law caps speedup based on fixed serial fraction; Gustafson's law shows near-linear scaling jab problem size bhi grow karta hai — dono ek hi coin ke do pehlu hain.

**Practice task(s):**
- Apne kisi recent multi-threaded/multi-worker code mein serial fraction identify karo out loud — kaunsa hissa strictly single-threaded hai?
- Amdahl's formula se calculate karo: agar tumhara workload sirf 60% parallelizable hai, to 8 cores par max theoretical speedup kya hoga? 64 cores par?

---

### 2. Race conditions, critical sections, mutual exclusion

**Kya hai (What):** **Race condition** tab hoti hai jab do ya zyada threads shared, mutable data ko bina proper synchronization ke simultaneously read-modify-write karte hain, aur final outcome *timing/interleaving* par depend karta hai — yani same code, alag runs mein alag result de sakta hai. **Critical section** program ka wo specific hissa hai jo shared resource (variable, data structure, file, connection) ko access/modify karta hai. **Mutual exclusion** ek guarantee hai ki ek waqt sirf ek thread hi critical section ke andar ho sakta hai — baaki sab wait karenge.

**Kyun important hai (Why):** Race conditions sabse dangerous bugs ki category mein aate hain kyunki wo **non-deterministic** hote hain — dev machine par, low load par, single-request testing mein sab theek dikhta hai, lekin production mein high concurrency ke saath silently data corrupt ho jata hai: lost updates, inconsistent reads, duplicate charges. Ye "heisenbugs" hote hain — debugger attach karte hi timing badal jati hai aur bug reproduce hona band ho jata hai.

**Kaise kaam karta hai (How):** Classic example `counter++` hai — ye ek hi CPU instruction nahi hai, balki teen steps hain: (1) memory se current value **load** karo register mein, (2) register mein **add** karo, (3) naya value memory mein **store** karo wapas. Agar thread A load karke add kar raha hai, aur usi beech thread B bhi load-add-store kar leta hai, to thread A ka store thread B ke update ko overwrite kar dega — ek update **lost** ho jayega. Fix: critical section ke around mutual exclusion lagao (lock) taaki poora load-add-store ek atomic unit ki tarah behave kare.

**Real-world example aur backend/system-design connection:** Bank account balance ko `balance = balance + amount` se update karna bina lock ke — do simultaneous deposits mein se ek "lost update" ho sakta hai. Backend mein: Go ka `go run -race` race detector, ya Java ka concurrent stress-testing tool aise races catch karte hain. System design mein, jab bhi multiple app-server instances ek shared counter (jaise "remaining inventory count") ko directly increment/decrement karte hain, wahi race condition distributed scale par hoti hai — isiliye production systems `SELECT ... FOR UPDATE`, atomic DB operations (`UPDATE inventory SET count = count - 1 WHERE id = ? AND count > 0`), ya distributed locks use karte hain instead of read-then-write patterns.

```go
// BUGGY: classic race condition
var counter int

func worker() {
    for i := 0; i < 1000; i++ {
        counter++ // load, add, store — 3 steps, NOT atomic
    }
}
// 100 goroutines chalao, expected counter = 100000
// actual result: kam milega, aur run-to-run alag hoga
// detect karne ke liye: go run -race ./buggy.go  -> "DATA RACE" warning

// FIXED: mutual exclusion se critical section protect karo
var mu sync.Mutex
var safeCounter int

func safeWorker() {
    for i := 0; i < 1000; i++ {
        mu.Lock()
        safeCounter++   // ab poora load-add-store atomic unit hai
        mu.Unlock()
    }
}
```

**Quick summary:** Race condition = timing-dependent bug jab shared state bina protection ke access hoti hai; critical section wo risky code-zone hai; mutual exclusion usko safe banati hai. `x++` jaisa "simple" operation bhi multi-step hota hai, isiliye atomic nahi hota by default.

**Practice task(s):**
- Upar wala buggy Go snippet (ya apni pasandida language mein equivalent) khud likho, 100 goroutines/threads ke saath chalao, aur dekho counter kya deta hai — expected se kitna kam?
- `-race` flag (Go) ya equivalent thread-sanitizer apni language mein try karo, output padho aur samjho kya report karta hai.

---

### 3. Locks: mutex, spinlock, read-write lock, reentrant lock

**Kya hai (What):** **Mutex** (mutual exclusion lock) ek **blocking** lock hai — agar lock already liya hua hai, requesting thread **sleep** ho jata hai (OS scheduler use suspend kar deta hai) aur jab lock free hota hai tab wake hota hai. **Spinlock** ek **busy-wait** lock hai — thread sleep nahi hota, ek tight loop mein baar-baar check karta rehta hai ki lock free hua ya nahi, CPU cycles "spin" karte hue burn karta hai, koi context switch nahi hota. **Read-write lock** (RWLock/RWMutex) do modes deta hai — multiple readers **simultaneously** allowed hain (jab tak koi writer nahi hai), lekin writer ko **exclusive** access chahiye (na koi reader, na koi doosra writer saath mein). **Reentrant lock** (recursive lock) wo lock hai jise **same thread** multiple baar acquire kar sakta hai bina khud ko deadlock kiye — internally lock ek "owner thread ID" aur "acquire count" track karta hai.

**Kyun important hai (Why):** Galat lock type choose karna throughput ko seriously hurt kar sakta hai. Spinlock agar single-core machine par ya long critical section ke liye use ho, to CPU sirf waste hota hai (jabki mutex thread ko sleep karake CPU doosre kaam ke liye free kar deta). Lekin multi-core machine par bahut chhoti critical sections ke liye spinlock mutex se **faster** hota hai kyunki context-switch ka overhead (kernel involvement, scheduling) avoid ho jata hai. RWLock read-heavy workloads (jaise config cache, ya "reads >> writes" data) ko dramatically improve karta hai kyunki readers ek doosre ko block nahi karte.

**Kaise kaam karta hai (How):** Linux par mutex typically **futex** (fast userspace mutex) se implement hota hai — **fast path**: agar lock uncontended hai, purely userspace mein ek atomic compare-and-swap se lock mil jata hai, koi syscall nahi. **Slow path**: agar already locked hai, tabhi kernel ko call karte hain (`futex_wait`) jo thread ko sleep karake CPU deta hai kisi aur ko, aur jab unlock hota hai `futex_wake` us thread ko wapas jagata hai. Spinlock bas ek loop hai jo CAS try karta rehta hai. RWLock internally do counters/states rakhta hai — reader count aur writer flag.

**Real-world example aur backend/system-design connection:** Postgres apne internal "lightweight locks" (LWLocks) mein ek hybrid approach use karta hai — pehle thodi der spin karta hai (assume karke ki lock jaldi free hoga), phir agar nahi mila to proper sleep mein chala jata hai — best of both worlds. Linux kernel interrupt-handling code mein spinlocks use karta hai kyunki interrupt context mein sleep karna allowed hi nahi hai. Java mein `synchronized` keyword aur `ReentrantLock` dono reentrant hain (same thread nested lock le sakta hai), lekin Go ka `sync.Mutex` **reentrant nahi hai** — ye ek bahut common porting mistake hai Java se Go aane walon ke liye.

```c
// simplified futex-based mutex — Linux
int lock(mutex_t *m) {
    // fast path: uncontended, pure userspace, no syscall
    if (atomic_compare_and_swap(&m->state, UNLOCKED, LOCKED))
        return 0;                       // instantly mil gaya, cheap

    // slow path: already locked -> kernel se sleep maango
    // jab tak owner unlock() par futex_wake() na kare
    return futex_wait(&m->state, LOCKED);
}
```

```go
// spinlock vs mutex vs RWMutex — Go flavour
var rw sync.RWMutex
var cache map[string]string

func readCache(k string) string {
    rw.RLock()            // multiple readers ek saath allowed
    defer rw.RUnlock()
    return cache[k]
}

func writeCache(k, v string) {
    rw.Lock()              // exclusive — sabhi readers ke khatam hone ka wait karta hai
    defer rw.Unlock()
    cache[k] = v
}
```

**Quick summary:** Mutex = sleep-and-wait (CPU-friendly, syscall overhead), spinlock = busy-wait (fast for tiny critical sections on multi-core, wasteful otherwise), RWLock = readers-share/writers-exclusive, reentrant = same-thread-safe re-acquire.

**Practice task(s):**
- Explain karo out loud: ek high-read, low-write in-memory cache ke liye tum RWMutex kyun choose karoge plain Mutex ke bajaye?
- Apni language mein reentrant lock (agar available hai) le kar same thread se do baar `Lock()` call karke dekho — mutex (non-reentrant) mein kya hota hai vs reentrant lock mein kya hota hai?

---

### 4. Deadlock (4 Coffman conditions), livelock, starvation, priority inversion

**Kya hai (What):** **Deadlock** ek situation hai jaha do ya zyada threads ek **cycle** mein ek doosre ke resource (lock) ka wait kar rahe hain, aur koi bhi aage nahi badh sakta — system permanently freeze ho jata hai us portion mein. Ye 4 **Coffman conditions** ek saath hone par hi ho sakta hai: (1) **mutual exclusion** — resource ek time par sirf ek thread use kar sakta hai, (2) **hold and wait** — thread ek resource pakde hue hai aur doosre ka wait kar raha hai, (3) **no preemption** — koi resource forcefully thread se le nahi sakta, (4) **circular wait** — threads ka ek cycle hai jaha har ek agle ka resource maang raha hai. Agar in charon mein se koi ek bhi condition break ho jaye, deadlock impossible hai. **Livelock** alag hai — threads **stuck nahi** hain, wo actively state change kar rahe hain ek doosre ke response mein, lekin overall **koi progress nahi** ho raha (jaise do log corridor mein dono baar-baar same side step karte hain ek doosre ko avoid karne ke liye). **Starvation** — ek thread ko resource **kabhi nahi milta** kyunki baaki threads hamesha priority mein aage rehte hain (scheduling unfairness). **Priority inversion** — ek low-priority thread ek lock hold kiye hue hai jo high-priority thread ko chahiye, lekin beech mein ek medium-priority thread low-priority thread ko preempt kar deta hai — net result: high-priority thread indefinitely wait karta hai, jabki uski priority sabse zyada thi.

**Kyun important hai (Why):** Deadlock production ka ek classic "silent freeze" symptom hai — app hang ho jata hai lekin CPU usage **zero ke paas** hota hai (infinite loop se different, jaha CPU 100% hota hai). 4 Coffman conditions samajhna practical hai kyunki tum in mein se sirf **ek** condition tod kar poori class of bugs eliminate kar sakte ho — typically "circular wait" todna sabse practical hai (consistent lock ordering se). Priority inversion famous hai kyunki ye ek real spacecraft mission ko crash hote-hote bacha — **Mars Pathfinder (1997)** mission mein exactly ye bug hua tha: low-priority meteorological data thread ek shared bus lock hold kar raha tha, medium-priority communications task use preempt kar raha tha, aur high-priority bus-management task starve ho raha tha — jisse system periodic watchdog resets kar raha tha. Fix hua **priority inheritance** protocol se (lock holder temporarily high-priority thread ki priority le leta hai jab tak wo lock release nahi karta).

**Kaise kaam karta hai (How):** Deadlock avoid karne ka sabse practical tareeka hai **consistent lock ordering** — hamesha locks ko same global order mein acquire karo (jaise thread ID ya memory address ke basis par sort karke), taaki circular wait bane hi na. Doosra approach: **try-lock with timeout** — agar lock N milliseconds mein nahi mila, backoff karke retry karo instead of blocking indefinitely. Diagnose karne ke liye **thread dump / goroutine dump** lo — ye har thread ka current stack trace dikhata hai, jisse pata chalta hai kaun kis lock ka wait kar raha hai, aur cycle visually dikh jata hai.

**Real-world example aur backend/system-design connection:** Do database transactions jo alag order mein do rows lock karte hain (transaction A: row1 phir row2; transaction B: row2 phir row1) — Postgres aur MySQL dono ke paas built-in **deadlock detector** hai jo cycle detect karke ek transaction ko forcefully abort kar deta hai (`ERROR: deadlock detected`), taaki system freeze na ho. System design mein, microservices ke beech synchronous circular calls (Service A → Service B → Service A, sab blocking calls) bhi ek distributed deadlock jaisa pattern create kar sakte hain agar connection pools exhaust ho jayein.

```go
// classic deadlock: inconsistent lock ordering across two goroutines
var mu1, mu2 sync.Mutex

func transferAtoB() {
    mu1.Lock()
    defer mu1.Unlock()
    time.Sleep(time.Millisecond) // race window widen karne ke liye
    mu2.Lock()
    defer mu2.Unlock()
    // ... balance move A -> B
}

func transferBtoA() {
    mu2.Lock()             // <-- reverse order! yahi bug hai
    defer mu2.Unlock()
    time.Sleep(time.Millisecond)
    mu1.Lock()
    defer mu1.Unlock()
    // ... balance move B -> A
}
// goroutine1: mu1 hold, mu2 ka wait
// goroutine2: mu2 hold, mu1 ka wait  -> classic deadlock cycle
```

```shell
# Go: saare goroutine stacks dump karo, dekho kaun kaha atka hai
kill -QUIT <pid>                                  # process ko SIGQUIT
curl localhost:6060/debug/pprof/goroutine?debug=2  # ya pprof endpoint se

# Java: automatic deadlock detection thread dump mein
jstack <pid> | grep -A5 "Found one Java-level deadlock"

# generic Linux: process kis par block hai (kernel wait channel)
cat /proc/<pid>/stack        # root access chahiye
```

**Quick summary:** Deadlock = circular wait, 4 Coffman conditions sab saath honi chahiye — ek tod do to deadlock impossible. Livelock = active but no progress, starvation = kabhi turn hi nahi aata, priority inversion = low-priority thread high-priority ko block kar deta hai bina lock ordering fix ke.

**Practice task(s):**
- Upar wala Go deadlock snippet khud run karo, `SIGQUIT`/goroutine dump se confirm karo ki dono goroutines kis lock ka wait kar rahe hain.
- Explain karo out loud: consistent lock ordering se circular wait kaise specifically todta hai — kaunsi Coffman condition break hoti hai?
- Ek livelock scenario khud code karo (do goroutines jo ek doosre ko "polite" tareeke se resource chhod dete hain infinite loop mein) aur observe karo CPU usage deadlock se kaise different dikhta hai.

---

### 5. Atomics, compare-and-swap, memory ordering, `volatile`, memory barriers

**Kya hai (What):** **Atomic operation** ek aisi operation hai jo hardware level par **indivisible** guarantee hoti hai — beech mein koi doosra thread use interrupt/interleave nahi kar sakta, chahe wo multi-step operation ho (jaise increment). **Compare-and-swap (CAS)** ek fundamental atomic primitive hai: "agar current value `expected` ke barabar hai, to use `new` se replace karo — atomically; warna kuch mat karo aur bata do ki fail hua." Ye lock-free programming ki poori foundation hai. **Memory ordering** ka matlab hai ki alag-alag CPU cores ko ek thread ke memory writes kis **order** mein visible hote hain — compiler aur CPU dono performance ke liye instructions ko **reorder** kar sakte hain (jab tak single-threaded correctness na tootey), lekin multi-threaded code mein ye reordering surprising bugs create kar sakti hai. **Memory barrier/fence** ek instruction hai jo compiler/CPU ko is reordering se explicitly **rokta** hai us point ke around. `volatile` (jiska meaning language se language badalta hai) compiler ko batata hai "is variable ko register mein cache mat karo, har baar memory se fresh read/write karo" — C/C++ mein ye sirf compiler-reordering rokta hai, **atomicity ya thread-synchronization guarantee nahi deta**; Java mein `volatile` iske alawa ek extra **happens-before** guarantee bhi deta hai jo isse thoda zyada powerful banata hai (lekin phir bhi compound operations jaise `x++` ko atomic nahi banata).

**Kyun important hai (Why):** Ye concepts lock-free data structures banane, aur bahut subtle bugs samajhne ke liye zaroori hain — jaise "double-checked locking" pattern jo bina proper memory barriers ke **broken** hota hai (ek thread object ko partially-constructed state mein "visible" dekh sakta hai doosre thread ko, reordering ki wajah se). High-throughput systems mein locks ka overhead avoid karne ke liye atomics use hote hain — ek single `LOCK XADD` (x86) instruction ek mutex lock/unlock cycle se **kahi zyada cheap** hai.

**Kaise kaam karta hai (How):** Hardware level par CAS x86 par `CMPXCHG` instruction hai, ARM par `LDXR`/`STXR` (load-link/store-conditional) pair hai. Lock-free algorithms typically ek **retry loop** pattern follow karte hain: current value read karo, naya value compute karo, CAS try karo — agar CAS fail hua (matlab beech mein koi doosra thread badal chuka), phir se try karo. Memory barriers 4 tarah ke hote hain conceptually — **load fence** (pehle ke loads baad ke loads se pehle complete ho), **store fence** (pehle ke stores baad ke stores se pehle visible ho), aur **full fence** (dono). **Acquire** semantics ek load ke saath aati hai (uske baad ke operations upar reorder nahi ho sakte), **release** semantics ek store ke saath (usse pehle ke operations neeche reorder nahi ho sakte) — ye pattern lock acquire/release ke exactly parallel hai.

**Real-world example aur backend/system-design connection:** Go ka `sync/atomic` package, Java ka `java.util.concurrent.atomic.AtomicInteger`, C++11 ka `std::atomic<T>` — sab high-throughput counters (metrics, request counts) ke liye mutex ke bajaye atomics use karte hain kyunki lock ka overhead avoid ho jata hai. Prometheus client libraries internally atomic counters use karte hain taaki metrics increment karna hot path ko slow na kare. Linux kernel apne memory barriers (`smp_mb()`, `smp_rmb()`, `smp_wmb()`) explicitly use karta hai lock-free data structures (jaise RCU — Read-Copy-Update) mein, jaha correctness memory ordering guarantees par heavily depend karta hai.

```c
// atomic increment via CAS retry loop (lock-free counter)
int fetch_and_add(atomic_int *counter) {
    int old_val, new_val;
    do {
        old_val = atomic_load(counter);
        new_val = old_val + 1;
    } while (!atomic_compare_exchange_weak(counter, &old_val, new_val));
    // beech mein agar koi aur thread value badal de, CAS fail hoga -> retry
    return new_val;
}
```

```go
var hits int64

func recordHit() {
    atomic.AddInt64(&hits, 1) // no mutex, no syscall — ek hi LOCK XADD instruction
}

func readHits() int64 {
    return atomic.LoadInt64(&hits) // safe read, guaranteed visibility
}
```

**Quick summary:** Atomics = hardware-guaranteed indivisible operations; CAS = "check-then-swap" primitive jispar poora lock-free world khada hai; memory ordering/barriers control karte hain ki writes kab visible hote hain doosre cores ko; `volatile` sirf caching rokta hai, atomicity nahi deta.

**Practice task(s):**
- Apni language ke atomic package (Go `sync/atomic`, Java `AtomicInteger`, C++ `std::atomic`) se ek counter implement karo aur mutex-based version se benchmark compare karo.
- Explain karo out loud: `volatile int x; x++;` C/Java mein thread-safe kyun nahi hai, chahe `volatile` lagaya ho.

---

### Subtopics — practical aha points

**Lock contention aur convoying; lock granularity; sharded locks/striping:** Jab bahut saare threads ek hi lock ke liye compete karte hain, throughput ek point ke baad **girna** shuru ho jata hai — isse **lock contention** kehte hain. **Convoying** tab hota hai jab lock-holder thread ko beech mein (I/O wait, page fault, OS scheduler) descheduled kar diya jata hai lock hold karte hue — sabhi waiting threads ek "traffic jam" ki tarah queue ho jate hain, chahe unka actual kaam chhota ho. **Lock granularity** ka matlab hai — ek bada, coarse-grained lock (poori data structure ke liye ek lock) simple hai lekin contention zyada, jabki fine-grained locks (row-level, bucket-level) complex hain lekin parallel throughput zyada. **Sharded locks / lock striping** iska practical solution hai: Java ka `ConcurrentHashMap` internally keyspace ko multiple segments (buckets) mein baant deta hai, har segment ka apna independent lock hota hai — `map.get("key")` sirf us ek segment ka lock leta hai, baaki 15+ segments free rehte hain concurrent access ke liye.

**Optimistic vs pessimistic concurrency control:** **Pessimistic concurrency** assume karta hai ki conflict likely hai, isliye data touch karne se pehle hi lock le leta hai (jaise SQL `SELECT ... FOR UPDATE`) — safe but throughput kam kar sakta hai high-contention workload mein. **Optimistic concurrency (OCC)** assume karta hai ki conflict rare hai, isliye bina lock liye directly read-modify karta hai, aur commit ke waqt check karta hai ki data change to nahi hua (version number ya timestamp compare karke) — agar change hua, transaction **retry** hoti hai. Ye choice Level 3 mein bade concepts ban jati hai — **MVCC (Multi-Version Concurrency Control)** optimistic ke close hai (Postgres readers writers ko block nahi karte), aur **2PL (Two-Phase Locking)** pessimistic ka formal version hai.

**Producer–consumer, thread pools, work stealing, bounded queues as backpressure:** **Producer-consumer** pattern mein producers data generate karte hain, consumers process karte hain, ek queue beech mein buffer ka kaam karti hai. **Bounded queue** (fixed capacity, jaise `make(chan Job, 100)` Go mein) tab important ban jati hai jab producer consumer se **fast** ho — queue full hote hi producer **block/reject** hota hai, jo ek natural **backpressure** signal hai (unbounded queue OOM tak grow karti rahegi). **Thread pool** threads ko reuse karta hai per-task spawn karne ke bajaye (thread creation ka overhead milliseconds order ka ho sakta hai). **Work stealing** ek scheduling optimization hai — idle worker apni queue khaali hone par busy worker ki queue se task "steal" kar leta hai instead of sitting idle; Go runtime scheduler, Java ka `ForkJoinPool`, aur Rust ka `rayon` isi pattern par based hain.

**Immutability aur copy-on-write as concurrency strategies:** Agar data **immutable** hai (kabhi modify hi nahi hota), to usse read karne ke liye koi lock ki zaroorat nahi — arbitrary number of threads safely share kar sakte hain. **Copy-on-write (COW)** ek related strategy hai — mutate karne ke liye pura naya copy banao, phir ek single atomic pointer-swap se purane ko replace karo; readers ko hamesha ek consistent snapshot milta hai, aur writers readers ko block nahi karte. Java ka `CopyOnWriteArrayList`, Linux ka `fork()` (child process parent ke memory pages COW se share karta hai jab tak koi modify na kare), aur Clojure ke persistent data structures — sab isi idea par based hain.

**Thundering herd, cache stampede:** **Thundering herd** ek generic OS-level phenomenon hai — jab ek event par bahut saare sleeping threads/processes ek saath wake hote hain (jaise multiple processes ek hi socket par `accept()` ke liye wait kar rahe the), sab race karte hain resource ke liye, zyadatar wapas so jate hain — wasted CPU/scheduling overhead (isi wajah se Linux ne `SO_REUSEPORT` introduce kiya). **Cache stampede** iska application-level version hai — jab ek hot cache key expire hoti hai, usi moment par thousands of concurrent requests cache-miss karke **saath mein** database par hit kar dete hain, DB ko overload kar dete hain. Common mitigations: `singleflight`-jaisi request coalescing (sirf ek request actual DB tak jaaye, baaki uska result wait karein), aur **jittered TTLs** (sab keys same exact second par expire na hon, thoda random spread rakho).

---

### Advanced concepts

**Lock-free aur wait-free algorithms; ABA problem; hazard pointers; epoch-based reclamation:** **Lock-free** algorithm ye guarantee deta hai ki system-wide **kuch na kuch** thread hamesha progress karega, chahe kisi individual thread ko starve/delay hona pade (CAS retry loops mein). **Wait-free** isse strong guarantee hai — **har** thread bounded number of steps mein guaranteed progress karega, chahe baaki threads kuch bhi karein (achieve karna bahut harder hai). **ABA problem** ek classic lock-free bug hai: thread ek CAS karne se pehle value `A` padhta hai; beech mein koi doosra thread value ko `A` se `B` mein badal deta hai, phir wapas `A` mein — CAS pehle wale thread ko "kuch nahi badla" dikhega (value abhi bhi `A` hai), lekin actually structure meaningfully change ho chuka tha (jaise ek linked-list node free hoke reuse ho gaya). Fix: **tagged pointers** (version counter attach karo), ya **hazard pointers** / **epoch-based reclamation** — ye techniques solve karti hain ki lock-free structure mein memory kab **safely free** ki ja sakti hai jab tumhe pata nahi ki koi doosra thread abhi bhi use read kar raha hai ya nahi.

**Memory models: sequential consistency, happens-before, acquire/release, relaxed ordering:** **Sequential consistency** sabse strong (aur sabse slow) model hai — jaise saare threads ke operations ek hi global timeline mein interleaved ho, program order respect karte hue. **Happens-before** ek partial-order relation hai jo Java Memory Model (JMM) aur Go memory model dono formally define karte hain — agar operation A "happens-before" operation B hai, to A ke effects B ko guaranteed visible hain. **Acquire/release semantics** iska practical middle-ground hai: ek **acquire** load ke baad ke operations upar reorder nahi ho sakte, ek **release** store se pehle ke operations neeche reorder nahi ho sakte — bilkul lock lock()/unlock() jaisa mental model. **Relaxed ordering** sirf atomicity guarantee karta hai, koi ordering constraint nahi — fastest but sabse zyada easy to misuse. C++ mein ye explicitly `std::memory_order_relaxed`, `std::memory_order_acquire`, `std::memory_order_release` ke through control kiya ja sakta hai.

**Actor model, CSP/channels, software transactional memory:** **Actor model** mein har actor apna **private, isolated state** rakhta hai aur baaki actors se sirf **asynchronous messages** ke through communicate karta hai — koi shared memory nahi, isliye classic race conditions structurally impossible hain (Erlang/Elixir, Akka). **CSP (Communicating Sequential Processes)** ek related lekin alag model hai — synchronization **channels** ke through hoti hai; Go ki poori concurrency philosophy isi par based hai: *"Don't communicate by sharing memory; share memory by communicating."* **STM (Software Transactional Memory)** database transactions jaisa idea memory operations par apply karta hai — code ko ek transaction mein wrap karo, agar conflict detect ho to automatically retry ho jaye (Clojure ke `refs`, Haskell ka STM library).

**False sharing aur padding; per-core data structures:** Do threads jab **logically unrelated** variables likhte hain jo memory mein **same 64-byte cache line** par accidentally land ho jate hain, to CPU cache-coherence protocol (MESI) baar-baar us cache line ko invalidate karta hai doosre core ke liye — result: unrelated writes bhi ek doosre ko slow kar dete hain. Fix: struct fields ko explicitly **pad** karo taaki hot, frequently-written-by-different-threads fields alag cache lines par land karein. Related idea — **per-core data structures**: har CPU core/thread ka apna private counter/buffer ho (jaise Linux kernel ke `percpu` variables), aggregation sirf reads ke time ho — writes kabhi contend hi nahi karte.

**Little's law applied to thread pools — `concurrency = arrival_rate × latency`:** Queueing theory ka **Little's Law** (`L = λW`) thread pool sizing mein directly apply hota hai: steady-state mein required concurrent workers = requests ki arrival rate × average time jo har request leta hai. Ye capacity planning ke liye ek quick, powerful back-of-envelope formula hai.

```
Little's Law:  Concurrency (L) = Arrival Rate (lambda) x Average Latency (W)

Example: API 500 requests/sec handle kar rahi hai (lambda = 500/sec),
har request average 40ms (0.04 sec) leti hai (W = 0.04)

  L = 500 * 0.04 = 20

  Matlab: steady-state mein hamesha ~20 requests "in flight" honge.
  Thread pool size isse kam rakha (jaise 10) to requests queue mein
  wait karengi -> latency aur badhegi -> aur zyada concurrency chahiye
  hogi (feedback loop) -> pool ko kam-se-kam ~20 (plus buffer/headroom)
  rakhna chahiye is load ko bina queueing ke serve karne ke liye.
```

---

### Important terms

- **Concurrency:** Multiple tasks ko overlapping time mein manage/structure karna — simultaneous execution zaroori nahi.
- **Parallelism:** Multiple tasks ka literally ek hi instant par, alag cores par execution.
- **Amdahl's law:** Fixed serial fraction ki wajah se speedup ki hard ceiling `1/(1-P)`.
- **Gustafson's law:** Problem size scale hone par near-linear speedup, serial overhead relatively chhota reh jata hai.
- **Race condition:** Timing-dependent bug jab shared mutable data bina synchronization ke access hoti hai.
- **Critical section:** Code ka hissa jo shared resource access/modify karta hai.
- **Mutual exclusion:** Guarantee ki ek time par sirf ek thread critical section mein ho.
- **Mutex:** Blocking lock — unavailable hone par thread sleep ho jata hai.
- **Spinlock:** Busy-wait lock — thread CPU spin karta hai check karte hue, sleep nahi hota.
- **Read-write lock (RWMutex):** Multiple concurrent readers, exclusive single writer.
- **Reentrant lock:** Same thread multiple baar acquire kar sakta hai bina self-deadlock ke.
- **Futex:** Linux ka fast userspace mutex primitive — fast path lock-free, slow path kernel sleep/wake.
- **Deadlock:** Threads ka cycle jaha har koi doosre ke resource ka wait kar raha hai, koi progress nahi.
- **Coffman conditions:** Deadlock ke liye zaroori 4 conditions — mutual exclusion, hold-and-wait, no preemption, circular wait.
- **Livelock:** Threads active hain, state change kar rahe hain, lekin overall progress zero.
- **Starvation:** Ek thread ko resource scheduling unfairness ki wajah se kabhi nahi milta.
- **Priority inversion:** Low-priority thread ka held lock high-priority thread ko indefinitely block kar deta hai.
- **Priority inheritance:** Priority inversion ka fix — lock holder temporarily waiting high-priority thread ki priority le leta hai.
- **Atomic operation:** Hardware-guaranteed indivisible operation, koi interrupt beech mein nahi.
- **Compare-and-swap (CAS):** "Agar current value expected ke barabar hai to swap karo" — atomic primitive, lock-free programming ki base.
- **Memory ordering:** Kis order mein ek thread ke writes doosre cores ko visible hote hain.
- **Memory barrier/fence:** Instruction jo compiler/CPU reordering ko explicitly rokta hai.
- **`volatile`:** Compiler ko register-caching se rokta hai; C/C++ mein atomicity guarantee nahi deta, Java mein happens-before extra deta hai.
- **Lock contention:** Bahut saare threads ek hi lock ke liye compete karte hain, throughput girta hai.
- **Convoying:** Descheduled lock-holder ki wajah se sab waiting threads queue ho jate hain.
- **Lock granularity:** Coarse (poori structure ek lock) vs fine-grained (per-row/per-bucket lock) tradeoff.
- **Lock striping/sharded locks:** Keyspace ko multiple independent-lock segments mein baantna (`ConcurrentHashMap`).
- **Optimistic concurrency control (OCC):** Bina lock liye proceed karo, commit time par conflict check karo.
- **Pessimistic concurrency control:** Data touch karne se pehle hi lock le lo.
- **MVCC:** Multi-Version Concurrency Control — optimistic-flavoured, readers writers ko block nahi karte (Level 3).
- **2PL:** Two-Phase Locking — pessimistic concurrency ka formal protocol (Level 3).
- **Backpressure:** Bounded queue full hone par producer ko block/reject karna, downstream overload se bachana.
- **Work stealing:** Idle worker busy worker ki queue se task le leta hai.
- **Copy-on-write (COW):** Mutate karne ke liye naya copy banao, atomic pointer-swap se replace karo.
- **Thundering herd:** Ek event par bahut saare sleeping threads/processes ek saath wake hokar race karte hain.
- **Cache stampede:** Hot key expire hone par bahut saare concurrent requests saath mein DB ko hit karte hain.
- **Request coalescing / singleflight:** Duplicate concurrent requests ko ek actual call mein merge karna.
- **Lock-free:** Guarantee ki system-wide kuch thread hamesha progress karega.
- **Wait-free:** Guarantee ki har thread bounded steps mein progress karega.
- **ABA problem:** CAS value ko unchanged samajhta hai jabki wo A→B→A cycle se guzar chuka tha.
- **Hazard pointers:** Lock-free structures mein safe memory reclamation ka mechanism.
- **Epoch-based reclamation:** Memory ko "epochs" mein group karke safely free karne ki technique jab koi reader use access nahi kar raha.
- **Sequential consistency:** Sabse strong memory model — global single interleaved order.
- **Happens-before:** Partial-order relation jo guarantee karta hai ek operation ke effects doosre ko visible hain.
- **Acquire/release semantics:** Lock-jaisa reorder-prevention pattern loads (acquire) aur stores (release) ke around.
- **Relaxed ordering:** Sirf atomicity, koi ordering guarantee nahi — fastest, most dangerous.
- **Actor model:** Isolated private state, communication sirf async messages se (Erlang, Akka).
- **CSP (Communicating Sequential Processes):** Channels ke through synchronization (Go).
- **Software transactional memory (STM):** DB-transaction-jaisa retry-on-conflict model memory operations par.
- **False sharing:** Unrelated variables same cache line par hone ki wajah se unnecessary cache invalidation.
- **Per-core data structures:** Har core ka apna private counter/buffer, contention-free writes.
- **Little's law:** `L = λW` — concurrency = arrival rate × average latency, thread pool sizing ke liye.

---

### Common mistakes

- Go mein `sync.Mutex` ko Java ke `synchronized` jaisa **reentrant** samajh lena — same goroutine se do baar `Lock()` call karna khud ko hi deadlock kar deta hai.
- Spinlock ko long-running critical sections ke liye ya single-core/oversubscribed environment mein use karna — CPU sirf waste hota hai, mutex se bhi slower ho jata hai.
- `mu.Lock()` ke turant baad `defer mu.Unlock()` na lagana — early return ya panic hone par lock permanently held reh jata hai, poora system freeze ho sakta hai.
- Alag-alag code paths mein locks **inconsistent order** mein acquire karna — deadlock sirf specific load/timing par manifest hota hai, dev mein kabhi nahi dikhta, production mein 3am ko dikhta hai.
- `volatile` ko C/C++/Java mein poori thread-safety samajh lena — ye compound operations (`x++`, `if (x == null) x = new X()`) ko atomic nahi banata, sirf caching/reordering rokta hai.
- Producer-consumer queue ko **unbounded** rakhna "simplicity" ke naam par — sustained producer-faster-than-consumer imbalance mein memory grow karti rehti hai OOM tak, backpressure nahi milti.
- Cache stampede ko sirf "zyada read replicas add karo" se fix karne ki koshish karna — ye root cause (thundering herd on expiry) address nahi karta, same expired key phir bhi sabko saath hit karayega.
- Bina measure kiye assume karna ki "zyada threads/cores = zyada throughput" — agar workload ek hot mutex par dominated hai, extra threads sirf contention aur context-switching overhead badhate hain, throughput **girta** hai.

---

### Interview questions

1. Concurrency aur parallelism mein exact difference kya hai — ek concrete example do jaha concurrency ho lekin parallelism na ho.
   *Hint:* Single-core CPU par OS time-slicing se multiple threads chalana — concurrent hai, parallel nahi (ek time par ek hi actually execute ho raha hai).

2. Amdahl's law ka formula likho aur explain karo ki cores add karne se diminishing returns kyun milte hain.
   *Hint:* `S(N) = 1/((1-P) + P/N)` — jaise `N → ∞`, formula `1/(1-P)` par saturate ho jata hai, serial fraction hi ceiling decide karta hai.

3. `counter++` thread-safe kyun nahi hai, chahe wo ek line ka code lage? Fix kaise karoge?
   *Hint:* Ye load-add-store — 3 separate steps hain, beech mein interleave ho sakta hai. Fix: mutex, ya atomic increment operation use karo.

4. 4 Coffman conditions kya hain? Deadlock avoid karne ke liye kaunsi condition tod na sabse practical approach hai, aur kyun?
   *Hint:* Mutual exclusion, hold-and-wait, no preemption, circular wait. Circular wait todna sabse practical hai — consistent global lock ordering se.

5. Spinlock aur mutex mein kab kaunsa choose karoge? Trade-offs batao.
   *Hint:* Spinlock — multi-core, bahut short critical section, context-switch overhead avoid karna hai. Mutex — long critical section ya single-core, CPU ko waste nahi karna.

6. Optimistic aur pessimistic concurrency control mein difference batao, real SQL example ke saath.
   *Hint:* Pessimistic = `SELECT ... FOR UPDATE` (lock pehle). Optimistic = version column check karke conditional `UPDATE`, conflict par retry.

7. ABA problem kya hai? Ek concrete scenario do jaha ye lock-free stack/queue mein bug create karta hai.
   *Hint:* Node A free hoke reuse ho gaya, phir wapas same address par allocate hua — CAS sochta hai kuch nahi badla, lekin structure ki linkage change ho chuki thi.

8. False sharing kya hai aur code mein ise kaise detect/fix karoge?
   *Hint:* Unrelated variables same 64-byte cache line par — padding add karke fields ko alag cache lines mein separate karo; profiling tools (perf c2c) se detect hota hai.

9. Priority inversion explain karo — Mars Pathfinder example ke saath, aur fix kya tha.
   *Hint:* Low-priority thread lock hold kar raha tha, medium-priority thread usse preempt kar raha tha, high-priority thread starve ho raha tha. Fix: priority inheritance.

10. Little's law ka formula batao aur ek thread pool sizing example do.
    *Hint:* `L = λW`. Agar 500 req/sec aur average latency 40ms hai, to steady-state concurrency ~20 — pool ko usse kam rakhoge to queueing badhegi.

---

### Hands-on lab

> Build a bounded thread-safe queue; then deliberately create a deadlock and a livelock and diagnose both from a thread dump.

**Ye specific lab kyun:** Ye lab teen alag skills ko ek saath force karta hai — pehle, ek **correct** concurrent data structure banana (bounded queue jisme backpressure, mutual exclusion, aur condition-variable-style signaling sab sahi ho) jo producer-consumer, locks, aur mutual exclusion — teeno topics ko ek jagah pull karta hai. Phir, **deliberately** deadlock aur livelock create karna tumhe in bugs ki "feel" deta hai — kya symptom dikhta hai (CPU usage deadlock mein near-zero, livelock mein high but no progress), aur most importantly, ek **real thread/goroutine dump** padhna sikhata hai — production mein jab koi service "hung" hoti hai, yahi skill use debug karne ka pehla step hoti hai. Ye theoretical Coffman-conditions knowledge ko ek muscle-memory diagnostic skill mein convert kar deta hai.

- [ ] Mera result: Deadlock reproduce hua? ______ | Thread/goroutine dump mein kaunsa lock/goroutine stuck dikha? ______ | Livelock reproduce hua? ______ | Deadlock vs livelock mein CPU usage/behavior mein kya difference observe kiya? ______

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.6 Reliability arithmetic & back-of-envelope estimation

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har topic neeche isi order mein follow karta hai: **kya hai (what) → kyun important hai (why) → kaise kaam karta hai (how) → real-world + backend/system-design example.**

### 1. Powers of 2 and 10; bytes/KB/MB/GB/TB/PB; seconds in a day (86,400) / year (~31.5 M)

**Kya hai:** Computing duniya mein do "number systems" parallel chalte hain jo aapko confuse kar sakte hain agar clarity nahi ho. Ek taraf **binary powers of 2** hain — kyunki hardware (address bus, memory chips, registers) fundamentally binary hai: `2^10 = 1024`, `2^20 ≈ 1.05 million`, `2^30 ≈ 1.07 billion`. Doosri taraf **decimal powers of 10** hain — jo SI units, networking bandwidth, aur marketing labels use karte hain: `1 KB = 1000 bytes` (decimal), jabki OS aksar `1 KiB = 1024 bytes` (binary) ko bhi "KB" hi likh deta hai. Ye ek historical naming confusion hai jiski wajah se "mera 1 TB hard disk sirf 931 GB dikha raha hai Windows mein" wali classic complaint hoti hai — disk manufacturer decimal TB (`10^12` bytes) bech raha tha, OS binary TiB (`2^40` bytes) mein dikha raha hai. Time constants bhi yaad rakhne zaroori hain: ek din mein `86,400` seconds hote hain (`24 × 60 × 60`), aur ek saal mein roughly `31.5 million` seconds (`86,400 × 365 ≈ 3.15 × 10^7`) — is number ka ek famous mnemonic hai: **π × 10^7** (coincidentally almost exact match, easy yaad rakhne ka trick).

**Kyun important hai:** System design interviews mein calculator allowed nahi hota — aapko mental math se fast, "roughly right" estimates deni hoti hain, exact nahi. Agar aap `2^10 ≈ 10^3` wali approximation jaante ho, to koi bhi bada binary exponent turant decimal order-of-magnitude mein convert ho jata hai bina long multiplication kiye. Aur agar KB vs KiB ka fark clear nahi hai, to bade estimates mein compounding error aata hai — chhota sa 2.4% gap (`1024` vs `1000`) bhi jab aap kai baar multiply karte ho (bytes → KB → MB → GB → TB) to final answer 5-10% tak off ho sakta hai, jo capacity planning mein real budget miss ban jata hai.

**Kaise kaam karta hai:** Core trick ye hai — `2^10 = 1024`, aur `1024` `1000` ke bahut kareeb hai, isliye **`2^n ≈ 10^(0.3n)`** (kyunki `log10(2) ≈ 0.301`). Isse aap kisi bhi binary number ka decimal magnitude turant nikal sakte ho: `2^32` → `0.3 × 32 ≈ 9.6` → roughly `10^9.6 ≈ 4 × 10^9` (4 billion) — yahi wajah hai IPv4 ka address space `~4.29 billion` hai (`2^32`). Time ke liye: `seconds/day = 86,400`, aur quick Fermi-estimation mein log isse `~10^5` round karte hain taaki daily-volume ko QPS mein divide karna easy ho jaye.

**Real-world example aur backend/system-design connection:** IPv4 exhaustion (`2^32 ≈ 4.3 billion` addresses) hi wajah hai jisne IPv6 adoption force kiya — ye seedha ek powers-of-2 estimation fact se nikla hua real infra crisis hai. 32-bit Unix timestamp overflow (Year 2038 problem) bhi isi se related hai. Backend mein jab aap "Redis mein max kitni keys fit hongi 64 GB RAM mein agar average key+value ~200 bytes hai" jaisa sawaal solve karte ho, to yahi powers-of-2/10 conversions aapko turant `~320 million keys` jaisa number dete hain bina calculator ke.

```
// Quick mental-math cheat sheet
2^10  = 1,024          ≈ 10^3   (Kilo)
2^20  = 1,048,576      ≈ 10^6   (Mega)
2^30  = 1,073,741,824  ≈ 10^9   (Giga)
2^32  = 4,294,967,296  ≈ 4.3 × 10^9  → IPv4 address space
2^40  ≈ 1.1 × 10^12    (Tera)
2^50  ≈ 1.1 × 10^15    (Peta)

seconds/day  = 24 × 60 × 60      = 86,400   (≈ 10^5 for quick estimates)
seconds/year = 86,400 × 365      ≈ 31,536,000  ≈ π × 10^7
```

**Quick summary:** Binary powers of 2 (`1024`-based, hardware-native) aur decimal powers of 10 (`1000`-based, SI/marketing) dono jaanna zaroori hai; `2^10 ≈ 10^3` trick se fast mental conversions hoti hain, aur `86,400` sec/day, `~31.5M` sec/year yaad rakhne se estimation interviews smooth chalte hain.

**Practice task:**
- Bina calculator ke mentally estimate karo: `2^36` bytes roughly kitne GB/TB hain?
- `date` ya kisi bhi language mein current Unix timestamp print karke check karo — kitne bits mein ye fit hoga, aur 2038 problem kyun aata hai, apne alfaazon mein explain karo.

---

### 2. QPS ↔ daily volume conversions; peak-to-average ratio (typically 2–10×)

**Kya hai:** **QPS** (Queries Per Second, ya generically Requests Per Second) aur daily total volume ke beech conversion ek basic-lekin-critical skill hai: `average QPS = total daily requests / 86,400`. Lekin real traffic **kabhi uniform nahi hota** — din bhar mein ek diurnal (daily) pattern hota hai jahan peak hours (evening, lunch break, ya region-specific busy times) mein traffic average se kaafi zyada hota hai. Ye multiplier **peak-to-average ratio** kehlata hai, typically **2× se 10×** ke beech — global, geographically-spread user base ke liye lower (traffic smooth ho jata hai kyunki alag-alag timezones alag time par peak karte hain), single-region/single-timezone service ke liye higher, aur viral/flash-sale/breaking-news events mein ye 10-100× tak spike kar sakta hai.

**Kyun important hai:** Agar aap sirf average QPS ke liye capacity plan karte ho, to system **peak hours mein gir jayega** — jo bilkul waisa hi hai jaise ek restaurant sirf "average customers per hour" dekh kar Friday dinner rush ke liye kitchen staff na badhaye. Ye interview mein sabse common "gotcha" hai — candidate average nikalta hai, seedha usi number par servers plan kar leta hai, aur interviewer poochta hai "aur peak traffic ka kya?"

**Kaise kaam karta hai:** Step 1: daily total requests nikalo (users × actions/user/day). Step 2: divide by `86,400` (ya quick estimate ke liye `~10^5`) → average QPS. Step 3: is average ko peak-to-average ratio se multiply karo (agar specific info nahi hai to safe default 3× lo, single-region ya spike-prone system ke liye 5-10× socho) → peak QPS, jiske liye actually provision karna hai. Auto-scaling groups, load balancers, database connection pools — sab peak ke liye sized hote hain, average ke liye nahi.

**Real-world example aur backend/system-design connection:** Twitter/X ka traffic major sports events (World Cup finals, Super Bowl) ke dauraan normal se 5-10× spike karta hai — unki infra teams inhi ratios ko pehle se model karke capacity reserve karti hain. AWS Auto Scaling Groups mein `min`, `desired`, aur `max` instance counts set karna seedha isi peak-to-average thinking se aata hai — `desired` average ke paas hota hai, `max` peak ke liye headroom deta hai.

```
# Worked example (roadmap ka classic Fermi estimate)
DAU (daily active users)     = 100,000,000        (100 M)
Actions per user per day     = 10

Daily requests = 100M × 10                = 1,000,000,000   (1 B req/day)

Average QPS = 1 B / 86,400 sec
            ≈ 11,574 QPS                  (~11.6k QPS average)

Peak QPS (assume 3x-5x peak-to-average ratio)
            = 11.6k × 3   to   11.6k × 5
            ≈ 35k QPS  to  58k QPS         (roadmap: ~35-60k QPS peak)
```

**Quick summary:** `avg QPS = daily volume / 86,400`, aur real provisioning hamesha **peak QPS** (avg × 2-10×, event-driven traffic ke liye zyada) ke liye hoti hai, average ke liye nahi.

**Practice task:**
- Apne kisi favorite app (jaise Zomato, Swiggy) ka approximate DAU aur daily orders/user Google karke average aur peak QPS estimate karo.
- Explain out loud: peak-to-average ratio global (multi-timezone) service mein single-region service se kam kyun hota hai?

---

### 3. Storage estimation: rows × row size × replication × retention × index overhead × growth

**Kya hai:** Storage estimation ek multiplicative formula hai jisme har factor apni jagah matter karta hai: **rows** (kitne records daily/total generate ho rahe hain) × **row size** (average bytes per record) × **replication factor** (data kitni baar duplicate stored hai fault-tolerance ke liye, typically 3) × **retention** (data kitne din/saal rakha jayega before deletion/archival) × **index overhead** (secondary indexes, B-tree/inverted-index structures raw data ke upar extra space lete hain, typically 20-100% extra) × **growth factor** (aapko current state ke liye nahi, 1-2 saal aage ke liye plan karna hai, compounding growth ke saath).

**Kyun important hai:** Cloud storage bill (EBS, S3, RDS storage) aksar sabse bada line item hota hai, aur agar aapne replication ya index overhead bhool diya to actual bill estimate se 3-5x zyada aa sakta hai — jo budget approval aur capacity planning dono ko galat kar deta hai. Interview mein bhi, "storage estimate karo" sawaal specifically test karta hai ki aap sirf raw data size nahi, **poora system** (replicas, indexes, growth) soch rahe ho ya nahi.

**Kaise kaam karta hai:** Step-by-step multiply karo, ek factor chhodo mat:

1. Daily new rows × row size = raw data/day.
2. × replication factor (RF) = actual disk footprint/day (Cassandra/HDFS default RF=3 hota hai fault tolerance ke liye).
3. × index overhead multiplier (agar heavy secondary indexing hai — jaise Elasticsearch replicas + inverted index — to yeh 2-3x tak ho sakta hai).
4. × retention window (kitne din/saal ka data ek saath rakha jayega) = total storage at steady state.
5. Growth ke liye har saal ka projected multiplier laga kar future headroom plan karo.

**Real-world example aur backend/system-design connection:** Kafka mein `log.retention.hours`/`log.retention.bytes` setting seedha disk usage ko multiply karta hai — retention 7 din se 30 din badhane ka matlab hai ~4x zyada disk. Cassandra/DynamoDB mein RF=3 default hai — matlab raw data 3x ho jata hai disk par. Elasticsearch mein replica shards + inverted index overhead ki wajah se raw JSON data ka indexed footprint aksar 1.5-3x tak ho jata hai. Backend decision: jab aap "kya main Postgres pe 5 secondary indexes daalu" soch rahe ho, to remember karo — har naya index storage cost + write latency dono badhata hai.

```
# Worked example (roadmap ka classic)
Record size          = 2 KB
Daily raw volume      = 1 B records/day × 2 KB  ≈ 2 TB/day (raw)

Apply replication (RF = 3):
  2 TB/day × 3        = 6 TB/day  (actual disk write/day across cluster)

Annualize (retention ~1 year, ignoring compaction/deletion):
  6 TB/day × 365 days ≈ 2,190 TB  ≈ ~2.2 PB/year
```

**Quick summary:** Storage estimate = rows × row size × replication × retention × index overhead × growth — koi bhi ek factor miss karna estimate ko drastically galat bana deta hai; hamesha replication aur index overhead explicitly likho.

**Practice task:**
- Estimate karo: agar aap ek chat app bana rahe ho jisme 50M DAU har din average 20 messages bhejte hain, har message ~500 bytes, RF=3, 1 saal retention — total storage kitna hoga?
- Kisi bhi Postgres table par `\d+ tablename` ya `pg_relation_size()` run karke check karo actual table size vs uske indexes ka combined size — ratio dekho.

---

### 4. Bandwidth estimation: QPS × payload size; ingress vs egress (egress is what you pay for)

**Kya hai:** Bandwidth estimate ka basic formula hai — **required bandwidth = QPS × average payload size** (bytes ko bits mein convert karne ke liye ×8 karo, phir per-second rate cloud provider ke Gbps limits se compare karo). Traffic direction ke do naam hain: **ingress** (data jo aapke servers ke **andar** aa raha hai — uploads, incoming API requests) aur **egress** (data jo aapke servers se **bahar** ja raha hai — API responses, downloads, images/video delivery). Cloud providers (AWS, GCP, Azure) mein ek asymmetric pricing pattern hai: **ingress almost hamesha free hota hai, egress paid hota hai** — aur egress hi actual bill mein surprise deta hai.

**Kyun important hai:** Bahut saari startups ka "sudden expensive AWS bill" story is exact asymmetry se aata hai — unhone user uploads (ingress, free) ka andaza laga liya, lekin har user ko baar-baar images/videos serve karna (egress) jo actual bill drive karta hai wo miss kar diya. System design interviews mein jab bhi content-heavy system (video, images, large file downloads) discuss hota hai, egress cost ek explicit design consideration hona chahiye — yehi CDN use karne ka primary financial justification hai.

**Kaise kaam karta hai:** Peak QPS × average response payload size (bytes) × 8 (bits/byte) = required egress bandwidth in bits/sec, jise phir Mbps/Gbps mein convert karke NIC (network interface card) capacity aur cloud egress pricing tier se compare karte hain. CDN yahan critical role play karta hai — agar CDN cache hit ratio 90% hai, to **origin servers sirf 10% traffic actually serve karte hain**, baaki 90% CDN edge nodes se serve hota hai jo origin egress cost drastically kam kar deta hai.

**Real-world example aur backend/system-design connection:** Netflix aur YouTube dono heavily CDN par depend karte hain (Netflix ka apna **Open Connect** CDN hai) exactly isliye — unke scale par agar har video-byte origin se directly serve hota, to bandwidth cost aur latency dono untenable ho jate. Backend/system-design mein classic decision hai: static assets/images ke liye **S3 + CloudFront** use karna direct EC2-se-serve karne ke bajaye — ye origin egress kam karta hai AND latency bhi improve karta hai (edge locations user ke paas hote hain).

```
# Bandwidth worked example
Peak QPS              = 50,000
Avg response payload  = 20 KB

Required egress bandwidth:
  50,000 × 20 KB = 1,000,000 KB/sec = ~1 GB/sec
  1 GB/sec × 8    ≈ 8 Gbps

# With CDN offloading 90% of traffic (only 10% hits origin):
  Origin egress needed ≈ 0.8 Gbps   (10x less origin load + cost)
```

**Quick summary:** Bandwidth = QPS × payload size; ingress free hai, **egress paid hai** — isliye CDN caching seedha ek cost-reduction lever hai, latency-reduction bonus ke saath.

**Practice task:**
- Apne current project (ya kisi public API) ke ek endpoint ka average response size dekho (browser DevTools Network tab), aur 10k QPS par egress bandwidth estimate karo.
- Explain out loud: kyun AWS S3 → CloudFront setup mein egress cost origin (S3) se seedha serve karne se kam padta hai.

---

### Subtopics — practical aha points

**Availability math: nines table, series vs parallel composition** — Availability ek percentage hai (`uptime / total time`), aur jab aap `99%` se `99.999%` ki taraf badhte ho, har extra "nine" downtime ko roughly **10x kam** kar deta hai — isliye ye non-linear cost curve hai (har additional nine achieve karna pichle se kaafi zyada expensive hota hai):

| Availability | Downtime/year | Downtime/month | Downtime/week |
|---|---|---|---|
| 99% | 3.65 d | 7.3 h | 1.7 h |
| 99.9% | 8.77 h | 43.8 m | 10.1 m |
| 99.95% | 4.38 h | 21.9 m | 5 m |
| 99.99% | 52.6 m | 4.4 m | 1 m |
| 99.999% | 5.26 m | 26 s | 6 s |

**Composition rules** critical hain: **series** (dependent) components ki availabilities **multiply** hoti hain — agar A aur B dono zaroori hain kaam karne ke liye, to `overall = A × B`, jo hamesha individual se kam hota hai. **Parallel** (redundant) components mein overall failure probability multiply hota hai — `overall_failure = (1-A) × (1-B)`, jo availability ko dramatically improve karta hai, **lekin sirf tab jab failures truly independent hon**. `SELECT` query jaisa hi soch lo — series dependency mein "sabse weak link" pura chain ka availability decide karta hai.

**Cost estimation: compute, storage, egress, managed-service premiums** — Back-of-envelope cost modeling mein 4 major buckets hain: **compute** (EC2/VM hours, CPU-hours), **storage** (disk GB-months — jaisa upar discuss kiya), **egress** (bandwidth out, jo aksar hidden-lekin-bada cost hai), aur **managed-service premiums** — jaise RDS/DynamoDB/MongoDB Atlas self-hosted equivalent se `20-60%` zyada charge karte hain kyunki wo operational burden (patching, backups, failover) aapse le lete hain. Ye trade-off explicitly evaluate karna chahiye: `managed service premium < engineer-hours spent on self-managed ops` ho to managed worth hai. Example: `db.r6g.xlarge` RDS Postgres vs equivalent self-managed EC2+Postgres — RDS costlier per-hour hai, lekin patching/backup/failover automatically handle hoti hai.

**Read:write ratio as the single most important number in a design** — System design shuru karte hi sabse pehla sawaal poochna chahiye: **"is system mein reads aur writes ka ratio kya hai?"** Kyunki ye ek single number puri architecture ko drive karta hai — read-heavy system (jaise Twitter feed, `~1000:1` read:write) caching (Redis), read replicas, aur CDN se massive benefit leta hai kyunki writes rare hain to unpar less focus chalega. Write-heavy system (jaise IoT sensor ingestion, logging pipeline) ko sharding, write-optimized storage engines (LSM-trees — jaise Cassandra, RocksDB), aur **kam indexes** (har extra index write latency badhata hai) ki zaroorat hoti hai. `SELECT`-heavy vs `INSERT`-heavy workload identify karna interview mein hamesha sabse pehla step hona chahiye, code likhne se pehle.

---

### Advanced concepts

**Series dependency: 10 services at 99.9% each ⇒ 99.0% overall. Dependencies multiply.** Agar ek request 10 microservices se **sequentially** guzarta hai (A calls B calls C...), aur har ek `99.9%` available hai, to overall availability `(0.999)^10 ≈ 0.99` — matlab sirf `99.0%`! Har extra service jo call chain mein add hota hai, **overall availability neeche girata hai**, chahe har individual service kitna bhi reliable kyun na ho. *Fayda:* microservices architecture mein "hum sab kuch chhote services mein tod denge" decision ka ek hidden reliability cost hai — jitne zyada hops, utna kam combined uptime, jab tak aap timeouts/retries/circuit-breakers se compensate na karo.

**Parallel redundancy: 2 replicas at 99% ⇒ 99.99% *if failures are independent* (they rarely are)** Agar do replicas independently fail hoti hain, to dono ka **simultaneously down hona** `(1-0.99) × (1-0.99) = 0.0001` probability hai — matlab `99.99%` combined availability. Lekin ye math sirf tab valid hai jab failures **genuinely independent** hon. *Fayda:* ye samjhaata hai ki "bas 2 replicas laga do, 5 nines mil jayenge" kitna naive assumption hai bina correlated-failure analysis ke.

**Correlated failure: shared power, shared control plane, shared config push, shared deploy — the real killer** Real-world mein replicas **rarely truly independent** hote hain — agar dono same datacenter, same power grid, same Kubernetes control plane, ya same CI/CD pipeline se deploy hoti hain, to ek bad config push ya ek power outage **dono ko ek saath** gira deta hai. *Fayda:* ye samjhaata hai ki multi-AZ/multi-region deployment sirf "nice to have" nahi hai — agar replicas same failure domain mein hain, to parallel-redundancy ka math (upar wala) completely invalid ho jata hai. AWS ke multi-AZ outages (jaise us-east-1 ke bade incidents) isi correlated-failure pattern ke classic examples hain — shared control plane down hone se "independent" replicas ek saath affected hue.

**MTBF, MTTR, MTTD, MTTF; `availability = MTBF / (MTBF + MTTR)` ⇒ reducing MTTR is usually cheaper than raising MTBF** Char reliability metrics: **MTBF** (Mean Time Between Failures — kitni der system chalta hai before failing), **MTTR** (Mean Time To Recovery/Repair — kitni der lagta hai failure se wapas aane mein), **MTTD** (Mean Time To Detect — kitni der lagta hai failure ko notice karne mein), **MTTF** (Mean Time To Failure — non-repairable systems ke liye, jaise hardware). Formula: `availability = MTBF / (MTBF + MTTR)`. *Fayda:* MTBF badhana (better hardware, more testing, fewer bugs) expensive aur slow hai; **MTTR ghatana** (better monitoring, automated rollback, runbooks, faster on-call response, chaos engineering practice) usually **much cheaper aur faster** hai — isliye modern SRE practice "resilience engineering" (fast recovery) par zyada focus karti hai bajaye "failure prevention" ke.

**Birthday-paradox collision math for ID generation; UUID/ULID collision probability** Birthday paradox kehta hai — agar aap random values ek `N`-size space se pick kar rahe ho, to collision probability **linearly nahi, quadratically** badhti hai number of items ke saath: roughly `~sqrt(N)` items ke baad collision likely ho jata hai (23 logon mein hi 50%+ chance same birthday hone ka, 365 options hone ke bawajood). *Fayda:* ye samjhaata hai ki UUID v4 (122 random bits) itna safe kyun hai practically — collision-relevant space `2^61`-ish hai (birthday bound), jo itna bada hai ki trillions of IDs generate karne par bhi collision probability negligible hai. Lekin **ULID** ya Snowflake-style IDs (jo time-based prefix + smaller random suffix use karte hain, sorting ke liye) mein random component chhota hota hai — high-throughput single-millisecond-window ID generation mein collision risk real ho sakta hai agar coordination na ho.

**Queueing theory preview: utilization ρ, and why latency → ∞ as ρ → 1 (`M/M/1: W = 1/(μ-λ)`)** Ek server/queue ka **utilization** `ρ = λ/μ` hota hai (`λ` = arrival rate, `μ` = service rate). Jab `ρ` `1` ke paas jaata hai (matlab system apni max capacity ke bahut kareeb chal raha hai), **average wait time exponentially badhta hai**, aur `ρ = 1` par theoretically infinite ho jata hai (`M/M/1` model: `W = 1/(μ - λ)` — jaise-jaise `λ` `μ` ke kareeb aata hai, denominator zero ke paas jaata hai). *Fayda:* isilye production systems `~70%` utilization target rakhte hain, `100%` nahi — headroom rakhna latency explosion se bachne ke liye. Isi se **Little's Law** (`L = λ × W`) juda hai — average concurrent requests (`L`) = arrival rate × average time spent in system — jo capacity planning mein cores/threads/connections estimate karne ke liye directly use hota hai.

```
# Worked example (Little's Law → cores needed)
Peak QPS (λ)              = 60,000 req/sec
Time per request per core = 5 ms = 0.005 sec

Concurrent in-flight requests (Little's Law: L = λ × W):
  L = 60,000 × 0.005 = 300 concurrent requests

⇒ Minimum ~300 cores needed simultaneously to keep up with peak load
⇒ At 70% target utilization (headroom for spikes):
     300 / 0.7 ≈ 430 cores needed
⇒ At 8 cores/node:
     430 / 8 ≈ ~54 nodes   (roadmap's ballpark: ~40 × 8-core nodes + headroom)
```

**Quick summary:** Series dependencies multiply availability down; parallel redundancy only helps if failures are truly independent (rarely true — correlated failures are the real killer); MTTR reduction beats MTBF improvement for cost-effectiveness; birthday-paradox math governs ID collision risk; and utilization `ρ → 1` is why systems are sized with headroom, not at 100% capacity.

**Practice task:**
- Calculate: agar aapke system mein 5 services hain series mein, har ek `99.95%` available, to overall availability kya hogi?
- `python3 -c` ya kisi bhi language mein ek chhota script likho jo `M/M/1` formula (`W = 1/(μ-λ)`) plot kare `ρ = 0.1` se `ρ = 0.99` tak, aur dekho latency kaise explode karta hai.
- UUID v4 aur ULID dono generate karke unki structure compare karo (`uuidgen`, ya Python `uuid.uuid4()` vs koi ULID library) — random bits kitne hain dono mein?

---

### Important terms

- **QPS (Queries Per Second):** Ek second mein kitne requests/queries system handle kar raha hai — capacity planning ki base unit.
- **DAU (Daily Active Users):** Ek din mein system use karne wale unique users ki count.
- **Peak-to-average ratio:** Peak traffic aur average traffic ke beech ka multiplier (typically 2-10x), jisse capacity provisioning hoti hai.
- **Fermi estimation:** Rough, order-of-magnitude estimation technique bina exact data ke, sirf reasonable assumptions se.
- **KB vs KiB:** `KB` (kilobyte) decimal `1000` bytes hai officially, `KiB` (kibibyte) binary `1024` bytes hai — OS/tools aksar dono ko "KB" likh dete hain, confusion create karte hain.
- **Replication factor (RF):** Data ki kitni copies system mein maintained hoti hain fault-tolerance ke liye (common default: 3).
- **Retention:** Data kitne time tak store rakha jata hai before deletion/archival.
- **Index overhead:** Secondary indexes/inverted-index structures ki wajah se raw data ke upar extra storage consumption.
- **Ingress:** Data jo servers ke andar aata hai (uploads, incoming requests) — typically free in cloud billing.
- **Egress:** Data jo servers se bahar jata hai (responses, downloads) — typically the actual billed bandwidth cost.
- **CDN (Content Delivery Network):** Geographically-distributed edge caches jo origin servers ka egress load aur user latency dono kam karte hain (e.g. CloudFront, Netflix Open Connect).
- **Availability:** Percentage of time ek system operational/usable hai (`uptime / total time`).
- **Nines:** Availability ko `99%`, `99.9%`, `99.99%` etc. ("two nines", "three nines"...) ke roop mein describe karne ka shorthand.
- **Series composition:** Dependent components jinka combined availability **multiply** hota hai (weakest link dominates).
- **Parallel composition:** Redundant components jinka combined **failure probability** multiply hota hai (agar independent ho to availability improve hoti hai).
- **Correlated failure:** Multiple components ka ek hi shared cause (power, config, deploy) se ek saath fail hona — redundancy math ko invalid karta hai.
- **MTBF (Mean Time Between Failures):** Average time ek repairable system successfully chalta hai do failures ke beech.
- **MTTR (Mean Time To Recovery/Repair):** Average time lagta hai ek failure se recover hone mein.
- **MTTD (Mean Time To Detect):** Average time lagta hai ek failure ko notice/detect karne mein.
- **MTTF (Mean Time To Failure):** Non-repairable components ke liye average lifetime before failure.
- **Birthday paradox:** Statistical phenomenon jisme random collision probability items ki count ke saath expected se bahut zyada fast (`~sqrt(N)`) badhti hai.
- **UUID (Universally Unique Identifier):** 128-bit randomly/structured generated unique ID, commonly v4 (fully random).
- **ULID (Universally Unique Lexicographically sortable Identifier):** Time-prefixed + random-suffix ID format jo sortable hota hai UUID ke unlike.
- **Utilization (ρ, rho):** Ratio of arrival rate to service rate (`λ/μ`) — kitna "busy" ek server/queue hai.
- **M/M/1 queue:** Simplest queueing model — Markovian arrivals, Markovian service times, 1 server — used to model latency vs utilization behavior.
- **Little's Law:** `L = λ × W` — average number of items in a system equals arrival rate times average time spent in system; core formula for capacity planning.
- **Managed service premium:** Extra cost cloud-managed databases/services (RDS, DynamoDB, Atlas) charge over self-hosted equivalents, in exchange for ops offloading.
- **Read:write ratio:** Proportion of read operations to write operations in a system — foundational number that shapes caching, replication, and storage-engine choices.

---

### Common mistakes

- **KB/KiB confusion compounding across layers:** Bytes → KB → MB → GB conversions mein `1000` vs `1024` mix karna — chhota sa `2.4%`-per-layer gap, PB-scale estimates mein 5-10% tak drift kar sakta hai, jo budget miss karwata hai.
- **Average QPS ke liye provision karna, peak ke liye nahi:** Capacity planning sirf daily-average pe based hoti hai, aur system peak hours/events mein gir jata hai — classic "hum toh average traffic ke liye ready the" outage.
- **Replicas ko independent maan lena bina failure-domain check kiye:** "2 replicas at 99% = 99.99%" formula use karna bina ye verify kiye ki wo same AZ, same power, ya same deploy pipeline share toh nahi karte — correlated failure hone par actual availability formula se kaafi kam hoti hai.
- **Storage estimate mein replication factor ya index overhead bhool jaana:** Sirf raw row-size × row-count multiply karna, aur RF=3 ya index overhead ka factor bhool jaana — actual disk usage estimate se 3-5x zyada nikalta hai.
- **Egress cost ko design-time consideration na banana:** Video/image-heavy system design karte waqt CDN evaluate na karna, phir production mein shocking cloud bill milna jab origin directly serve kar raha tha.
- **MTBF improve karne pe hi focus karna, MTTR ignore karna:** "Better/more reliable hardware kharido" par paisa lagana, jabki faster rollback/better monitoring/runbooks se MTTR kam karna usually cheaper aur zyada impactful hota hai.
- **100% utilization target rakhna:** Servers/queues ko unki theoretical max capacity ke kareeb chalana bina headroom ke — chhoti si traffic spike bhi latency ko exponentially explode kar deti hai (`ρ → 1`).
- **Read:write ratio poochhe bina architecture design shuru karna:** Seedha "hum Postgres use karenge, phir Redis cache lagayenge" decide kar lena bina pehle ye samjhe ki system read-heavy hai ya write-heavy — jisse galat caching/sharding strategy choose hoti hai.

---

### Interview questions

1. **"100M DAU hain, har user 10 actions/day karta hai — average aur peak QPS estimate karo."** Hint: daily volume / 86,400 = avg; avg × (3-10x) = peak, assumption explicitly state karo.
2. **"10 microservices ek call chain mein hain, har ek 99.9% available — overall availability kya hogi?"** Hint: series composition — multiply karo: `(0.999)^10 ≈ 99.0%`. Ek weak link poore chain ko neeche kheenchta hai.
3. **"2 replicas 99% available hain — kya combined availability seedha 99.99% ho jayegi?"** Hint: sirf tab jab failures truly independent hon; real-world mein correlated failures (shared power/deploy/control-plane) is math ko invalid kar dete hain.
4. **"MTBF aur MTTR mein fark batao, aur kyun MTTR kam karna usually zyada cost-effective hai?"** Hint: `availability = MTBF/(MTBF+MTTR)`; MTTR improve karna (automation, faster rollback) MTBF improve karne (better hardware) se aksar cheaper hota hai.
5. **"1M records/day, har record 5KB, RF=3, 90-din retention — total storage estimate karo."** Hint: rows × size × RF × retention, step by step multiply karo, index overhead bhi mention karo.
6. **"Ingress vs egress mein kya fark hai, aur egress cost design decisions ko kaise affect karta hai?"** Hint: egress typically billed hai, ingress free — CDN use karna origin egress cost kam karne ka primary financial reason hai.
7. **"Birthday paradox UUID collision probability ko kaise affect karta hai?"** Hint: collision probability `~sqrt(N)` items ke baad significant hoti hai, linear nahi — UUID v4 ke 122 random bits itna bade hain ki practically safe hai, lekin chhote random-suffix wale schemes (jaise coordination-less Snowflake variants) risk mein ho sakte hain.
8. **"Little's Law kya hai, aur ye thread pool/connection pool sizing mein kaise use hota hai?"** Hint: `L = λ × W` — arrival rate × average time-in-system = concurrent items; isse minimum cores/connections/threads needed estimate hote hain.
9. **"Kyun read:write ratio ko system design ka sabse pehla sawaal manna chahiye?"** Hint: ye caching strategy, replication approach, aur storage-engine choice (B-tree vs LSM-tree) directly decide karta hai.
10. **"Utilization ρ jaise-jaise 1 ke kareeb jaata hai, latency ka kya hota hai, aur iska practical implication kya hai capacity planning mein?"** Hint: `M/M/1: W = 1/(μ-λ)` — `ρ → 1` par latency exponentially/theoretically infinite ho jati hai; isliye systems `~70%` utilization target par chalti hain, headroom ke saath.

---

### Hands-on lab

> Estimate storage/bandwidth/servers for Instagram, WhatsApp, and Uber. Compare against published engineering blogs.

**Ye specific lab kyun:** Ye poore section ke abstract formulas (rows × size × replication, QPS × payload, peak-to-average ratio, Little's Law) ko real, well-documented systems par apply karne ka exercise hai — aur phir apne estimates ko actual published engineering blog numbers se compare karke dekhna ki aapka Fermi-estimation "muscle" kitna accurate hai. Ye gap (aapka estimate vs real number) hi batata hai ki aapki mental-math assumptions (peak ratio, avg payload size, retention) kitni realistic hain, aur agle interview mein aap kitne confidently in numbers ko defend kar paoge.

- [ ] Mera result: Instagram estimate (storage/bandwidth/servers) = ______, WhatsApp estimate = ______, Uber estimate = ______, Published-blog actual numbers (jahan mile) = ______

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.7 Thinking tools & the design method

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain). Har topic isi order mein cover hoga: **kya hai → kyun important hai → kaise kaam karta hai → real-world aur backend/system-design example.**

### 1. The 7-step design method: Requirements → Constraints → API → Data model → High-level design → Deep dives → Bottlenecks & evolution

**Kya hai:** Ye ek **structured process** hai jisse tum koi bhi system design problem — chahe interview ho ya real production ka naya service — systematically attack karte ho, random se nahi. Saat steps hain:
1. **Requirements** — functional aur non-functional dono gather karo (kya banana hai, kis quality ke saath).
2. **Constraints** — scale, budget, team size, existing tech, time — jo cheezein tumhare solution space ko limit karti hain.
3. **API** — system ke public contract ko define karo (endpoints, request/response shape) — isse "kya banana hai" concrete ban jata hai.
4. **Data model** — entities, relationships, aur unhe kaha/kaise store karoge (SQL schema, NoSQL document shape, etc.).
5. **High-level design** — boxes aur arrows: services, databases, caches, queues, load balancers — bina deep detail ke overall shape.
6. **Deep dives** — jo 1-2 components sabse critical/risky hain unme zoom in karo (e.g. "unique ID kaise generate karoge at scale").
7. **Bottlenecks & evolution** — abhi ka design kahan toot sakta hai (single point of failure, hot partition, etc.), aur 10x scale par kya change karna padega.

**Kyun important hai:** Bina structure ke log seedha "chalo Kafka aur microservices use karte hain" bol dete hain bina requirements samjhe — ye **resume-driven architecture** ka classic starting point hai. Interviews mein bhi jo candidate seedha whiteboard par boxes banane lagta hai (step 5 se start karke) usko requirements-gathering ke marks nahi milte, aur real design bhi galat direction mein chala jata hai kyunki assumptions kabhi explicit hue hi nahi. Ye method ek **forcing function** hai — tumhe har baar pehle "kya chahiye" aur "kaunsi constraints hain" pucchne par majboor karta hai, tabhi "kaise banayenge" tak pohochte ho.

**Kaise kaam karta hai:** Ye process linear lagta hai lekin practically iterative hota hai — deep dive karte waqt (step 6) kabhi realize hota hai ki ek requirement clarify nahi hui thi, to step 1 par wapas jaate ho. Interview mein rough time-boxing helpful hai: ~5 min requirements+constraints, ~5 min API, ~5 min data model, ~10 min high-level design, ~15 min deep dives, ~5 min bottlenecks. Production mein ye same steps ek **design doc** (RFC) ka structure ban jate hain — jisko team review karti hai commit karne se pehle.

**Real-world example aur backend/system-design connection:** Jab koi company "URL shortener" ya "rate limiter" design karti hai internally bhi (na ki sirf interview mein), wahi 7 steps follow hote hain: pehle PM/eng requirements likhte hain (functional: shorten + redirect; non-functional: 100ms redirect latency, 99.99% uptime), phir capacity constraints (kitne QPS, kitna storage), phir API contract (`POST /shorten`, `GET /{code}`), phir data model (mapping table: short_code → long_url), phir high-level architecture diagram, phir deep dive (ID generation strategy — counter vs hash vs Snowflake-style), aur last mein "agar traffic 100x ho jaye to kya toothega" (single DB → sharding). Yahi structure system-design interviews mein evaluators dhoondte hain kyunki yahi structure real engineering teams follow karti hain RFC likhte waqt.

```text
# 7-step design method — quick template (use as your scratch structure)

1. Requirements
   - Functional: ...
   - Non-functional: latency=?, availability=?, consistency=?, scale=?

2. Constraints
   - Read:Write ratio = ?
   - Total users / QPS peak / data volume / growth rate = ?
   - Budget, team size, existing infra = ?

3. API
   POST /shorten   { long_url } -> { short_code }
   GET  /{code}    -> 302 redirect to long_url

4. Data model
   urls(short_code PK, long_url, created_at, expires_at)

5. High-level design
   Client -> LB -> App servers -> Cache (hot codes) -> DB (sharded)

6. Deep dive
   - ID generation: counter+base62 vs random+collision-check vs Snowflake

7. Bottlenecks & evolution
   - Hot key problem (viral URL) -> cache + read replicas
   - DB growth -> shard by short_code hash
```

**Quick summary:** 7-step method — Requirements, Constraints, API, Data model, High-level design, Deep dives, Bottlenecks & evolution — ek repeatable checklist hai jo kisi bhi design problem ko random guessing se structured engineering mein badal deta hai.

**Practice task(s):**
- Kisi bhi random system (e.g. "parking lot booking app") ke liye ye 7 steps zor se bolkar explain karo, bina likhe, 2 minute mein.
- Ek chhota RFC-style doc likho (paper ya text file mein) kisi real feature ke liye jo tumne kabhi banaya ho, in 7 headings ke saath.

---

### 2. Functional vs non-functional requirements (NFRs)

**Kya hai:** **Functional requirements (FR)** batate hain system **kya karta hai** — features, behaviors, user-facing actions (e.g. "user URL shorten kar sake", "user login kar sake"). **Non-functional requirements (NFR)** batate hain system **kaisa hona chahiye** — qualities jaise latency, throughput, availability, consistency, durability, security, cost, scalability. NFRs ko aksar quantify kiya jata hai: **SLA** (Service Level Agreement — customer ke saath formal promise, penalty ke saath), **SLO** (Service Level Objective — internal target, e.g. "p99 latency < 200ms"), **SLI** (Service Level Indicator — actual measured metric jo SLO ke against track hoti hai).

**Kyun important hai:** Interviews mein sabse common galti hai sirf FRs discuss karna aur NFRs ko skip kar dena — jisse evaluator ko lagta hai candidate ne "production mindset" nahi dikhaya. Real world mein NFRs hi actual architecture decide karte hain, FRs nahi: "URL shorten karo" ek simple CRUD app se solve ho sakta hai — lekin "100M redirects/day, p99 < 50ms, 99.99% uptime" ye requirement hai jo caching, replication, aur sharding ko **mandatory** bana deti hai. Bina NFRs clarify kiye design karna aisa hai jaise bina destination bataye directions maangna.

**Kaise kaam karta hai:** Practically, tum requirements-gathering step mein explicitly dono categories ke liye puchte ho: FRs ke liye "system ko kya-kya karna chahiye, MVP kya hai, kya out-of-scope hai", NFRs ke liye specific numbers maangte ho — "latency budget kitna hai?", "kitna downtime acceptable hai per month (99.9% = ~43 min/month, 99.99% = ~4.3 min/month)?", "strong consistency chahiye ya eventual chalega?". Ye numbers seedha tumhare technology choices (cache layer chahiye ya nahi, single-region ya multi-region, SQL ya NoSQL) ko drive karte hain.

**Real-world example aur backend/system-design connection:** WhatsApp ke liye FR hai "message deliver ho" — lekin NFR hai "message order preserve ho within a chat, aur agar receiver offline hai to message queue ho jaye aur deliver ho jab wo online aaye, aur ye sab <1 second latency mein ho jab dono online hain". Ye NFR hi decide karta hai ki tum ek simple pub-sub use nahi kar sakte — tumhe per-user message queue (jaise Kafka partition per user, ya a dedicated queue service) chahiye. Backend mein, jab tum ek naya microservice design karte ho, uske NFRs (availability target, latency SLO) hi decide karte hain ki tumhe circuit breakers chahiye, retries chahiye, kitne replicas chahiye, aur kaunsa consistency model chalega.

```text
Functional requirements (URL shortener):
  - User submits a long URL, gets back a short code
  - Visiting the short URL redirects to the original
  - (optional) Custom aliases, expiry, analytics

Non-functional requirements:
  - Latency: redirect p99 < 50ms
  - Availability: 99.99% (SLA) -> ~4.3 min downtime/month allowed
  - Consistency: eventual OK for analytics, but code->URL mapping must be
    immediately consistent after creation (no "not found" right after create)
  - Scale: 100M redirects/day (~1150 QPS avg, ~5000 QPS peak)
  - Durability: mapping data must never be lost (no data loss even on node failure)
```

**Quick summary:** FR = system kya karta hai, NFR = system kaisa perform/behave karta hai (latency, availability, consistency, scale) — SLA/SLO/SLI in NFRs ko measurable banate hain, aur asli architecture NFRs se decide hoti hai.

**Practice task(s):**
- Apne last built feature ke 3 FRs aur 3 NFRs likho — agar NFRs kabhi explicitly discuss nahi hui thi, socho unka implicit default kya tha.
- Ek SLO likho ("p99 latency < X ms") kisi API ke liye jo tumne use kiya ho, aur socho wo kaise measure (SLI) hoga.

---

### 3. Trade-off vocabulary: latency vs throughput, consistency vs availability, cost vs performance, simplicity vs flexibility, coupling vs autonomy

**Kya hai:** System design mein "best" solution nahi hota — har decision kisi cheez ko doosri cheez ke against **trade** karta hai. Ye 5 common trade-off pairs har design discussion mein baar-baar aate hain:
- **Latency vs throughput:** Latency = ek single request ko complete hone mein kitna time lagta hai. Throughput = total kitne requests/data per second system process kar sakta hai. Inhe badhane ke tareeke often conflict karte hain (batching se throughput badhta hai, latency badhti hai).
- **Consistency vs availability:** (CAP theorem se related) — network partition ke waqt, ya to har node latest data dikhaye (consistency, lekin kuch nodes down/unreachable lag sakte hain) ya har node respond kare (availability, lekin stale data mil sakta hai).
- **Cost vs performance:** Zyada fast/reliable infra (SSD, multi-region replicas, bigger instances) zyada paisa lagta hai — har perf improvement ka $$ cost hota hai.
- **Simplicity vs flexibility:** Ek simple, opinionated system samajhna/maintain karna aasan hai lekin naye use-cases ke liye rigid hota hai; ek flexible/configurable system future-proof hai lekin complexity, bugs, aur cognitive load badha deta hai.
- **Coupling vs autonomy:** Tightly coupled components (shared DB, direct calls) fast aur simple hote hain build karne mein lekin teams ek doosre par depend karti hain deploy karne ke liye; loosely coupled/autonomous services (own DB, async events) teams ko independently move karne dete hain lekin operational complexity (distributed transactions, eventual consistency) badha dete hain.

**Kyun important hai:** Ye vocabulary tumhe apni team/interviewer ke saath **precisely** communicate karne deta hai ki tumne conscious trade-off liya hai, random choice nahi. Jab koi pooche "why did you choose eventual consistency here", "consistency vs availability trade-off" bolna sirf keyword-drop nahi hai — ye batata hai tumne CAP theorem ka implication samjha hai us specific decision ke context mein. Production mein bhi, jab do teams disagree karti hain architecture par, aksar wo asal mein isi vocabulary ke different priorities par disagree kar rahi hoti hain (ek team simplicity chahti hai, doosri flexibility) — naming the trade-off explicit conversation ko productive banata hai.

**Kaise kaam karta hai:** Har pair mein ek **mechanism** hota hai jo trade-off create karta hai — ye sirf abstract concept nahi hai:
- Latency vs throughput: batching (Kafka producer `linger.ms`) requests ko group karke bhejta hai — throughput badhta hai (fewer network round-trips per message) lekin har individual message ko queue mein wait karna padta hai (latency badhti hai).
- Consistency vs availability: distributed databases (Cassandra, DynamoDB) replication factor N ke saath likhte hain — agar tum **quorum writes/reads** maango (strong consistency), to kuch replicas unreachable hone par write/read fail ho sakta hai (availability down); agar tum sirf ek replica se confirm le lo (eventual consistency), system available rehta hai lekin stale read possible hai.
- Cost vs performance: caching layer (Redis) add karna latency kam karta hai lekin infra cost + operational complexity badhata hai — ROI calculate karna padta hai.
- Simplicity vs flexibility: monolith ek deploy unit hai (simple) — microservices independently scalable/deployable hain (flexible) lekin service discovery, network calls, distributed debugging add ho jaate hain.
- Coupling vs autonomy: shared database do services ke beech (coupling) — agar ek service schema change kare, doosri break ho sakti hai; separate databases + event-driven sync (autonomy) — har service apni pace par evolve kar sakti hai lekin data consistency ab async hai.

**Real-world example aur backend/system-design connection:** Kafka producers ka `linger.ms` aur `batch.size` config exactly latency-vs-throughput trade-off hai — `linger.ms=0` matlab har message turant bheja jaye (low latency, low throughput per connection), `linger.ms=20` matlab 20ms tak messages batch karo phir bhejo (thoda latency add hota hai per message, lekin overall throughput bahut zyada). DynamoDB mein `ConsistentRead=true` vs default eventual read exactly consistency-vs-availability hai — strong read thoda slower aur kam available (extra coordination chahiye) hota hai lekin latest data guarantee karta hai. Backend design mein jab tum decide karte ho "is service ko apna DB milega ya shared DB use karega", tum explicitly coupling-vs-autonomy trade-off resolve kar rahe ho.

```text
# Latency vs throughput — worked micro-example (Kafka-style batching)

No batching: 1 network round-trip per message
  - per-message latency  = ~2ms (network RTT)
  - throughput           = 1 message / 2ms = 500 msg/sec (per connection)

Batching (linger.ms = 10, batch up to 100 msgs):
  - per-message latency  = up to 10ms (wait for batch to fill or timer)
  - throughput           = 100 messages / (10ms + tiny send time)
                          = ~9,900 msg/sec (per connection)

=> ~20x throughput gain, ~5x worst-case latency cost. Trade-off is explicit,
   tunable via linger.ms — not free, but a deliberate dial.
```

**Quick summary:** Har design decision in 5 axes (latency/throughput, consistency/availability, cost/performance, simplicity/flexibility, coupling/autonomy) mein kahin na kahin trade hoti hai — inhe naam se pehchanna tumhe conscious, defensible decisions lene deta hai.

**Practice task(s):**
- Apne kisi production system mein ek jagah dhoondo jaha consistency-vs-availability trade-off explicitly (ya implicitly) liya gaya ho — likho konsa side chuna gaya aur kyun.
- Kafka ya kisi similar tool ki batching config (`linger.ms`, `batch.size`) docs padho aur explain karo wo latency-vs-throughput dial ko kaise control karta hai.

---

### Subtopics — practical aha points

**Requirement clarification questions checklist (scale, read/write ratio, consistency, latency SLO, geography, retention, growth, budget, team size):** Ye ek ready-to-use checklist hai jo interview ke pehle 5 minute mein ya real design doc likhte waqt use hoti hai taaki koi critical NFR miss na ho: **scale** (kitne total users/DAU, kitna data volume), **read/write ratio** (Twitter jaisa read-heavy hai ~100:1, chat app write-heavy ho sakta hai), **consistency** (strong chahiye ya eventual chalega), **latency SLO** (p50/p99 target kya hai), **geography** (single-region ya global users — cross-region latency matter karega), **retention** (data kitne time tak rakhna hai — 30 din ya forever), **growth** (agle 1-2 saal mein kitna badhega — abhi design karte waqt hi headroom chahiye), **budget** (paisa unlimited nahi hota, cost-performance trade-off yahin se aata hai), **team size** (2-log team ek complex distributed system maintain nahi kar payegi — operational complexity team ki capacity se match honi chahiye). *Fayda:* ye checklist tumhe ek systematic order deta hai puchne ka, taaki tum kabhi bhool na jao "geography" ya "team size" jaisi cheez jo architecture ko fundamentally badal sakti hai (e.g. global users ka matlab multi-region + higher consistency cost).

**Drawing systems — C4 model (Context, Container, Component, Code); sequence diagrams; data-flow diagrams:** **C4 model** ek layered way hai architecture diagrams banane ka, zoom-level ke hisaab se: **Context** diagram (system ek black box hai, uske users aur external systems ke saath — sabse high level, non-technical audience ke liye bhi samajh aata hai), **Container** diagram (system ke andar deployable units — web app, API, DB, queue — ek level zoom-in), **Component** diagram (ek container ke andar ke major modules/classes), **Code** diagram (class diagrams — rarely drawn, IDE generate kar deta hai). **Sequence diagrams** time-ordered interactions dikhate hain (kaunsa service kisko call karta hai kis order mein) — race conditions aur async flows samajhne ke liye best hain. **Data-flow diagrams** data kaha se aata hai, kaise transform hota hai, kaha jata hai — ye batate hain, control flow nahi. *Fayda:* `interview whiteboard mein "Container" level diagram usually kaafi hota hai — Context bahut high-level hai, Component bahut low-level. Sequence diagram tab nikalo jab ek specific tricky interaction (e.g. "distributed lock kaise acquire hoga") explain karni ho.`

**Napkin math before diagrams; diagrams before code:** Ye ek **ordering principle** hai — pehle rough numbers nikaalo (kitna QPS, kitna storage, kitna bandwidth — "napkin math" / **back-of-envelope estimation**), phir un numbers ke hisaab se high-level diagram banao (agar QPS bahut zyada hai to load balancer + horizontal scaling zaroor dikhna chahiye diagram mein), aur tabhi jaakar actual code/implementation detail mein jao. Reverse order mein jaana — seedha code likhna bina estimate kiye — sabse common galti hai jismein log premature optimization ya premature simplification kar dete hain. *Fayda:* Agar `napkin math` bataye ki system ko sirf 10 QPS handle karna hai, to distributed system design karna waste of effort hai — ek single Postgres instance kaafi hai. Agar math bataye 1M QPS chahiye, to single instance discuss karna hi galat hai — ye order tumhara time sahi jagah invest karwata hai.

---

### Advanced concepts

**Explicitly naming your failure domains and blast radius in every design:** Ek **failure domain** wo boundary hai jiske andar ek failure contain rehti hai (e.g. ek availability zone, ek shard, ek service instance). **Blast radius** batata hai agar ye domain fail ho jaye to kitna impact hoga (kitne users affected, kaunse downstream services break honge). Senior-level design reviews mein sirf "high-level design" dikhana kaafi nahi hai — tumhe explicitly bolna padta hai "agar ye database shard down ho jaye, to sirf us shard ke users affected honge (blast radius = 1/N users), baaki system chalta rahega" ya "agar ye single Redis instance down ho jaye, poora system down ho jayega (blast radius = 100%, single point of failure)". *Fayda:* Ye habit tumhe automatically single points of failure dhoondhne par majboor karti hai, aur design reviews mein ye sabse pehla sawal hota hai jo senior engineers puchte hain — "yaha kya fail ho sakta hai, aur kitna toot jayega".

**Reversible vs irreversible decisions (one-way vs two-way doors) — invest review effort proportionally:** Ye Amazon/Jeff Bezos se popularize hua concept hai. **Two-way door** decisions wo hain jo easily undo ho sakte hain (e.g. ek naya API endpoint add karna, ek config value change karna) — inpe zyada time discuss/review karne mein waste mat karo, jaldi decide karke aage badho. **One-way door** decisions wo hain jinhe undo karna mehenga ya impossible hai (e.g. primary database technology choose karna, public API ka breaking contract, data model jo migration ke bina badal na sake) — inpe zyada rigor, review, aur multiple stakeholders ka input lena chahiye. *Fayda:* Ye tumhe decision-making speed aur thoroughness ke beech sahi balance dhoondhne deta hai — har decision ko "one-way door" jaisa treat karna (endless meetings) team ko slow kar deta hai, aur har decision ko "two-way door" jaisa treat karna (rush kar dena) production mein costly mistakes create karta hai jinhe undo karna mushkil hota hai.

**YAGNI vs designing for 10× (rule of thumb: design for 10×, plan to rewrite at 100×):** **YAGNI** ("You Aren't Gonna Need It") kehta hai — wo feature/abstraction mat banao jiski abhi zaroorat nahi hai, sirf "future mein kaam aa sakta hai" sochkar. Lekin system design mein bilkul opposite extreme bhi galat hai — agar tum sirf "abhi ke liye" design karoge (present scale ke liye), to system jaldi hi toot jayega jab thoda bhi grow karega. Industry rule-of-thumb: apna system **10x current scale** ke liye design karo (thoda headroom, bina over-engineer kiye), lekin ye accept karo ki **100x scale** par tumhe ek significant rewrite/re-architecture karni hi padegi — aur ye theek hai, ye failure nahi hai. *Fayda:* Ye extreme premature-optimization (jo YAGNI violate karta hai — "hum 1B users ke liye design kar rahe hain" jab abhi 100 users hain) aur under-engineering (jo "next quarter fir se rewrite karna padega" wala pain deta hai) — dono se bachata hai.

**Second-system effect, premature abstraction, resume-driven architecture — named anti-patterns:** Ye teen named anti-patterns hain jo experienced engineers bhi baar-baar fall karte hain:
- **Second-system effect** (Fred Brooks, *The Mythical Man-Month* se): jab tumne ek successful "v1" system banaya ho (jo constraints ke andar simple/pragmatic tha), tumhara agla "v2" system over-engineered ho jata hai kyunki tum har cheez "sahi" (perfect, general-purpose) banana chahte ho jo v1 mein compromise karni padi thi — result: bloated, over-complex system.
- **Premature abstraction:** kisi cheez ko generic/configurable banana before tumhe do-teen real use-cases dikhe jo justify karein ki abstraction sahi shape mein hai — result: wrong abstraction jo actual use-cases ke liye bhi awkward fit hoti hai (aur wrong abstraction, duplicate code se worse hoti hai kyunki usse remove karna harder hai).
- **Resume-driven architecture:** technology choices sirf isliye kiye jaate hain kyunki wo trendy hain aur engineer ke resume par acha lagega (e.g. har chhoti si app ke liye Kubernetes + microservices + Kafka, jab ek monolith kaafi tha) — business requirement se nahi, career incentive se driven decision.
*Fayda:* Inhe naam se jaanna tumhe apni khud ki design meetings mein "wait, kya hum ye second-system effect kar rahe hain?" jaisa self-check karne deta hai, aur team discussions mein politely inko point out karne ke liye vocabulary deta hai bina personal attack kiye.

```text
# Quick self-check before finalizing a design:

- [ ] Har major component ka failure domain aur blast radius likha hai?
- [ ] Kaunsi decisions one-way doors hain? Unpe extra review lagaya?
- [ ] Design 10x scale handle karega? (Not 100x — that's a future rewrite, that's OK.)
- [ ] Koi abstraction bina 2-3 real use-cases dekhe banayi to nahi?
- [ ] Tech choices requirements se aa rahi hain ya "cool lagega" se?
```

**Quick summary:** Advanced design thinking = failure ko explicitly design ke andar name karna (blast radius), decision-speed ko reversibility ke hisaab se adjust karna, sirf zaroorat-bhar future-proof karna (10x, not 100x), aur apne khud ke known anti-patterns (second-system effect, premature abstraction, resume-driven architecture) ko catch karna.

**Practice task(s):**
- Apne kisi recent design/PR mein ek decision dhoondo aur classify karo — one-way door tha ya two-way door? Kya usko utna hi review mila jitna deserve karta tha?
- Ek system jo tumne recently dekha/banaya, uska failure domain aur blast radius zor se bol kar explain karo — "agar X fail ho jaye to Y hoga".

---

### Important terms

- **NFR (Non-Functional Requirement):** System ki quality attributes — latency, availability, consistency, durability, security, cost, scalability — "kaisa" system hona chahiye, "kya karega" nahi.
- **SLA (Service Level Agreement):** Customer ke saath formal, often contractual promise (penalty ke saath) — e.g. "99.9% uptime guaranteed".
- **SLO (Service Level Objective):** Internal target jo SLA se conservative rakha jata hai (buffer ke liye) — e.g. "p99 latency < 200ms".
- **SLI (Service Level Indicator):** Actual measured metric jo SLO ke against compare hoti hai (e.g. real-time p99 latency dashboard number).
- **CAP theorem:** Distributed system network partition ke waqt consistency aur availability dono simultaneously guarantee nahi kar sakta — kisi ek ko choose karna padta hai.
- **C4 model:** Architecture diagram karne ka layered approach — Context, Container, Component, Code — zoom level ke hisaab se.
- **Failure domain:** Boundary jiske andar ek failure contain rehti hai (zone, shard, instance).
- **Blast radius:** Ek failure hone par kitna impact/scope affected hota hai.
- **One-way door (irreversible decision):** Decision jo undo karna mehenga/impossible hai — extra review chahiye.
- **Two-way door (reversible decision):** Decision jo easily undo ho sakti hai — jaldi decide karo.
- **YAGNI:** "You Aren't Gonna Need It" — jo abhi zaroorat nahi hai wo mat banao.
- **Second-system effect:** Successful simple v1 ke baad over-engineered, bloated v2 banane ki tendency.
- **Premature abstraction:** Bina kaafi real use-cases dekhe generic/configurable code banana — jiski wrong-shape hone ki risk high hoti hai.
- **Resume-driven architecture:** Tech choices jo trendiness/career ke liye ki jaati hain, requirement se nahi.
- **Napkin math / back-of-envelope estimation:** Diagram/code se pehle rough numbers (QPS, storage, bandwidth) nikaalna taaki design ki scale-appropriateness pata chale.
- **Batching (`linger.ms` type config):** Multiple requests/messages ko group karke bhejna — throughput badhta hai, per-item latency badhti hai.

---

### Common mistakes

- Seedha high-level design (boxes-and-arrows) se shuru kar dena bina requirements/constraints clarify kiye — interview aur real RFC dono mein common galti.
- NFRs (latency, availability, consistency targets) ko silently assume kar lena instead of explicitly pooch kar likhna — baad mein pata chalta hai wrong assumption thi.
- Napkin math skip karke seedha "hume microservices/Kafka/sharding chahiye" bol dena bina ye confirm kiye ki scale actually itni bhi hai.
- Har decision ko "one-way door" jaisa treat karna — chhote reversible decisions par bhi hafton discuss karna, momentum kill karna.
- Second-system effect mein fasna — v2 ko itna "generic aur perfect" banane ki koshish karna ki wo v1 se zyada complex aur late ho jaye.
- Failure domains explicitly na batana — design mein single point of failure chupa rehta hai kyunki kabhi zor se bola hi nahi gaya "ye component down hua to kya hoga".
- "Design for 10x" ko "design for 1000x" samajh lena — bahut zyada distant-future scale ke liye abhi hi over-engineer kar dena, jab abhi ki team/timeline ke liye wo waste hai.
- C4 model ke sabhi levels (Context/Container/Component/Code) ek hi diagram mein zabardasti fit karne ki koshish karna — audience confuse ho jaati hai kyunki zoom-level mix ho jata hai.

---

### Interview questions

1. **7-step design method ke steps bolo aur batao har step kyun us order mein hai.** — Hint: requirements/constraints pehle isliye kyunki wo baaki sab kuch constrain karte hain; deep dives end mein isliye kyunki wo high-level design ke baad hi meaningful hain.
2. **FR aur NFR mein farak batao, aur ek example do jaha NFR ne architecture fundamentally badli ho.** — Hint: URL shortener jaisa simple CRUD app tabhi complex banta hai jab NFR ("100M reads/day, p99<50ms") add hoti hai.
3. **SLA, SLO, aur SLI mein farak kya hai?** — Hint: SLA = external promise/contract, SLO = internal (usually tighter) target, SLI = actually measured real number.
4. **Latency aur throughput mein trade-off kyun hota hai? Ek concrete example do.** — Hint: batching (`linger.ms`) — batch fill hone ka wait individual latency badhata hai, lekin per-round-trip zyada data jaane se throughput badhta hai.
5. **CAP theorem explain karo apne alfaazon mein, ek real database example ke saath.** — Hint: network partition ke waqt Cassandra/DynamoDB jaise systems eventual consistency choose karte hain availability ke liye; traditional RDBMS (single-node) ye trade-off avoid karta hai kyunki wo partition hi nahi hota single node mein.
6. **C4 model ke 4 levels kya hain, aur interview whiteboard mein kaunsa level sabse useful hota hai?** — Hint: Context, Container, Component, Code — usually Container level sabse useful hota hai interview mein (zyada high-level Context useless hai, zyada low-level Code out-of-scope hai).
7. **One-way door aur two-way door decisions mein farak batao, ek-ek example ke saath.** — Hint: primary database choice = one-way door (migration mehenga); ek cache TTL value = two-way door (easily change ho sakta hai).
8. **"Design for 10x, rewrite at 100x" ka matlab kya hai — ye YAGNI se contradict kyun nahi karta?** — Hint: YAGNI features/abstractions ke baare mein hai jo abhi zaroorat nahi; 10x scale headroom ek NFR hai jo already tumhare current requirements ka part honi chahiye, speculative feature nahi.
9. **Second-system effect aur premature abstraction mein similarity/difference batao.** — Hint: dono over-engineering ke forms hain — second-system effect specifically "v1 ke baad v2" pattern hai, premature abstraction kabhi bhi ho sakta hai (bina kaafi use-cases dekhe generalize karna).
10. **Failure domain aur blast radius define karo, aur explain karo ye design review mein kaise use hote hain.** — Hint: har major component ke liye pucho "ye fail ho to kya, aur kitna toot jayega" — jo answer "sab kuch" ho, wahi single point of failure hai jo fix hona chahiye.

---

### Hands-on lab

> Design a URL shortener using the 7 steps in exactly 45 minutes, timed, on paper.

**Ye specific lab kyun:** Ye lab pura section ka **synthesis test** hai — sirf padh lene se pata nahi chalta ki tum 7-step method ko **time pressure ke under, bina notes ke, paper par** actually apply kar sakte ho ya nahi. 45 minutes ka timer real interview conditions simulate karta hai. "Paper par" likhna zaroori hai kyunki whiteboard/paper par likhna vs keyboard par type karna alag muscle hai — diagram banana, arrows draw karna, space constraint ke saath kaam karna. Ye ek baar karne se pata chal jayega ki kaunse steps tumhe slow kar rahe hain (usually requirements-gathering mein sabse zyada time waste hota hai agar practice nahi hai), aur agle attempts mein wahi gap improve hota hai.

- [ ] Mera result: Requirements+Constraints mein kitna time laga = ______, API+Data model mein kitna time laga = ______, High-level design mein kitna time laga = ______, Deep dive + Bottlenecks mein kitna time laga = ______, Total 45 min mein complete hua? = ______

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## ✅ Exit gate for Level 0

You can, without notes:
1. Recite the latency table to the right order of magnitude and explain why cross-region writes cap at ~70 ms RTT.
   *(Covered in: 0.2 — canonical latency table; speed-of-light / cross-region RTT subtopic.)*
2. Explain why a B+tree suits disk and a hash table suits memory.
   *(Covered in: 0.3 — trees & hash tables topics; 0.1 — memory hierarchy (RAM vs disk) underpins the "why".)*
3. Compute availability of a 6-service serial call chain, and say whether MTBF or MTTR is the better lever.
   *(Covered in: 0.6 — series/parallel composition and MTBF/MTTR advanced concepts.)*
4. Estimate servers, storage, and bandwidth for a 50 M DAU app in under 10 minutes.
   *(Covered in: 0.6 — QPS, storage, and bandwidth estimation topics, plus the hands-on lab.)*
5. Explain why p99 latency is more actionable than average latency, with a fan-out example.
   *(Covered in: 0.2 — percentiles and tail-at-scale/fan-out amplification advanced concepts.)*

---
