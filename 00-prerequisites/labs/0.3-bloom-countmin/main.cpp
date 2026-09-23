#include <iostream>
#include <iomanip>
#include <random>
#include <string>
#include <unordered_set>
#include <unordered_map>

#include "bloom_filter.hpp"
#include "count_min_sketch.hpp"

static std::string make_key(const std::string& prefix, size_t i) {
    return prefix + std::to_string(i);
}

static void run_bloom_filter_test() {
    const size_t n = 100000;          // items actually inserted
    const double target_fp = 0.01;    // 1% target false-positive rate
    const size_t num_probe = 100000;  // items NOT inserted, probed for false positives

    BloomFilter filter(n, target_fp);

    std::cout << "=== Bloom Filter ===\n";
    std::cout << "n = " << n << ", target p = " << target_fp << "\n";
    std::cout << "m (bits) = " << filter.num_bits()
              << " (" << filter.num_bits() / 8 / 1024.0 << " KB), k (hashes) = "
              << filter.num_hashes() << "\n";

    for (size_t i = 0; i < n; ++i) {
        filter.add(make_key("inserted-", i));
    }

    // Sanity check: every inserted key must report present (no false negatives allowed).
    size_t missed = 0;
    for (size_t i = 0; i < n; ++i) {
        if (!filter.possibly_contains(make_key("inserted-", i))) ++missed;
    }
    std::cout << "false negatives (must be 0): " << missed << "\n";

    // Probe keys that were never inserted; count how many the filter wrongly claims are present.
    size_t false_positives = 0;
    for (size_t i = 0; i < num_probe; ++i) {
        if (filter.possibly_contains(make_key("absent-", i))) ++false_positives;
    }

    double measured_fp = static_cast<double>(false_positives) / num_probe;
    std::cout << "measured false-positive rate: " << std::fixed << std::setprecision(5)
              << measured_fp << "  (formula predicted: " << target_fp << ")\n\n";
}

static void run_count_min_sketch_test() {
    const double eps = 0.001;   // error factor
    const double delta = 0.01;  // failure probability
    const size_t num_distinct = 5000;
    const size_t total_events = 2000000;

    CountMinSketch cms(eps, delta);
    std::unordered_map<std::string, uint32_t> ground_truth;

    std::cout << "=== Count-Min Sketch ===\n";
    std::cout << "eps = " << eps << ", delta = " << delta
              << ", width = " << cms.width() << ", depth = " << cms.depth()
              << " (" << (cms.width() * cms.depth() * sizeof(uint32_t)) / 1024.0 << " KB)\n";

    // Zipf-like skew: a few keys get most of the traffic (heavy hitters), like real hot-key traffic.
    std::mt19937 rng(42);
    std::vector<double> weights(num_distinct);
    for (size_t i = 0; i < num_distinct; ++i) weights[i] = 1.0 / (i + 1);
    std::discrete_distribution<size_t> dist(weights.begin(), weights.end());

    for (size_t e = 0; e < total_events; ++e) {
        std::string key = make_key("item-", dist(rng));
        cms.increment(key);
        ground_truth[key]++;
    }

    // Error bound: estimate should not exceed true_count + eps * total_events (with prob >= 1 - delta).
    double error_bound = eps * total_events;
    double max_observed_error = 0;
    size_t violations = 0;

    for (const auto& [key, true_count] : ground_truth) {
        uint32_t est = cms.estimate(key);
        double error = static_cast<double>(est) - true_count;
        max_observed_error = std::max(max_observed_error, error);
        if (error > error_bound) ++violations;
        if (error < 0) {
            std::cout << "BUG: underestimate found for " << key << "\n"; // should never happen
        }
    }

    std::cout << "theoretical max error per key (eps * total_events): " << error_bound << "\n";
    std::cout << "max observed error: " << max_observed_error << "\n";
    std::cout << "keys exceeding error bound (should be ~0, allowed by delta): " << violations
              << " / " << ground_truth.size() << "\n";

    // Show the top-5 heaviest keys: sketch estimate vs ground truth.
    std::vector<std::pair<std::string, uint32_t>> sorted_truth(ground_truth.begin(), ground_truth.end());
    std::sort(sorted_truth.begin(), sorted_truth.end(),
              [](auto& a, auto& b) { return a.second > b.second; });

    std::cout << "\ntop 5 heavy hitters (true vs estimated):\n";
    for (size_t i = 0; i < 5 && i < sorted_truth.size(); ++i) {
        std::cout << "  " << sorted_truth[i].first << ": true=" << sorted_truth[i].second
                  << " estimated=" << cms.estimate(sorted_truth[i].first) << "\n";
    }
}

int main() {
    run_bloom_filter_test();
    run_count_min_sketch_test();
    return 0;
}
