// float->sRGB8 conversions - two variants.
// by Fabian "ryg" Giesen
//
// I hereby place this code in the public domain.
//
// Both variants come with absolute error bounds and a reversibility and monotonicity
// guarantee (see test driver code below). They should pass D3D10 conformance testing
// (not that you can verify this, but still). They are verified against a clean reference
// implementation provided below, and the test driver checks all floats exhaustively.
//
// Variant 1 uses a smaller table (256 bytes) but a bit more code; variant 2 uses
// a 416-byte table and has simpler dataflow, so in terms of raw cycle count it should
// be faster, at the cost of a few more cache lines polluting L1. Both come in scalar
// and SSE2 variants. There's no total SSE2-isms there, so it should be reasonably easy
// to port to a different architecture. The biggest single part that would have to be
// replaced is the (admittedly weird) usage of PMADDWD (_mm_madd_epi16). The scale/bias
// computation can be done in other ways, but this one happened to map quite nicely to
// my requirements, so I used it.
//
// Generators for the tables are also included, for the curious. (Nothing up my sleeve!)

#include <stdio.h>
#include <math.h>
#include <stdint.h>

#if defined(__ARM_NEON)
#include <arm_neon.h>
#endif
#include "UInt8SRGBTable.hpp"

#define IT_USE_SSE2 0

#if IT_USE_SSE2
#include <emmintrin.h>
#endif


typedef uint32_t uint;
typedef uint8_t uint8;

union FP32 {
    uint u;
    float f;
    struct {
        uint Mantissa : 23;
        uint Exponent : 8;
        uint Sign : 1;
    };
};

// Returns "exact" float value. Round to nearest integer for conversion.
// Done this way so that conversion error can be estimated properly.
static float float_to_srgb8_ref(float f) {
    float s;

    // also covers NaNs
    if (!(f > 0.0f)) {
        s = 0.0f;
    }
    else if (f <= 0.0031308f) {
        s = 12.92f * f;
    }
    else if (f < 1.0f) {
        s = 1.055f * pow(f, 1.0f / 2.4f) - 0.055f;
    }
    else {
        s = 1.0f;
    }

    return s * 255.0f;
}

uint8_t float_to_srgb8_ref_int(float f) {
    return (uint8) (float_to_srgb8_ref(f) + 0.5f); // round, then float->int
}

float srgb8_to_float(uint8_t val) {
#if 1
    return uint8_srgb_to_float32_linear[val];
#else
    // There's just 256 different input valus - just use a table.
    static float table[256];
    static bool init;
    if (!init)
    {
        init = true;
        for (int i=0; i < 256; i++) {
            // Matches the conversion mandated by D3D10 spec
            float c = (float) i * (1.0f / 255.0f);
            if (c <= 0.04045f) {
                table[i] = c / 12.92f;
            }
            else {
                table[i] = pow((c + 0.055f) / 1.055f, 2.4f);
            }
        }
    }

    return table[val];
#endif
}


// This is the version that tries to use a small table (4 cache lines at 64 bytes/line)
// at the expense of a few extra instructions. Use "var2" below for a version with
// less instructions that uses a somewhat larger table.
//
// Float is semi-logarithmic.
// Linear x->sRGB for x >= 0.0031308 is (mostly) a power function which we
// approximate with a bunch of linear segments based on exponent and 3 highest
// bits of mantissa (2 was too inaccurate).
//
// Which exponents do we care about?
// Exponent >= 0: value was >=1, so we return 255 (in fact, we min with 1.0f-eps, so this never happens anyway).
// Exponent < -9: x < 1/512 which is well into the linear part of the sRGB mapping function.
// So the interesting exponent range is [-9,-1].
//
// To get a pow2-sized range, we cheat a bit and only store anchors for linear segments in
// the exponent range [-8,-1], using linear sRGB part of the formula until 1/256.
// This means that we treat a small part of the nonlinear range (namely, the interval
// [0.0031308,0.00390625]) as linear. Our linear scale value needs to be adjusted for this.
// This is done simply by starting from the "correct" scale value (255*12.92, 0x454de99a)
// and doing a binary search for the value that gives the best results (=lowest max error
// in this case) across the range we care about.
//
// The table itself has a bias in the top 16 bits and a scale factor for the linear function
// (based on the next 8 mantissa bits after the 3 we already used). Both are scaled to make
// good use of the available bits. The format was chosen this way so the linear function
// can be evaluated with a single PMADDWD after the mantissa bits were extracted - okay, we do
// need to insert one more set bit in the high half for the bias part to work.
// These coefficients were determined simply by doing a least-squares fit of a linear function
// f(x) = a+b*x inside each "bucket" (see table-making functions below).
//
// Max error for whole function (integer-rounded result minus "exact" value, as computed in
// floats using the official formula): 0.573277 at 0x3b7a88c6
static const uint fp32_to_srgb8_tab3[64] = {
    0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
    0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
    0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
    0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
    0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
    0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
    0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
    0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
};

