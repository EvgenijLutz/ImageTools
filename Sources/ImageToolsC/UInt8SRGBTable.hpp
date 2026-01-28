//
//  UInt8SRGBTable.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 27.01.26.
//

#pragma once

#include <ImageToolsC/Common.hpp>


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


static inline float fromLinearToSRGB(float linear) {
    if (linear < 0.0031308) {
        return linear * 12.92;
    }
    
    return std::pow(linear, 1.0/2.4) * 1.055 - 0.055;
}


static inline float fromSRGBToLinear(float sRGB) {
    if (sRGB <= 0.04045) {
        return sRGB / 12.92;
    }
    
    return std::pow((sRGB + 0.055) / 1.055, 2.4);
}


void printUInt8Table();
