# Notes — Level 0 — Prerequisites & Mental Models

> Source: [`../ROADMAP.md`](../ROADMAP.md), started ______, passed exit gate ______

## 0.1 Computer architecture as a performance model

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain (interview/docs mein wahi use hote hain), samjhaya Hindi-English mix mein hai. Har point ke saath: **kya hai → kyun important hai (fayda) → example.**

### 1. CPU, cores, sockets, NUMA nodes

**Kya hai:** Ek server mein sirf ek "brain" nahi hota. Usme multiple **sockets** (alag physical CPU chips) ho sakte hain, har socket ke andar multiple **cores**, aur har core aksar 2 **hardware threads** (hyperthreading) run karta hai jo resources share karte hain. Memory har socket se directly juda hota hai — isi ko **NUMA** (Non-Uniform Memory Access) kehte hain: apne khud ke socket ki memory fast milti hai, doosre socket ki memory access karne mein zyada time lagta hai.

**Kyun important hai:** Agar ye pata nahi hoga, to soch loge "jitne zyada cores utna zyada throughput" — lekin real mein 96-core wala box kabhi-kabhi 16-core jaisa perform karta hai kisi workload mein. Ye samajhna production ke "random slowness" wale mysteries solve karta hai.

**Example:** Dual-socket, 48-core machine par Postgres ko tune karte waqt uske buffer pool aur worker threads ko ek hi NUMA node par pin kiya jata hai. Agar aisa na karein, to queries baar-baar "doosre socket" ki memory access karengi — latency badhegi, monitoring mein sirf "random slow query" dikhega, root cause samajh nahi aayega jab tak NUMA ka pata na ho.

---

### 2. Memory hierarchy: registers → L1/L2/L3 cache → RAM → SSD → HDD → network → tape

**Kya hai:** CPU se jitna door data hota hai, utni der lagti hai use fetch karne mein — aur ye gap orders-of-magnitude ka hota hai:

| Layer | Roughly kitna time |
|---|---|
| Register | <1 ns |
| L1 cache | ~1 ns |
| L2 cache | ~4 ns |
| L3 cache | ~10–20 ns |
| RAM | ~100 ns |
| SSD | ~50–150 µs |
| HDD | ~5–10 ms |
| Network (same datacenter) | ~0.5 ms |

**Kyun important hai:** Ye table hi batata hai ki software ke har layer mein caching kyun hoti hai (CPU cache, app cache, CDN, DNS cache). Jab pata chal jaye ki RAM network hop se ~1,00,000x fast hai, to "bas cache kar do" sirf buzzword nahi rehta — ek measurable, predictable engineering decision ban jata hai.

**Example:** User profile Redis (RAM, ~1ms including network) se fetch karna vs Postgres-on-disk-plus-network (~10–50ms) se — ye koi "nice to have" nahi hai, ye 10-50x latency ka fayda hai jo aap build karne se pehle hi is table se predict kar sakte ho.

---

### 3. Cache lines (64B), spatial/temporal locality, cache misses

**Kya hai:** CPU RAM se ek-ek byte nahi laata, wo ek pura 64-byte **cache line** fetch karta hai ek baar mein. "Spatial locality" ka matlab hai — nearby memory addresses cache mein saath fetch ho jate hain. "Temporal locality" ka matlab — jo data abhi use hua hai, wo phir se jaldi use hoga. **Cache miss** tab hota hai jab CPU ko chahiye wala data kisi bhi cache mein nahi hota, to RAM tak jaana padta hai — jo mehenga hai.

**Kyun important hai:** Ye samjhata hai ki data ko memory mein kaise arrange kiya gaya hai, ye bhi utna hi matter karta hai jitna algorithm ka Big-O. Same complexity ke do algorithms real speed mein 10x tak alag ho sakte hain sirf cache-friendliness ki wajah se.

**Example:** 2D array ko row-by-row iterate karna (jaise memory mein stored hota hai) vs column-by-column karna. Row-by-row sequential cache lines hit karta hai (fast); column-by-column har baar memory mein door jump karta hai — almost har access par cache miss — same O(n²) hone ke bawajood, real time bahut alag hota hai.

---

### 4. Instruction pipelining, branch prediction, speculative execution, SIMD

**Kya hai:** CPU ek instruction complete karke doosri start nahi karta — wo kai instructions ko overlapping stages mein **pipeline** karta hai. `if` branches ke through pipeline ko busy rakhne ke liye CPU **predict** karta hai ki branch kis taraf jayega aur speculatively aage execute karta hai. **SIMD** (Single Instruction, Multiple Data) ek instruction se multiple data elements par kaam karta hai ek hi cycle mein (jaise 8 numbers ek saath add karna).

**Kyun important hai:** Ye batata hai ki **unpredictable** branches (jaise `if (random() > 0.5)`) predictable branches se slow kyun hote hain, aur numeric/data-heavy code (image processing, ML, compression) SIMD-friendly loops likhne se dramatically fast kyun ho sakta hai.

**Example:** Bade array ko sort karna pehle, phir filter loop (`if (x > threshold)`) chalana — branch predictor ke liye aasan ho jata hai (long runs of true/false), loop fast ho jata hai — vs unsorted data filter karna jahan branch essentially coin-flip jaisa hota hai har iteration mein, pipeline stalls hote hain (har mispredict ~15-20 cycles waste karta hai).