uint8_t float_to_srgb8(float in) {
    static const FP32 almostone = { 0x3f7fffff }; // 1-eps
    static const FP32 lutthresh = { 0x3b800000 }; // 2^(-8)
    static const FP32 linearsc  = { 0x454c5d00 };
    static const FP32 float2int = { (127 + 23) << 23 };
    FP32 f;

    // Clamp to [0, 1-eps]; these two values map to 0 and 1, respectively.
    // The tests are carefully written so that NaNs map to 0, same as in the reference
    // implementation.
    if (!(in > 0.0f)) { // written this way to catch NaNs
        in = 0.0f;
    }
    if (in > almostone.f) {
        in = almostone.f;
    }

    // Check which region this value falls into
    f.f = in;
    // linear region
    if (f.f < lutthresh.f) {
        f.f *= linearsc.f;
        f.f += float2int.f; // use "magic value" to get float->int with rounding.
        return (uint8) (f.u & 255);
    }
    // non-linear region
    else {
        // Unpack bias, scale from table
        uint tab = fp32_to_srgb8_tab3[(f.u >> 20) & 63];
        uint bias = (tab >> 16) << 9;
        uint scale = tab & 0xffff;

        // Grab next-highest mantissa bits and perform linear interpolation
        uint t = (f.u >> 12) & 0xff;
        return (uint8) ((bias + scale*t) >> 16);
    }
}


#if IT_USE_SSE2
static __m128i float_to_srgb8_SSE2(__m128 f) {
#define SSE_CONST4(name, val) static const __declspec(align(16)) uint name[4] = { (val), (val), (val), (val) }
#define CONST(name) *(const __m128i *)&name
#define CONSTF(name) *(const __m128 *)&name

    SSE_CONST4(c_almostone, 0x3f7fffff);
    SSE_CONST4(c_lutthresh, 0x3b800000);
    SSE_CONST4(c_tabmask,   63);
    SSE_CONST4(c_linearsc,  0x454c5d00);
    SSE_CONST4(c_mantmask,  0xff);
    SSE_CONST4(c_topscale,  0x02000000);

    __m128i temp; // temp value (on stack)

    // Initial clamp
    __m128  zero        = _mm_setzero_ps();
    __m128  clamp1      = _mm_max_ps(f, zero); // limit to [0,1-eps] - also nukes NaNs
    __m128  clamp2      = _mm_min_ps(clamp1, CONSTF(c_almostone));

    // Table index
    __m128i tabidx1     = _mm_srli_epi32(_mm_castps_si128(clamp2), 20);
    __m128i tabidx2     = _mm_and_si128(tabidx1, CONST(c_tabmask));
    _mm_store_si128(&temp, tabidx2);

    // Table lookup
    temp.m128i_u32[0] = fp32_to_srgb8_tab3[temp.m128i_u32[0]];
    temp.m128i_u32[1] = fp32_to_srgb8_tab3[temp.m128i_u32[1]];
    temp.m128i_u32[2] = fp32_to_srgb8_tab3[temp.m128i_u32[2]];
    temp.m128i_u32[3] = fp32_to_srgb8_tab3[temp.m128i_u32[3]];

    // Linear part of ramp
    __m128  linear1     = _mm_mul_ps(clamp2, CONSTF(c_linearsc));
    __m128i linear2     = _mm_cvtps_epi32(linear1);

    // Table finisher
    __m128i tabval      = _mm_load_si128(&temp);
    __m128i tabmult1    = _mm_srli_epi32(_mm_castps_si128(clamp2), 12);
    __m128i tabmult2    = _mm_and_si128(tabmult1, CONST(c_mantmask));
    __m128i tabmult3    = _mm_or_si128(tabmult2, CONST(c_topscale));
    __m128i tabprod     = _mm_madd_epi16(tabval, tabmult3);
    __m128i tabshifted  = _mm_srli_epi32(tabprod, 16);

    // Combine linear+table
    __m128  b_uselin    = _mm_cmplt_ps(clamp2, CONSTF(c_lutthresh)); // use linear results
    __m128i merge1      = _mm_and_si128(linear2, _mm_castps_si128(b_uselin));
    __m128i merge2      = _mm_andnot_si128(_mm_castps_si128(b_uselin), tabshifted);
    __m128i result      = _mm_or_si128(merge1, merge2);

    return result;

#undef SSE_CONST4
#undef CONST
#undef CONSTF
}
#endif


