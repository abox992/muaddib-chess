#ifndef BIT_ARRAY_H
#define BIT_ARRAY_H

#include <cassert>
#include <cstddef>
#include <cstdint>

// N bools in one byte. 
// bool b = flags[i] copies the bit. auto b = flags[i] names the proxy.
template <std::size_t N>
class BitArray {
    static_assert(N >= 1 && N <= 8);

    uint8_t bits_{};

    class Reference {
        uint8_t& bits_;
        uint8_t  index_;

    public:
        Reference(uint8_t& bits, std::size_t index) noexcept
            : bits_(bits), index_(static_cast<uint8_t>(index)) {}

        operator bool() const noexcept { return ((bits_ >> index_) & 1u) != 0; }

        Reference& operator=(bool value) noexcept {
            const uint8_t mask = static_cast<uint8_t>(1u << index_);
            bits_ = static_cast<uint8_t>((bits_ & static_cast<uint8_t>(~mask)) |
                                         (static_cast<uint8_t>(value) << index_));
            return *this;
        }

        // a[i] = a[j] must copy the bit, not the proxy's address and index.
        Reference& operator=(const Reference& other) noexcept { return *this = static_cast<bool>(other); }
    };

public:
    constexpr BitArray() noexcept = default;

    [[nodiscard]] constexpr bool operator[](std::size_t index) const noexcept {
        assert(index < N);
        return ((bits_ >> index) & 1u) != 0;
    }

    [[nodiscard]] Reference operator[](std::size_t index) noexcept {
        assert(index < N);
        return Reference(bits_, index);
    }

    [[nodiscard]] constexpr uint8_t raw() const noexcept { return bits_; }

    constexpr void setRaw(uint8_t bits) noexcept {
        assert((bits >> N) == 0);
        bits_ = bits;
    }
};

static_assert(sizeof(BitArray<4>) == 1);

#endif