---

### Subtopics — practical "aha" points

**Array traversal linked-list se fast hai, same Big-O hone ke bawajood:** Dono O(n) hain traverse karne mein. Lekin array memory mein contiguous (lagatar) hota hai — cache-friendly. Linked list ke nodes memory mein bikhre hote hain — har `next` pointer likely ek cache miss hai. *Fayda:* samajh aata hai ki Big-O hamesha poori kahani nahi batata — real-world constant factors (memory layout) typical data sizes ke liye zyada matter karte hain.

**False sharing:** Do threads jab **alag-alag** variables likhte hain jo memory mein same 64-byte cache line par hote hain, to unka cache baar-baar invalidate hota rehta hai — even though data unrelated hai. *Fayda:* multi-threaded slowdowns samajh aate hain jaha zyada threads add karne se cheezein slow ho jati hain. Fix: struct ko pad karo taaki hot variables alag cache lines par land karein.

**Sequential vs random access SSD vs HDD par:** Sequential reads dono par fast hain; random reads SSD par fast hain lekin HDD par bahut slow (mechanical seek, ~5-10ms). *Fayda:* ye samjhata hai ki database storage engines (B-trees, LSM-trees) bilkul isi fact ke around design kiye gaye hain — random disk I/O minimize karna hi unka poora design philosophy hai (Level 3 isi par build hota hai).

**Page size (4KB), page cache, `mmap`, dirty pages:** OS disk pages ko RAM mein transparently cache karta hai (page cache). "Dirty page" wo hoti hai jo RAM mein modify hui hai lekin abhi disk par flush nahi hui. *Fayda:* samajh aata hai ki server "disk" data ke liye bhi fast reads kaise deta hai (actually wo RAM mein page cache ke through hota hai), aur sudden power loss recent writes lose kyun kar sakta hai jo sirf dirty page cache mein the, `fsync` nahi hua tha.

**DMA, zero-copy (`sendfile`, `splice`):** Normally disk se network tak data bhejne ke liye multiple CPU copies lagti hain (disk → kernel buffer → user buffer → socket buffer). **Zero-copy** techniques kernel ko directly data move karne dete hain, CPU copies skip karke. *Fayda:* Kafka aur Nginx ka itna zyada throughput isi wajah se possible hai — unnecessary copies bilkul avoid karte hain.

**Example:** Nginx static file serve karte waqt `sendfile()` use karta hai taaki file disk cache se directly network socket tak jaye, application memory ke through kabhi copy hue bina. Yahi ek badi wajah hai Nginx naive app-server file handler se better perform karta hai.

---

### Advanced concepts

**NUMA-aware allocation & thread pinning:** Jaisa upar bataya — 96-core box actually kai chhote NUMA-local machines hain jo aapas mein jude hain. *Fayda:* samajh aata hai ki high-performance systems (Cassandra, Postgres tuning guides) NUMA nodes par processes pin karne ki recommendation kyun karte hain.

**Mechanical sympathy (struct-of-arrays vs array-of-structs):** Agar ek bade struct mein se sirf ek field baar-baar chahiye, to fields ko separate parallel arrays mein store karna (struct-of-arrays) ek array of full structs se zyada cache-efficient hai. *Fayda:* ye hi core idea hai jiski wajah se columnar databases (ClickHouse, Parquet) analytics ke liye fast hote hain — sirf zaroori columns hi contiguously padhte hain.

**Hardware failure statistics (bit rot, AFR, checksums):** Disks kabhi-kabhi silently fail hote hain — bits flip ho jate hain bina kisi obvious error ke. *Fayda:* samajh aata hai ki har serious storage system (ZFS, Cassandra, S3) data par checksums kyun use karta hai — "disk ne complain nahi kiya" iska matlab data correct hai, ye proof nahi hai.

**Modern storage (NVMe queue depth, write amplification, wear leveling, TRIM):** SSDs in-place overwrite nahi kar sakte — wo block mein erase karke kahin aur likhte hain, jisse **write amplification** hota hai (ek 4KB logical write bahut zyada physical write activity cause kar sakta hai). *Fayda:* samajh aata hai ki SSDs fill hone ke saath slow kyun ho jate hain, aur databases jo SSD ke liye tune kiye gaye hain (log-structured/sequential writes ko favor karte hain) naive random-write patterns se better kyun perform karte hain.

---

### Hands-on lab (isi section ka asli point)

> 1 GB data ko sequentially vs randomly traverse karne ka benchmark likho — RAM mein bhi aur disk par bhi. Chaaron numbers plot karo. Inhe sambhaal kar rakho.

**Ye specific lab kyun:** Ye upar ki saari cheezein "padhi hui facts" se "khud measure kiye hue numbers" mein badal deta hai. Tumhe 4 data points milenge (RAM-sequential, RAM-random, disk-sequential, disk-random) jo orders of magnitude alag honge — aur yahi gap caching, indexing, aur storage-engine design ke exist karne ki poori wajah hai. Aage ke saare levels inhi numbers ko reference karenge.

- [ ] Mera result: RAM-sequential = ______, RAM-random = ______, Disk-sequential = ______, Disk-random = ______

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.2 Latency numbers every engineer must know

