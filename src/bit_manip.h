#ifndef BIT_MANIP_H
#define BIT_MANIP_H

    //#define x86 1

    #if defined(__x86_64__)
        #include <cstdint>
        #include <immintrin.h>

        // pext equivalent 
        inline uint64_t extract_bits(uint64_t x, uint64_t mask) {
            return _pext_u64(x, mask);
        }

        inline constexpr uint64_t maskForPos(const int x) {
            return (uint64_t(1) << (x));
        }

        inline int tz_count(const uint64_t& x) {
            return _tzcnt_u64(x);
        }

        inline void pop_lsb(uint64_t& x) {
            x = _blsr_u64(x);
            //x &= (x - 1);
        }

    #else
        #include <bit>
        #include <cstdint>

        // pext equivalent 
        constexpr uint64_t extract_bits(uint64_t x, uint64_t mask) {
            uint64_t res = 0;
            for (uint64_t bb = 1; mask != 0; bb += bb) {
                if (x & mask & -mask) {
                res |= bb;
                }
                mask &= (mask - 1);
            }
            return res;
        }

        inline uint64_t maskForPos(int x) {
            return (uint64_t(1) << (x));
        }

        inline int tz_count(uint64_t x) {
            return std::countr_zero(x);
        }

        // disable rightmost set bit
        // https://stackoverflow.com/questions/64605039/how-does-the-formula-x-x-1-works
        inline void pop_lsb(uint64_t& x) {
            x &= (x - 1);
        }

    #endif

#endif