// This version uses a larger table than the code above (104 entries at 4 bytes each,
// or 6.5 cache lines at 64b/line) but is conceptually simpler and needs less instructions.
//
// The basic ideas are still the same, only this time, we squeeze everything into the
// table, even the linear part of the range; since we are approximating the function as
// piecewise linear anyway, this is fairly easy.
//
// In the exact version of the conversion, any value that produces an output float less
// than 0.5 will be rounded to an integer of zero. Inverting the linear part of the transform,
// we get:
//
//   log2(0.5 / (255 * 12.92)) =~ -12.686
//
// which in turn means that any value smaller than about 2^(-12.687) will return 0.
// What this means is that we can adapt the clamping code to just clamp to
// [2^(-13), 1-eps] and we're covered. This means our table needs to cover a range of
// 13 different exponents from -13 to -1.
//
// The table lookup, storage and interpolation works exactly the same way as in the code
// above.
//
// Max error for the whole function (integer-rounded result minus "exact" value, as computed in
// floats using the official formula): 0.544403 at 0x3e9f8000
static const uint fp32_to_srgb8_tab4[104] = {
    0x0073000d, 0x007a000d, 0x0080000d, 0x0087000d, 0x008d000d, 0x0094000d, 0x009a000d, 0x00a1000d,
    0x00a7001a, 0x00b4001a, 0x00c1001a, 0x00ce001a, 0x00da001a, 0x00e7001a, 0x00f4001a, 0x0101001a,
    0x010e0033, 0x01280033, 0x01410033, 0x015b0033, 0x01750033, 0x018f0033, 0x01a80033, 0x01c20033,
    0x01dc0067, 0x020f0067, 0x02430067, 0x02760067, 0x02aa0067, 0x02dd0067, 0x03110067, 0x03440067,
    0x037800ce, 0x03df00ce, 0x044600ce, 0x04ad00ce, 0x051400ce, 0x057b00c5, 0x05dd00bc, 0x063b00b5,
    0x06970158, 0x07420142, 0x07e30130, 0x087b0120, 0x090b0112, 0x09940106, 0x0a1700fc, 0x0a9500f2,
    0x0b0f01cb, 0x0bf401ae, 0x0ccb0195, 0x0d950180, 0x0e56016e, 0x0f0d015e, 0x0fbc0150, 0x10630143,
    0x11070264, 0x1238023e, 0x1357021d, 0x14660201, 0x156601e9, 0x165a01d3, 0x174401c0, 0x182401af,
    0x18fe0331, 0x1a9602fe, 0x1c1502d2, 0x1d7e02ad, 0x1ed4028d, 0x201a0270, 0x21520256, 0x227d0240,
    0x239f0443, 0x25c003fe, 0x27bf03c4, 0x29a10392, 0x2b6a0367, 0x2d1d0341, 0x2ebe031f, 0x304d0300,
    0x31d105b0, 0x34a80555, 0x37520507, 0x39d504c5, 0x3c37048b, 0x3e7c0458, 0x40a8042a, 0x42bd0401,
    0x44c20798, 0x488e071e, 0x4c1c06b6, 0x4f76065d, 0x52a50610, 0x55ac05cc, 0x5892058f, 0x5b590559,
    0x5e0c0a23, 0x631c0980, 0x67db08f6, 0x6c55087f, 0x70940818, 0x74a007bd, 0x787d076c, 0x7c330723,
};

