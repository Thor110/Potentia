// Sieve — rows of exact numbers, stored packed.
//
// A counting table is rows of numbers: the completions from each state with r symbols left. Held
// as a BigUint each, every number is a vector and an allocation of its own (about 40 bytes before
// its value, and that much for a zero): the 236,034-state table of every text plugin at 32
// characters held 358 MB for 68 MB of numbers. Here each row is one block of limbs, every number's
// one after another, and where each starts (4 bytes): 99 MB for the same table. A row is worked
// out number by number into one BigUint whose memory is reused, then stored at exactly its size.
// Read-only once a row is added, so threads may read it together.
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

namespace sieve {

class PackedRows
{
public:
    void reserve(size_t rows) { rows_.reserve(rows); }
    size_t rows() const { return rows_.size(); }

    // Appends a row of `count` numbers: value(i, v) works out number i into v (zero when called;
    // add_mul_small, +=, and the like). Rows already added may be read from inside it.
    template <class Value>
    void add_row(size_t count, Value&& value)
    {
        Row row;
        row.start.reserve(count + 1);
        scratch_.clear();
        for (size_t i = 0; i < count; ++i)
        {
            acc_.set_zero();
            value(i, acc_);
            const auto limbs = acc_.limbs();
            if (scratch_.size() + limbs.size() > UINT32_MAX) throw std::length_error("a row of numbers too large to store");
            row.start.push_back(uint32_t(scratch_.size()));
            scratch_.insert(scratch_.end(), limbs.begin(), limbs.end());
        }
        row.start.push_back(uint32_t(scratch_.size()));
        row.limbs.assign(scratch_.begin(), scratch_.end()); // exactly its size
        rows_.push_back(std::move(row));
    }

    // Frees what building used (the scratch row), once the last row is added.
    void finish()
    {
        scratch_ = {};
        acc_ = BigUint();
    }

    // Number i of row r: its limbs (none for zero), as a BigUint, or whether it is zero.
    std::span<const uint64_t> limbs(size_t r, size_t i) const
    {
        const Row& row = rows_[r];
        return {row.limbs.data() + row.start[i], row.start[i + 1] - row.start[i]};
    }
    BigUint value(size_t r, size_t i) const { return BigUint::from_limbs64(limbs(r, i)); }
    bool is_zero(size_t r, size_t i) const { return rows_[r].start[i] == rows_[r].start[i + 1]; }

    // The memory held, in bytes (the rows' blocks and offsets).
    double bytes() const
    {
        double b = 0;
        for (const Row& row : rows_) b += double(row.limbs.capacity()) * sizeof(uint64_t) + double(row.start.capacity()) * sizeof(uint32_t);
        return b;
    }
    // What `rows` rows of `count` numbers of up to `bits` bits take: each number where it starts,
    // and its limbs at half the longest (counts grow along a table), rounded up a limb. An
    // overestimate, for deciding before building whether a table fits the filter memory.
    static double estimate(double rows, double count, double bits)
    {
        return rows * count * (sizeof(uint32_t) + (bits / 2.0 / 64.0 + 1.0) * sizeof(uint64_t));
    }

private:
    struct Row
    {
        std::vector<uint64_t> limbs;  // every number's limbs, one after another
        std::vector<uint32_t> start;  // number i's limbs are [start[i], start[i + 1])
    };
    std::vector<Row> rows_;
    std::vector<uint64_t> scratch_; // the row being worked out (its memory reused)
    BigUint acc_;                   // the number being worked out (its memory reused)
};

} // namespace sieve
