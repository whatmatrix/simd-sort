namespace qs {

    namespace avx512 {

        __mmask16 FORCE_INLINE get_range_mask(uint32_t position, uint32_t length) {
            
            return (uint32_t(1) << (position + length)) - (uint32_t(1) << position);
        }

        /*
            r stores 1 to 16 elements to partition, mask select leading element
        */
        void partition_register(__m512i& r, const __m512i pivot, __mmask16 mask, int& left, int& right) {

            // example:

            // r     = [  1,   2,   3, 100, 200, 300,   4,   5,   50,  900,  -1,  -2,  -3,  -4,  -5,  -6] 
            // mask  = 0x03ff  -- first 10 elements are subject to parition
            // pivot = packed_dword(50) 
            
            const __mmask16 less_mask       = _mm512_mask_cmplt_epi32_mask(mask, r, pivot);
            const __mmask16 equal_mask      = _mm512_mask_cmpeq_epi32_mask(mask, r, pivot);

            // less    = [1, 2, 3, 4, 5, ... rest are zeros]
            const __m512i less              = _mm512_maskz_compress_epi32(less_mask, r);

            // greater = [100, 200, 300, 900, ... rest are zeros]
            const __m512i greater           = _mm512_maskz_compress_epi32(~(less_mask | equal_mask) & mask, r);

            // less_cnt  = 5
            // equal_cnt = 1
            const int less_cnt              = _mm_popcnt_u32(less_mask);
            const int equal_cnt             = _mm_popcnt_u32(equal_mask);

            const __mmask16 store_less      = get_range_mask(0, less_cnt);
            const __mmask16 store_equal     = get_range_mask(less_cnt, equal_cnt);
            const __mmask16 store_greater   = ~(store_less | store_equal) & mask;

            // merge less with input
            // r = [  1,   2,   3,   4,   5, 300,   4,   5,  50, 900,  -1,  -2,  -3,  -4,  -5,  -6] 
            //        ^    ^    ^    ^    ^
            //        merged
            r = _mm512_mask_mov_epi32   (r, store_less,     less);

            // r = [  1,   2,   3,   4,   5, 300, 100, 200, 300, 900,  -1,  -2,  -3,  -4,  -5,  -6] 
            //                                    ^^^  ^^^  ^^^  ^^^
            //                                    merged
            r = _mm512_mask_expand_epi32(r, store_greater,  greater);

            // r = [  1,   2,   3,   4,   5,  50, 100, 200, 300, 900,  -1,  -2,  -3,  -4,  -5,  -6] 
            //                                ^^
            //                              merged
            r = _mm512_mask_mov_epi32   (r, store_equal,    pivot);

            right = left + less_cnt;
            left  = left + less_cnt + equal_cnt;
        }

    } // namespace avx512

} // namespace qs