uint8 float_to_srgb8_var2(float in) {
    static const FP32 almostone = { 0x3f7fffff }; // 1-eps
    static const FP32 minval    = { (127-13) << 23 };
    FP32 f;

    // Clamp to [2^(-13), 1-eps]; these two values map to 0 and 1, respectively.
    // The tests are carefully written so that NaNs map to 0, same as in the reference
    // implementation.
    if (!(in > minval.f)) { // written this way to catch NaNs
        in = minval.f;
    }
    if (in > almostone.f) {
        in = almostone.f;
    }

    // Do the table lookup and unpack bias, scale
    f.f = in;
    uint tab = fp32_to_srgb8_tab4[(f.u - minval.u) >> 20];
    uint bias = (tab >> 16) << 9;
    uint scale = tab & 0xffff;

    // Grab next-highest mantissa bits and perform linear interpolation
    uint t = (f.u >> 12) & 0xff;
    return (uint8) ((bias + scale*t) >> 16);
}


#if IT_USE_SSE2
static __m128i float_to_srgb8_var2_SSE2(__m128 f) {
#define SSE_CONST4(name, val) static const __declspec(align(16)) uint name[4] = { (val), (val), (val), (val) }
#define CONST(name) *(const __m128i *)&name
#define CONSTF(name) *(const __m128 *)&name

    SSE_CONST4(c_clampmin,  (127 - 13) << 23);
    SSE_CONST4(c_almostone, 0x3f7fffff);
    SSE_CONST4(c_lutthresh, 0x3b800000);
    SSE_CONST4(c_mantmask,  0xff);
    SSE_CONST4(c_topscale,  0x02000000);

    __m128i temp; // temp value (on stack)

    // Initial clamp
    __m128  clamp1      = _mm_max_ps(f, CONSTF(c_clampmin)); // limit to [clampmin,1-eps] - also nuke NaNs
    __m128  clamp2      = _mm_min_ps(clamp1, CONSTF(c_almostone));

    // Table index
    __m128i tabidx      = _mm_srli_epi32(_mm_castps_si128(clamp2), 20);
    _mm_store_si128(&temp, tabidx);

    // Table lookup
    temp.m128i_u32[0] = fp32_to_srgb8_tab4[temp.m128i_i32[0] - (127-13)*8];
    temp.m128i_u32[1] = fp32_to_srgb8_tab4[temp.m128i_i32[1] - (127-13)*8];
    temp.m128i_u32[2] = fp32_to_srgb8_tab4[temp.m128i_i32[2] - (127-13)*8];
    temp.m128i_u32[3] = fp32_to_srgb8_tab4[temp.m128i_i32[3] - (127-13)*8];

    // Finisher
    __m128i tabval      = _mm_load_si128(&temp);
    __m128i tabmult1    = _mm_srli_epi32(_mm_castps_si128(clamp2), 12);
    __m128i tabmult2    = _mm_and_si128(tabmult1, CONST(c_mantmask));
    __m128i tabmult3    = _mm_or_si128(tabmult2, CONST(c_topscale));
    __m128i tabprod     = _mm_madd_epi16(tabval, tabmult3);
    __m128i result      = _mm_srli_epi32(tabprod, 16);

    return result;

#undef SSE_CONST4
#undef CONST
#undef CONSTF
}
#endif