## 0.3 Data structures & algorithms as design tools

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain, samjhaya Hindi-English mix mein hai. Har topic: **kya hai → kyun important hai → example.**

### 1. Basic linear structures: Arrays, dynamic arrays, linked lists, stacks, queues, deques, ring buffers

**Kya hai:** **Array** — fixed-size, memory mein contiguous block. **Dynamic array** (Python list, Java ArrayList) — jab full ho jaye to bada naya array banake copy karta hai (amortized growth). **Linked list** — nodes memory mein bikhre hue, pointers se jude. **Stack** (LIFO), **Queue** (FIFO), **Deque** (dono taraf se insert/remove), **Ring buffer** (fixed-size circular array, purana data overwrite hota hai).

**Kyun important hai:** Ye "vocabulary" hai jisse tum kisi bhi system ka internal design describe kar sakte ho. Ring buffer jaisi cheez directly production systems mein use hoti hai (logging, streaming).

**Example:** Kafka ka producer buffer internally ek ring buffer jaisa structure use karta hai — fixed memory mein continuously naye messages aate rehte hain, purane consume ho chuke messages ki jagah overwrite ho jati hai. Rate limiter bhi ring buffer se implement hota hai (sliding window counters).

---

### 2. Hash tables: hashing, collisions, load factor, resizing

**Kya hai:** Hash table ek key ko hash function se ek index mein convert karta hai, taaki O(1) average lookup mile. **Collision** tab hota hai jab do keys ka hash same index par aa jaye — do tarike se handle hota hai: **chaining** (usi index par linked list/bucket) ya **open addressing** (agla khaali slot dhoondo). **Load factor** = entries / buckets — jitna high hoga utni zyada collisions. Jab load factor limit cross karta hai, table **resize** (rehash) hoti hai — sab kuch naye, bade table mein move hota hai.

**Kyun important hai:** Har language ka dictionary/map/set internally yahi hai. Resize operation ek "pause" create karta hai — agar tumhare production system mein p99 latency mein achanak spike aata hai, ho sakta hai wo ek internal hash table resize ki wajah se ho (ya GC pause isi se related).

**Example:** Redis ka hash table jab bahut bada ho jata hai, to wo **incremental rehashing** karta hai (ek saath sab move nahi karta, thoda-thoda) — taaki ek single resize operation poore server ko block na kare. Ye design choice isi problem (resize pause) ko solve karne ke liye hai.

---

### 3. Trees: BST, balanced trees (AVL/red-black), B-tree/B+tree, tries, heaps, skip lists

**Kya hai:** **BST** (Binary Search Tree) — left chhota, right bada, lekin skewed ho sakta hai (worst case O(n)). **AVL/Red-Black tree** — self-balancing, height hamesha O(log n) guarantee karte hain. **B-tree/B+tree** — ek node mein multiple keys/children (high fanout), disk-optimized. **Trie** — string ke characters ko tree mein path banake store karta hai (prefix search fast). **Heap** — priority queue ke liye, min/max jaldi milta hai O(1), insert/remove O(log n). **Skip list** — linked list + multiple levels, probabilistically balanced, O(log n) search.

**Kyun important hai:** Ye samajhna Level 3 (Databases) ke liye foundation hai. Almost har database index ka core structure yahin se aata hai.

**Example:** Postgres/MySQL ka default index **B+tree** hai — kyunki B+tree ka high fanout (ek node mein 100+ keys) matlab hai billions rows ke liye bhi tree ki height sirf 3-4 levels rehti hai, isliye disk se sirf 3-4 reads mein koi bhi row mil jati hai. Redis ka Sorted Set skip list se implement hota hai (leaderboards ke liye).

---

### 4. Graphs: representations, BFS/DFS, topological sort, shortest path

**Kya hai:** Graph = nodes + edges. Represent karne ke do common tarike: **adjacency list** (har node ki list of neighbors — sparse graphs ke liye efficient) aur **adjacency matrix** (NxN grid — dense graphs, O(1) edge check). **BFS** (level-by-level, shortest path unweighted graph mein), **DFS** (depth-first, cycle detection, backtracking). **Topological sort** — dependency order (jaise build systems, task scheduling). **Shortest path** algorithms — Dijkstra (non-negative weights), Bellman-Ford (negative weights allowed).

**Kyun important hai:** Bahut saari real systems graph problems hain jo dikhte nahi — service dependency graphs, social network connections, recommendation systems, routing.

**Example:** CI/CD pipeline ya build system (Bazel, Make) task dependencies ko graph banake **topological sort** se execute order decide karta hai — taaki koi task apni dependency se pehle na chale. Microservices ka dependency graph samajhna incident response mein critical hota hai (kaunsi service down hone se kya-kya affect hoga).

---

### 5. Sorting & searching, external sorting (merge sort on disk)

**Kya hai:** Normal sorting (quicksort, mergesort) data ko RAM mein fit maan ke chalte hain. **External sorting** tab use hota hai jab data RAM se bada ho (jaise disk par GBs/TBs ka data) — data ko chhote chunks mein sort karke disk par likha jata hai, phir un sorted chunks ko **merge** kiya jata hai (merge sort ka merge step, disk-friendly kyunki sequential read hai).

