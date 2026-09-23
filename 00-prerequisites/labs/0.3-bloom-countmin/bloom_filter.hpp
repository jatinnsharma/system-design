#pragma once

#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <functional>

// Bloom filter: probabilistic set membership.
// False positives possible ("maybe present"), false negatives impossible ("definitely absent").
// Sizing formulas (n = expected inserts, p = target false-positive rate):
//   m (bits)        = -n * ln(p) / (ln2)^2
//   k (hash count)  = (m / n) * ln2
class BloomFilter {
public:
    BloomFilter(size_t expected_inserts, double target_fp_rate)
        : num_bits_(optimal_num_bits(expected_inserts, target_fp_rate)),
          num_hashes_(optimal_num_hashes(num_bits_, expected_inserts)),
          bits_(num_bits_, false) {}

    void add(const std::string& key) {
        for (size_t i = 0; i < num_hashes_; ++i) {
            bits_[hash_i(key, i) % num_bits_] = true;
        }
    }

    // true => "maybe present" (could be a false positive)
    // false => "definitely absent"
    bool possibly_contains(const std::string& key) const {
        for (size_t i = 0; i < num_hashes_; ++i) {
            if (!bits_[hash_i(key, i) % num_bits_]) return false;
        }
        return true;
    }

    size_t num_bits() const { return num_bits_; }
    size_t num_hashes() const { return num_hashes_; }

    static size_t optimal_num_bits(size_t n, double p) {
        double m = -static_cast<double>(n) * std::log(p) / (std::log(2.0) * std::log(2.0));
        return static_cast<size_t>(std::ceil(m));
    }

    static size_t optimal_num_hashes(size_t m, size_t n) {
        if (n == 0) return 1;
        double k = (static_cast<double>(m) / n) * std::log(2.0);
        return std::max<size_t>(1, static_cast<size_t>(std::round(k)));
    }

private:
    // Double hashing (Kirsch-Mitzenmacher): derive k hash functions from two independent hashes,
    // avoids running k separate hash algorithms.
    uint64_t hash_i(const std::string& key, size_t i) const {
        uint64_t h1 = std::hash<std::string>{}(key);
        uint64_t h2 = fnv1a(key);
        return h1 + i * h2;
    }

    static uint64_t fnv1a(const std::string& s) {
        uint64_t h = 1469598103934665603ULL;
        for (unsigned char c : s) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        return h;
    }

    size_t num_bits_;
    size_t num_hashes_;
    std::vector<bool> bits_;
};