#if defined(__ARM_NEON)
// Like the SIMD version, returns four 32-bit lanes containing values in [0, 255].
static uint32x4_t float32x4_to_srgb8_var2_NEON(float32x4_t f) {
    const float32x4_t clampmin = vreinterpretq_f32_u32(vdupq_n_u32((127 - 13) << 23));
    const float32x4_t almostone = vreinterpretq_f32_u32(vdupq_n_u32(0x3f7fffff));
    
    // Compare/select preserves SSE MAXPS behavior: NaNs select the lower bound.
    // Plain NEON vmaxq_f32 would propagate NaNs instead.
    const float32x4_t clamp1 = vbslq_f32(vcgtq_f32(f, clampmin), f, clampmin);
    const float32x4_t clamp2 = vminq_f32(clamp1, almostone);
    const uint32x4_t bits = vreinterpretq_u32_f32(clamp2);
    const uint32x4_t indices = vsubq_u32(vshrq_n_u32(bits, 20), vdupq_n_u32((127 - 13) * 8));

    // NEON has no general gather load, so fetch each table entry separately.
    const uint32_t entries[4] = {
        fp32_to_srgb8_tab4[vgetq_lane_u32(indices, 0)],
        fp32_to_srgb8_tab4[vgetq_lane_u32(indices, 1)],
        fp32_to_srgb8_tab4[vgetq_lane_u32(indices, 2)],
        fp32_to_srgb8_tab4[vgetq_lane_u32(indices, 3)]
    };
    const uint32x4_t tabval = vld1q_u32(entries);

    // Equivalent to PMADDWD: scale * t + bias * 512. Both table halfwords
    // are below 0x8000, so unsigned 32-bit arithmetic gives the same result.
    const uint32x4_t bias = vshlq_n_u32(vshrq_n_u32(tabval, 16), 9);
    const uint32x4_t scale = vandq_u32(tabval, vdupq_n_u32(0xffff));
    const uint32x4_t t = vandq_u32(vshrq_n_u32(bits, 12), vdupq_n_u32(0xff));
    return vshrq_n_u32(vmlaq_u32(bias, scale, t), 16);
}


// Convert directly from binary16 bits, without floating-point arithmetic.
// The low four bytes contain sRGB results; the upper four bytes are zero.
static inline uint8x8_t float16x4_to_srgb8_var2_NEON(const float16x4_t& f) {
    const uint16x4_t bits = vreinterpret_u16_f16(f);
    const uint16x4_t minval = vdup_n_u16(0x0800); // 2^-13 in binary16.

    // Unsigned encodings above +infinity include positive NaNs and all
    // negative values (including negative NaNs). Map these to the lower bound.
    const uint16x4_t valid = vcle_u16(bits, vdup_n_u16(0x7c00));
    const uint16x4_t positive = vbsl_u16(valid, bits, minval);
    const uint16x4_t saturated = vcge_u16(positive, vdup_n_u16(0x3c00));
    const uint16x4_t clamped = vmin_u16(vmax_u16(positive, minval), vdup_n_u16(0x3bff));

    // Five exponent bits plus the top three mantissa bits select the same
    // table bucket as binary32. The remaining seven mantissa bits give t/2.
    const uint16x4_t indices = vsub_u16(vshr_n_u16(clamped, 7), vdup_n_u16((15 - 13) * 8));
    const uint32_t entries[4] = {
        fp32_to_srgb8_tab4[vget_lane_u16(indices, 0)],
        fp32_to_srgb8_tab4[vget_lane_u16(indices, 1)],
        fp32_to_srgb8_tab4[vget_lane_u16(indices, 2)],
        fp32_to_srgb8_tab4[vget_lane_u16(indices, 3)]
    };
    const uint32x4_t tabval = vld1q_u32(entries);
    const uint16x4_t mantissa = vshl_n_u16(vand_u16(clamped, vdup_n_u16(0x7f)), 1);

    // Inputs >= 1 use binary32's 1-epsilon endpoint (t = 255), rather than
    // binary16's largest value below 1 (t = 254).
    const uint16x4_t t = vbsl_u16(saturated, vdup_n_u16(255), mantissa);
    const uint32x4_t bias = vshlq_n_u32(vshrq_n_u32(tabval, 16), 9);
    const uint32x4_t product = vmlal_u16(bias, vmovn_u32(tabval), t);
    const uint16x4_t values = vshrn_n_u32(product, 16);
    return vmovn_u16(vcombine_u16(values, vdup_n_u16(0)));
}