**Kyun important hai:** Bade data processing systems (Hadoop, Spark, database's `ORDER BY` on huge tables) isi technique par based hain.

**Example:** Jab tum Postgres mein `ORDER BY` karte ho ek bahut badi table par jo RAM (`work_mem`) mein fit nahi hoti, to Postgres internally external merge sort use karta hai — chunks ko temp disk files mein sort karke likhta hai, phir merge karta hai. `EXPLAIN ANALYZE` mein "external merge Disk" dikhna performance red flag hai.

---

### Subtopics — deeper insights

**Amortized vs worst-case complexity:** Dynamic array ka `append` "average" O(1) hai (amortized), lekin jis moment resize hota hai, wo ek single call O(n) leta hai. *Fayda:* p99 latency worst-case se affect hoti hai, average se nahi — isliye "amortized O(1)" sunke bhi tail latency samajhna zaroori hai (jaise rehash pause, GC pause).

**B+tree on-disk indexes kyun dominate karte hain:** High fanout (ek node = ek disk page, usme 100+ keys fit ho jati hain) → height sirf 3-4 levels billions rows ke liye bhi → matlab kisi bhi row tak pahunchne ke liye sirf 3-4 disk reads chahiye. *Fayda:* ye samajhna hai ki "node = page" design choice hi B+tree ko disk ke liye perfect banata hai.

**LSM-tree structure:** Memtable (RAM mein sorted structure, naye writes yahin jate hain) → immutable memtable → flush disk par as SSTable (Sorted String Table) → background mein **compaction** (multiple SSTables ko merge karke purane/duplicate data clean karna). *Fayda:* ye Level 3 mein B-tree se compare hoga — LSM writes ke liye fast hai (sequential disk writes), reads thoda slower ho sakte hain (multiple SSTables check karne padte hain).

**Priority queues for schedulers/timers/rate limiters:** Heap-based priority queue se "next timer kab fire hoga" ya "next task kya hai priority ke hisaab se" O(log n) mein pata chal jata hai. *Example:* OS scheduler, cron-like delayed job queues, rate limiter ka token bucket internally isi tarah manage hota hai.

---

### Advanced: Probabilistic & sketch structures (real systems mein bohot use hote hain)

Ye sab structures **approximate** answer dete hain, lekin bahut kam memory mein — jab exact answer ki zaroorat nahi hoti tab ye life-saver hain.

**Bloom filter** — "kya ye element set mein hai?" — false positive ho sakta hai (bolega "haan" jab actually nahi hai), lekin false negative kabhi nahi (agar "nahi" bola to pakka nahi hai). Sizing formula: `m = -n·ln(p)/(ln2)²`. *Kahan use hota hai:* LSM-tree reads mein — pehle Bloom filter check karo, agar wo bole "nahi hai" to us SSTable ko disk se padhna hi nahi padega. Massive I/O savings.

**Cuckoo filter / Quotient filter** — Bloom filter jaisa hi, lekin isme elements **delete** bhi kar sakte ho (Bloom filter mein delete karna mushkil hai).

**HyperLogLog** — sirf ~1.5 KB memory mein millions/billions unique items ka count estimate kar deta hai (~2% error ke saath). *Example:* "kal kitne unique visitors the" — exact count ke liye har visitor ID store karna padta, HyperLogLog se sirf 1.5KB mein approximate answer mil jata hai.

**Count-Min Sketch** — "kaunsi cheez sabse zyada baar aayi" (frequency estimation) kam memory mein. *Example:* "trending hashtags" ya "hot keys" detect karna without storing exact count of every single item.

**t-digest / HdrHistogram** — percentiles (p50, p99) ko mergeable tarike se estimate karte hain. *Example:* Metrics systems (Prometheus, monitoring dashboards) jo distributed servers se percentile latency combine karte hain, isi ka use karte hain.

**MinHash / SimHash / LSH** — do documents/items kitne similar hain, ye jaldi estimate karne ke liye. *Example:* Duplicate content detection (plagiarism check, spam detection near-duplicate emails).

**Merkle trees** — data ke chunks ka hash-tree banate hain taaki do replicas ke beech sirf "kaunsa hissa different hai" jaldi pata chal jaye, poora data compare kiye bina. *Example:* Cassandra/Dynamo mein anti-entropy repair, Git commits, blockchain.

**Consistent hashing & rendezvous (HRW) hashing** — jab ek naya server add/remove hota hai, minimum keys hi reshuffle hoti hain (poora remap nahi hota). *Example:* Distributed caches (Memcached clusters), CDN routing, database sharding.

**Inverted index** — word → uss word wale documents ki list. *Example:* Search engines (Elasticsearch) ka core structure — "system design" search karo to turant un documents ki list mil jati hai jinme ye words hain.

**Trie / FST** — prefix-based lookups fast. *Example:* Autocomplete (Google search suggestions), IP routing tables (longest-prefix match).

**Roaring bitmaps** — bade sets par compressed AND/OR/NOT operations fast karte hain. *Example:* Analytics queries jaise "in dono segments mein kaun log common hain" — bitmap AND operation se instantly milta hai.

**Ring buffer / disruptor pattern** — lock-free, ek producer-ek consumer ke beech data pass karne ka fastest tarika. *Example:* High-frequency trading systems, LMAX Disruptor.

---

### Hands-on lab

> Bloom filter aur Count-Min Sketch ko scratch se implement karo. Apna measured false-positive rate formula wale expected rate se compare karo.

**Ye lab kyun important hai:** Jab tum khud Bloom filter banaoge, formula `m = -n·ln(p)/(ln2)²` sirf theory nahi rahegi — tumhe pata chalega ki bits kam karne se false-positive rate kaise badhta hai, aur real systems (databases) itni memory kyun dedicate karte hain isko sahi tune karne ke liye.

- [x] C++ implementation: [`labs/0.3-bloom-countmin/`](labs/0.3-bloom-countmin/)
  - Bloom filter: measured false-positive rate = **0.00972** (formula predicted 0.01, n=100000)
  - Count-Min Sketch: max observed error = 1092, theoretical bound = 2000 (eps=0.001, delta=0.01) — koi bhi underestimate nahi mila (jaisa expect tha)

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.4 Operating systems for system designers

> Hinglish mein detailed notes — technical terms English mein hi rakhe hain, samjhaya Hindi-English mix mein hai. Har topic: **kya hai → kyun important hai → example.**

### 1. Processes vs threads vs coroutines; the scheduler; context switch cost

**Kya hai:** **Process** apna khud ka independent memory address space rakhta hai (isolated — ek process crash ho to doosra safe rehta hai). **Thread** ek process ke andar chalta hai aur us process ka memory (heap, globals) doosre threads ke saath **share** karta hai — sirf apna stack aur registers alag hote hain. **Coroutine/green thread** (goroutine, Python async, Kotlin coroutine) ek "user-space thread" hai — OS ko iske baare mein pata bhi nahi hota, language runtime khud isko schedule karta hai. **Scheduler** (OS ka ya runtime ka) decide karta hai kaunsa thread/process CPU par kab chalega. **Context switch** — jab CPU ek thread/process ko roक kar doosra chalata hai, to registers, program counter, stack pointer save-restore karne padte hain — isme ~1–5 µs lagta hai (aur cache/TLB bhi "cold" ho jata hai naye thread ke liye).

**Kyun important hai:** Ye teeno options ek hi problem (concurrency) ke alag-alag price points hain — process sabse safe lekin sabse mehenga (context switch + no shared memory, IPC chahiye), thread sasta hai lekin shared-memory bugs ka risk, coroutine sabse sasta hai (context switch sirf user-space mein, ~nanoseconds) lekin ek hi OS thread par run hone ka matlab hai ek blocking call poori coroutine set ko rok sakta hai agar sambhal kar na likha jaye.

**Example:** Nginx worker processes use karta hai (process-per-worker, isolation ke liye — ek worker crash se poora server down nahi hota), jabki Go ka HTTP server har request ke liye ek **goroutine** spawn karta hai (lakhon goroutines cheaply chal sakti hain kyunki context switch OS-level nahi, runtime-level hai). Java ka thread-per-request model (Tomcat) OS threads use karta tha — 10k concurrent connections ka matlab 10k OS threads, jo context-switch overhead se hi server ko struggle karwa deta tha (isi wajah se Project Loom/virtual threads aaye).

---

### 2. Virtual memory, page tables, TLB, swap, OOM killer

**Kya hai:** Har process ko OS ek **virtual address space** ka illusion deta hai — jaise use poori RAM akela mil rahi ho. **Page table** virtual addresses ko actual physical RAM addresses mein translate karta hai (per-process, kernel maintain karta hai). Ye translation har memory access par honi chahiye, isliye CPU ke paas ek chhota, super-fast cache hota hai isi ke liye — **TLB** (Translation Lookaside Buffer). TLB miss hone par CPU ko page table ko RAM se walk karke padhna padta hai (multiple extra memory accesses — mehenga). **Swap** — jab RAM khatam ho jaye, OS kam-used pages ko disk par "swap out" kar deta hai taaki RAM free ho. **OOM killer** (Out-Of-Memory killer, Linux) — jab RAM + swap dono khatam ho jayein, kernel forcibly kisi process ko kill kar deta hai (heuristic score se decide karta hai kisko marna hai) taaki system crash na ho.

**Kyun important hai:** Virtual memory hi wo abstraction hai jo processes ko ek-doosre se isolate karta hai (ek process doosre ka memory directly access nahi kar sakta). Swap "RAM khatam hui to bas slow ho jayega" jaisa lagta hai lekin practically disk RAM se 1000x+ slow hai — ek database jo swap karna shuru kar de, effectively "hung" jaisa dikhta hai. OOM killer production ka ek classic mystery hai: application log mein koi error nahi, process bas achanak gayab ho jata hai (`dmesg` mein "Killed process" milta hai).

**Example:** Container ko memory limit diya (jaise `docker run -m 512m`) aur application usse zyada use kare, to Linux cgroup OOM killer us container ke process ko turant kill kar dega — application ko clean shutdown ka mauka bhi nahi milega. Isi wajah se JVM heap size ko container memory limit se kam set karna padta hai (JVM khud ka GC/metaspace bhi memory leta hai jo heap se bahar hai) — warna "randomly" pod restart hote rehte hain Kubernetes mein.

---

### 3. File systems: inodes, page cache, `fsync`, journaling, durability guarantees

**Kya hai:** **Inode** ek file ka metadata structure hai (size, permissions, timestamps, aur disk par data blocks ke pointers) — filename khud inode mein nahi hota, wo directory entry mein hota hai jo inode number se link karta hai. **Page cache** — OS disk ke blocks ko RAM mein cache karta hai transparently; ek `write()` call actually sirf page cache mein likhti hai (fast), disk tak turant nahi jaati. **`fsync()`** ek system call hai jo application ko explicitly bolne deta hai "ab is data ko physically disk tak pahुँचao, mujhe guarantee chahiye" — isse pehle data sirf RAM (page cache) mein hai, power loss mein gayab ho sakta hai. **Journaling** file system apne metadata changes (aur options se data changes bhi) ko pehle ek chhote sequential "journal"/log mein likhta hai, phir actual jagah — taaki crash ke baad journal replay karke consistent state mein wapas aa sake (poora disk scan nahi karna padta, jaisa purane FAT/ext2 mein hota tha).

**Kyun important hai:** Ye samajhna zaroori hai ki "file save ho gayi" ka matlab OS-level par kya hai — agar sirf `write()` kiya `fsync()` nahi, to power loss mein wo data disk par nahi hoga, chahe application ne "success" bhi return kar diya ho. Ye Level 3 (databases) ka seedha foundation hai — WAL (write-ahead log) ka poora point hi ye hai ki `fsync` ke through durability guarantee di ja sake.

**Example:** Postgres har commit par WAL ko `fsync` karta hai (ya `synchronous_commit` setting ke hisaab se) — ye guarantee deta hai ki commit ke baad power loss ho bhi jaye, wo transaction disk par surakshit hai. Agar koi application `write()` karke bina `fsync` kiye "data saved" bol de (jaisa naive file-based apps karte hain), to server crash hone par recent writes silently gayab ho sakti hain — isko debug karna bahut mushkil hota hai kyunki normal operation mein sab kuch theek dikhta hai.

---

### 4. I/O models: blocking, non-blocking, I/O multiplexing (`select`/`poll`/`epoll`/`kqueue`), async (`io_uring`), signal-driven

**Kya hai:** Ye sab ek hi sawaal ke jawab hain: "jab main data padhne/likhne ka wait kar raha hoon (network socket, disk), to us waqt CPU/thread kya kare?"

| Model | Kya hota hai |
|---|---|
| **Blocking** | Thread `read()` call karta hai aur wahi ruk jata hai jab tak data na aaye. Simple, lekin ek thread = ek connection (10k connections = 10k threads). |
| **Non-blocking** | `read()` turant return karta hai — data hai to milta hai, nahi to ek "abhi kuch nahi hai" error. Application ko baar-baar poll (check) karna padta hai. |
| **I/O multiplexing** (`select`/`poll`/`epoll`/`kqueue`) | Ek single call se OS ko bolo "in 10,000 sockets mein se jo bhi ready ho, mujhe bata do" — thread ek hi jagah wait karta hai sab ke liye, jaise hi koi socket ready ho jaye, OS thread ko jaga deta hai. `epoll` (Linux)/`kqueue` (BSD/macOS) is O(ready sockets) hain — `select`/`poll` O(total sockets), isliye purane hain aur bade scale par slow. |
| **Async (`io_uring`)** | Application ek queue mein "ye operation karo" request submit karta hai aur turant aage badh jata hai; kernel operation complete karke ek doosri queue mein result daal deta hai — bina baar-baar system call kiye, bina thread block kiye. Sabse kam overhead, newest approach (Linux). |
| **Signal-driven** | Kernel process ko ek signal bhejta hai jab I/O ready ho — kam use hota hai (signal handling complex/error-prone hai), historically important. |

**Kyun important hai:** Ye samajhna hi C10K se C10M tak ka poora safar hai — ek event-loop-based server (Nginx, epoll use karke) sirf ek ya chand threads se lakhon idle connections handle kar sakta hai, jabki thread-per-connection model (Apache prefork/Tomcat classic) memory aur context-switch overhead ki wajah se kuch hazaar par hi gir jata hai.

**Example:** Redis single-threaded hai (apne main data operations ke liye) lekin phir bhi lakhon requests/sec handle karta hai — kyunki wo `epoll` event loop use karta hai: jab tak koi socket data ke saath ready nahi hai, Redis kisi bhi socket par block nahi hota, ek single thread hi sabko efficiently juggle karta hai. Nginx isi tarah se ek worker process se hazaaron connections handle karta hai jabki purana Apache (prefork mode) har connection ke liye naya process spawn karta tha.

---

### 5. System calls, user vs kernel space, cost of crossing the boundary

**Kya hai:** CPU do modes mein chalta hai — **user space** (normal applications, restricted — direct hardware access nahi) aur **kernel space** (OS khud, full hardware access). Jab application ko kuch karna ho jo sirf kernel kar sakta hai (file padhna, network bhejna, memory allocate karna), wo ek **system call** karta hai — CPU mode switch karta hai user se kernel mein, kernel operation karta hai, phir wapas user mode mein switch karta hai. Ye switch free nahi hai — CPU registers save karne padte hain, security checks hote hain, cache/TLB effect padta hai — typically ~100ns-1µs (context switch se kam lekin phir bhi ek "far function call" se saikdon guna mehenga).

**Kyun important hai:** Har `read()`, `write()`, `malloc` (jo internally `brk`/`mmap` call kar sakta hai), network send/receive — sab system calls hain. High-performance systems (databases, proxies) explicitly system-call count minimize karne ki koshish karte hain (batching, `io_uring`, zero-copy) kyunki lakhon requests/sec par ye overhead directly throughput cap ban jata hai.

**Example:** `strace` se koi bhi program run karke dekho — ek simple "hello world" bhi kai `write`, `mmap`, `brk` system calls karta hai. Database engines buffered I/O (page cache use karke) aur batched syscalls (ek baar mein bade chunks read/write) is liye prefer karte hain kyunki har chhota syscall alag se overhead add karta hai — 1 million rows ko 1-row-at-a-time syscalls se padhna vs bade batches mein padhna, real-world mein order-of-magnitude farak dalta hai.

---

### Subtopics — practical "aha" points

**Thread-per-request vs event loop vs async runtimes vs goroutines/green threads:** Ye chaaron ek hi spectrum par hain — kitna concurrency kis cost par milta hai. *Thread-per-request* (classic Java servlets) simple code likhna aasan banata hai (synchronous-looking) lekin thread stack memory (~1-8MB default) aur context-switch cost se scale nahi karta. *Event loop* (Node.js, Nginx, Redis) ek single thread par non-blocking I/O se hazaaron connections handle karta hai, lekin ek CPU-heavy operation poori event loop ko block kar deta hai. *Async runtimes* (Rust Tokio, Python asyncio) event loop ka structured version hain — code synchronous jaisa dikhta hai (`await`) lekin actually cooperative scheduling hai. *Goroutines/green threads* (Go, Erlang) runtime khud M:N scheduling karta hai (M goroutines ko N OS threads par map karta hai) — developer ko "thread jaisa" simple model milta hai lekin cost coroutine jaisi kam. *Fayda:* interview mein "iske liye kya use karoge" ka jawab yahi framework hai — blocking-heavy simple CRUD app → thread-per-request theek hai; lakhon concurrent connections (chat, notifications) → event loop/async/goroutines zaroori hain.

**C10K → C10M problem:** 1999 mein "C10K" (10,000 concurrent connections) ek hard scaling wall tha thread-per-connection model ki wajah se. `epoll`/`kqueue` ne ye wall tod diya — ab "C10M" (10 million) tak baat ho rahi hai, jisme bottleneck thread scheduling nahi balki kernel networking stack khud ban jata hai (isi liye kernel-bypass jaisi techniques exist karti hain, neeche dekho). *Fayda:* samajh aata hai ki har "scaling wall" ek specific bottleneck hoti hai jo tools change karke door hoti hai — aur agli wall hamesha kahin aur wait kar rahi hoti hai.

**`ulimit`, file descriptors as a hard scaling limit, ephemeral port exhaustion:** Linux mein har open socket/file ek **file descriptor** leta hai, aur process ke paas ek default limit hoti hai (aksar 1024!) — `ulimit -n` se badhai jati hai. Ephemeral ports (client side se outgoing connections ke liye) ek range se allocate hote hain (~28,000 usable per destination IP:port tuple) — agar tumhara proxy/service ek hi downstream host ko bahut zyada connections banaye (bina connection pooling ke), to ye range khatam ho sakti hai — naye connections banana fail hone lagta hai, symptoms confusing hote hain ("connection refused" bina kisi obvious cause ke). *Fayda:* production outage jaha "bas thoda zyada load aaya aur connections fail hone lage" — bahut baar ye do limits mein se ek hoti hai, log mein "too many open files" ya socket errors dhoondo.

**cgroups & namespaces — containers ka actual mechanism:** Container "lightweight VM" nahi hai — wo ek normal Linux process hai jisko do kernel features se isolate kiya gaya hai: **namespaces** (process ko apna khud ka view deta hai — PID namespace mein wo khud ko PID 1 dikhta hai, network namespace mein apna khud ka network stack, mount namespace mein apna khud ka filesystem view) aur **cgroups** (control groups — kitna CPU/memory/IO ye process (ya group) use kar sakta hai, hard limit). Docker/Kubernetes in dono ko combine karke "container" banate hain. *Fayda:* samajh aata hai ki container VM se itna lightweight (~ms startup) kyun hai — koi hardware emulation nahi, sirf kernel-level isolation hai; aur ye bhi ki container "security boundary" VM jitni strong nahi hai (same kernel share hota hai sabke beech).

**CPU throttling in containers (CFS quota) aur p99 latency:** Kubernetes/Docker CPU limit **CFS (Completely Fair Scheduler) quota** se enforce hota hai — ek fixed time-period (default 100ms) mein process ko sirf itna CPU time milta hai, us se zyada use karne par process ko **throttle** kar diya jata hai (bilkul rok diya jata hai baaki period ke liye), chahe poore machine mein CPU idle hi kyun na pada ho. *Fayda:* ye ek famous "invisible" p99 latency killer hai — CPU usage graph normal (jaise 50%) dikh sakta hai lekin throttling metric (`container_cpu_cfs_throttled_periods`) high ho, matlab requests bursty tarike se CPU maang rahi hain aur beech-beech mein pause ho rahi hain. Fix: CPU limit hata do (sirf request rakho), ya limit ko generously bada rakho.

---

### Advanced concepts

**`fsync` semantics, write barriers, disk cache lies, `O_DIRECT`:** `fsync()` sirf tabhi kaam karta hai jab disk khud honest ho — kuch cheap/consumer SSDs "fsync ho gaya" bol dete hain jabki data abhi bhi disk ke apne internal write cache mein hai (power loss mein wo bhi gayab ho sakta hai) — isi liye enterprise SSDs mein capacitor-backed write cache hoti hai jo ye lie nahi bolti. **Write barriers** ensure karte hain ki writes ek specific order mein hi disk tak pahunchein (jaise journal pehle, data baad mein). **`O_DIRECT`** flag OS ke page cache ko bypass karke seedha disk se read/write karta hai — databases isko use karte hain kyunki unka apna buffer/cache management (buffer pool) hota hai, OS ka double-caching sirf memory waste aur unpredictability add karta hai. *Fayda:* samajh aata hai ki "durability" sirf software guarantee nahi, hardware ki honesty par bhi depend karti hai — aur production databases apna khud ka caching layer kyun banate hain OS ke upar bharosa karne ke bajaye.

**Copy-on-write, `fork` semantics, Redis `BGSAVE`:** Jab Linux `fork()` karta hai (naya child process banata hai), wo turant poora parent memory copy nahi karta — dono processes same physical memory pages **share** karte hain, marked "copy-on-write" (COW). Jis moment koi bhi process (parent ya child) us page ko **modify** karta hai, tabhi actual copy banti hai (sirf us page ki, poori memory ki nahi). *Fayda:* Redis `BGSAVE` isi trick se snapshot leta hai — `fork()` se ek child process banata hai (jo instantly "free" hai, koi bada copy nahi hua), child apna (frozen, consistent) view disk par likhta hai jabki parent normal serving continue karta hai. Lekin agar parent process bahut saari writes kare (high write-heavy workload) is dauran, to bahut saare pages COW ho jayenge — worst case memory usage **double** ho sakta hai (ye ek real production gotcha hai jo Redis capacity planning mein yaad rakhna padta hai).

**Huge pages, transparent huge pages, latency side effects:** Normal page size 4KB hoti hai — bade memory (jaise 32GB heap) ke liye lakhon pages ka matlab hai bada page table aur zyada TLB misses. **Huge pages** (2MB ya 1GB size) is problem ko kam karte hain (kam pages = chhota page table = kam TLB misses = better throughput). **Transparent Huge Pages (THP)** Linux ka feature hai jo automatically chhoti pages ko badi mein merge karne ki koshish karta hai background mein — lekin ye merging (defragmentation) khud ek CPU-intensive operation hai jo random latency spikes create kar sakta hai. *Fayda:* ye ek well-known "database ko THP disable karne ki recommendation" ke peeche ka reason hai (Redis, MongoDB, Postgres docs sab THP disable karne bolte hain) — throughput ka thoda fayda p99 latency ki unpredictability se zyada mehenga pad jata hai un workloads mein.

**Kernel bypass: DPDK, SR-IOV, user-space TCP:** Normal networking mein packet NIC → kernel driver → kernel network stack (TCP/IP) → syscall boundary → application, hote hue aata hai — har hop overhead hai. **Kernel bypass** techniques (DPDK — Data Plane Development Kit) application ko seedha NIC hardware ke saath directly (poll-mode, kernel ko bilkul involve kiye bina) baat karne dete hain — aur agar zaroori ho to apna khud ka user-space TCP stack bhi likha jata hai. **SR-IOV** hardware-level virtualization hai jo ek physical NIC ko multiple virtual NICs mein "split" karta hai bina hypervisor overhead ke, VMs ko near-native network performance dene ke liye. *Fayda:* ye samajhna zaroori hai kyunki 99% systems ko iski zaroorat nahi (complexity bahut zyada hai, normal kernel networking already fast hai) — lekin extreme-low-latency domains (high-frequency trading, kuch CDN/load-balancer edge cases) mein microseconds bhi matter karte hain, wahan ye justified hai.

---

### Hands-on lab

> TCP echo server teen tarike se likho — thread-per-connection, `epoll` event loop, aur async runtime (jaise Rust Tokio / Python asyncio / Go). Teeno ko load test karo taaki "knee" (jahan performance girna shuru hoti hai) mil jaye.

**Ye lab kyun important hai:** Ye subtopics ke "Thread-per-request vs event loop vs async" wale theory ko numbers mein badal deta hai. Tumhe khud dikhega ki thread-per-connection model kis concurrent-connection count par (shayad ~1,000-5,000 ke aas-paas, machine ke hisaab se) memory/context-switch se struggle karna shuru karta hai, jabki `epoll` wala model wahi machine par ussi resource budget mein kai guna zyada connections handle kar leta hai. Ye C10K problem ko apni aankhon se dekhna hai.

- [ ] Mera result: Thread-per-connection knee = ______ connections, `epoll` event loop knee = ______ connections, Async runtime knee = ______ connections

**Apne shabdon mein (haath se likho):**
_________________________________________________
_________________________________________________
_________________________________________________

## 0.5 Concurrency & parallelism
## 0.6 Reliability arithmetic & back-of-envelope estimation
## 0.7 Thinking tools & the design method
## ✅ Exit gate for Level 0
