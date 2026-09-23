#pragma once

#include <cstdint>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>

// Count-Min Sketch: approximate frequency counting in sublinear space.
// Estimates are always >= true count (collisions only ever add extra, never subtract),
// so it's biased upward — take the min across rows to reduce that bias.
// Sizing (eps = error factor, delta = failure probability):
//   width (w) = ceil(e / eps)
//   depth (d) = ceil(ln(1 / delta))
class CountMinSketch {
public:
    CountMinSketch(double eps, double delta)
        : width_(static_cast<size_t>(std::ceil(std::exp(1.0) / eps))),
          depth_(static_cast<size_t>(std::ceil(std::log(1.0 / delta)))),
          table_(depth_, std::vector<uint32_t>(width_, 0)) {}

    void increment(const std::string& key, uint32_t count = 1) {
        for (size_t row = 0; row < depth_; ++row) {
            table_[row][hash_row(key, row) % width_] += count;
        }
    }

    // Always an overestimate or exact; never an underestimate.
    uint32_t estimate(const std::string& key) const {
        uint32_t result = std::numeric_limits<uint32_t>::max();
        for (size_t row = 0; row < depth_; ++row) {
            result = std::min(result, table_[row][hash_row(key, row) % width_]);
        }
        return result;
    }

    size_t width() const { return width_; }
    size_t depth() const { return depth_; }

private:
    uint64_t hash_row(const std::string& key, size_t row) const {
        // Seed the FNV hash per row so each row behaves like an independent hash function.
        uint64_t h = 1469598103934665603ULL ^ (row * 0x9E3779B97F4A7C15ULL);
        for (unsigned char c : key) {
            h ^= c;
            h *= 1099511628211ULL;
        }
        return h;
    }

    size_t width_;
    size_t depth_;
    std::vector<std::vector<uint32_t>> table_;
};