// Eight-lane binary16 conversion; all preprocessing stays in one 128-bit vector.
static inline uint8x8_t float16x8_to_srgb8_var2_NEON(const float16x8_t& f) {
    const uint16x8_t bits = vreinterpretq_u16_f16(f);
    const uint16x8_t minval = vdupq_n_u16(0x0800);

    // Map NaNs and negative values to zero, and saturate positive inputs >= 1.
    const uint16x8_t valid = vcleq_u16(bits, vdupq_n_u16(0x7c00));
    const uint16x8_t positive = vbslq_u16(valid, bits, minval);
    const uint16x8_t saturated = vcgeq_u16(positive, vdupq_n_u16(0x3c00));
    const uint16x8_t clamped = vminq_u16(vmaxq_u16(positive, minval), vdupq_n_u16(0x3bff));
    const uint16x8_t indices = vsubq_u16(vshrq_n_u16(clamped, 7), vdupq_n_u16((15 - 13) * 8));

    // NEON has no general gather load; each lane selects one packed bias/scale.
    const uint32_t entries[8] = {
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 0)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 1)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 2)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 3)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 4)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 5)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 6)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indices, 7)]
    };
    const uint32x4_t tabLow = vld1q_u32(entries);
    const uint32x4_t tabHigh = vld1q_u32(entries + 4);
    const uint16x8_t mantissa = vshlq_n_u16(vandq_u16(clamped, vdupq_n_u16(0x7f)), 1);
    // Match binary32's 1-epsilon endpoint for saturated inputs.
    const uint16x8_t t = vbslq_u16(saturated, vdupq_n_u16(255), mantissa);

    // Widen only for interpolation, then pack all eight results in lane order.
    const uint32x4_t biasLow = vshlq_n_u32(vshrq_n_u32(tabLow, 16), 9);
    const uint32x4_t biasHigh = vshlq_n_u32(vshrq_n_u32(tabHigh, 16), 9);
    const uint32x4_t productLow = vmlal_u16(biasLow, vmovn_u32(tabLow), vget_low_u16(t));
    const uint32x4_t productHigh = vmlal_u16(biasHigh, vmovn_u32(tabHigh), vget_high_u16(t));
    const uint16x8_t values = vcombine_u16(vshrn_n_u32(productLow, 16), vshrn_n_u32(productHigh, 16));
    return vmovn_u16(values);
}


