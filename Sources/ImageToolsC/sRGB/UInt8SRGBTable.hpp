//
//  UInt8SRGBTable.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 27.01.26.
//

#pragma once

#include <ImageToolsC/Common.hpp>
#include <simd/simd.h>

// float->sRGB8 using SSE2 (and a table)
// https://gist.github.com/rygorous/2203834


/// Enabling this flag significantly improves `sRGB <-> linear` space conversion performance (almost instant) for `uint8` `ImageContainer`s.
#define USE_UINT8_TABLE 1

struct uint8_srgb_linear_value {
    uint8_t srgb;
    uint8_t linear;
    
    __fp16 fp16SRGB;
    __fp16 fp16Linear;
    
    float fp32SRGB;
    float fp32Linear;
    
    __fp16 fp16Value;
    float fp32Value;
};


/// `uint8` value table. Generated using the ``printUInt8Table()`` function.
///
/// `Index` is the `source value`.
///
///  If you access the ``linear`` or ``fpLinear`` property, then the `source value` is in `sRGB` space.
///
///  If you access the ``srgb`` or ``fpSRGB`` property, the the `source value` is in `linear` space.
extern uint8_srgb_linear_value uint8Table[256];

extern _Float16 uint8_to_float16[256];
extern _Float16 uint8_srgb_to_float16_linear[256];
extern float uint8_srgb_to_float32_linear[256];


static inline float fromLinearToSRGB(float linear) {
    if (linear < 0.0031308f) {
        return linear * 12.92f;
    }
    
    return std::pow(linear, 1.0f / 2.4f) * 1.055f - 0.055f;
}


static inline float fromSRGBToLinear(float sRGB) {
    if (sRGB <= 0.04045f) {
        return sRGB / 12.92f;
    }
    
    return std::pow((sRGB + 0.055f) / 1.055f, 2.4f);
}


float srgb8_to_float(uint8_t val);
uint8_t float_to_srgb8(float in);
uint8_t float_to_srgb8_var2(float in);
uint8_t float_to_srgb8_ref_int(float f);

simd_uchar4 float32x4_to_srgb8(const simd_float4& in);
simd_uchar2 float16x2_to_srgb8(const simd_half2& in);
simd_uchar4 float16x4_to_srgb8(const simd_half4& in);
simd_uchar8 float16x8_to_srgb8(const simd_half8& in);


void printUInt8Table();