#if 0
// Sixteen-lane binary16 conversion, packed as val[0] followed by val[1].
static inline uint8x16_t float16x8x2_to_srgb8_var2_NEON(const float16x8x2_t& f) {
    const uint16x8_t bitsLow = vreinterpretq_u16_f16(f.val[0]);
    const uint16x8_t bitsHigh = vreinterpretq_u16_f16(f.val[1]);
    const uint16x8_t minval = vdupq_n_u16(0x0800);
    const uint16x8_t infinity = vdupq_n_u16(0x7c00);
    const uint16x8_t one = vdupq_n_u16(0x3c00);
    const uint16x8_t almostone = vdupq_n_u16(0x3bff);
    const uint16x8_t indexOffset = vdupq_n_u16((15 - 13) * 8);
    const uint16x8_t mantissaMask = vdupq_n_u16(0x7f);
    const uint16x8_t endpoint = vdupq_n_u16(255);

    // Map NaNs and negative values to zero, and saturate positive inputs >= 1.
    const uint16x8_t validLow = vcleq_u16(bitsLow, infinity);
    const uint16x8_t validHigh = vcleq_u16(bitsHigh, infinity);
    const uint16x8_t positiveLow = vbslq_u16(validLow, bitsLow, minval);
    const uint16x8_t positiveHigh = vbslq_u16(validHigh, bitsHigh, minval);
    const uint16x8_t saturatedLow = vcgeq_u16(positiveLow, one);
    const uint16x8_t saturatedHigh = vcgeq_u16(positiveHigh, one);
    const uint16x8_t clampedLow = vminq_u16(vmaxq_u16(positiveLow, minval), almostone);
    const uint16x8_t clampedHigh = vminq_u16(vmaxq_u16(positiveHigh, minval), almostone);
    const uint16x8_t indicesLow = vsubq_u16(vshrq_n_u16(clampedLow, 7), indexOffset);
    const uint16x8_t indicesHigh = vsubq_u16(vshrq_n_u16(clampedHigh, 7), indexOffset);

    // NEON has no general gather load; fetch packed bias/scale for all 16 lanes.
    const uint32_t entries[16] = {
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 0)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 1)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 2)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 3)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 4)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 5)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 6)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesLow, 7)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 0)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 1)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 2)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 3)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 4)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 5)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 6)],
        fp32_to_srgb8_tab4[vgetq_lane_u16(indicesHigh, 7)]
    };
    const uint32x4_t tab0 = vld1q_u32(entries);
    const uint32x4_t tab1 = vld1q_u32(entries + 4);
    const uint32x4_t tab2 = vld1q_u32(entries + 8);
    const uint32x4_t tab3 = vld1q_u32(entries + 12);
    const uint16x8_t mantissaLow = vshlq_n_u16(vandq_u16(clampedLow, mantissaMask), 1);
    const uint16x8_t mantissaHigh = vshlq_n_u16(vandq_u16(clampedHigh, mantissaMask), 1);

    // Match binary32's 1-epsilon endpoint for saturated inputs.
    const uint16x8_t tLow = vbslq_u16(saturatedLow, endpoint, mantissaLow);
    const uint16x8_t tHigh = vbslq_u16(saturatedHigh, endpoint, mantissaHigh);

    // Widen only for interpolation, then pack val[0] followed by val[1].
    const uint32x4_t bias0 = vshlq_n_u32(vshrq_n_u32(tab0, 16), 9);
    const uint32x4_t bias1 = vshlq_n_u32(vshrq_n_u32(tab1, 16), 9);
    const uint32x4_t bias2 = vshlq_n_u32(vshrq_n_u32(tab2, 16), 9);
    const uint32x4_t bias3 = vshlq_n_u32(vshrq_n_u32(tab3, 16), 9);
    const uint32x4_t product0 = vmlal_u16(bias0, vmovn_u32(tab0), vget_low_u16(tLow));
    const uint32x4_t product1 = vmlal_u16(bias1, vmovn_u32(tab1), vget_high_u16(tLow));
    const uint32x4_t product2 = vmlal_u16(bias2, vmovn_u32(tab2), vget_low_u16(tHigh));
    const uint32x4_t product3 = vmlal_u16(bias3, vmovn_u32(tab3), vget_high_u16(tHigh));
    const uint16x8_t valuesLow = vcombine_u16(vshrn_n_u32(product0, 16), vshrn_n_u32(product1, 16));
    const uint16x8_t valuesHigh = vcombine_u16(vshrn_n_u32(product2, 16), vshrn_n_u32(product3, 16));
    return vcombine_u8(vmovn_u16(valuesLow), vmovn_u16(valuesHigh));
}
#endif

#endif


simd_uchar4 float32x4_to_srgb8(const simd_float4& in) {
#if defined(__ARM_NEON)
    auto v = float32x4_to_srgb8_var2_NEON(in);
    return simd_make_uchar4(v);
#else
#error Use float_to_srgb8_var2_SSE2 instead
#endif
}


simd_uchar4 float16x4_to_srgb8(const simd_half4& in) {
#if defined(__ARM_NEON)
    auto v = float16x4_to_srgb8_var2_NEON(in);
    return simd_make_uchar4(v);
#else
#error Use float_to_srgb8_var2_SSE2 instead
#endif
}


simd_uchar8 float16x8_to_srgb8(const simd_half8& in) {
#if defined(__ARM_NEON)
    return float16x8_to_srgb8_var2_NEON(in);
#else
#error Use float_to_srgb8_var2_SSE2 instead
#endif
}
