//
//  checkIfFlatColor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 18.10.26.
//

#include <ImageToolsC/ImageContainer.hpp>


static bool check_flat_color_uint8(uint8_t* fn_nonnull contents fn_noescape, long width, long height, long depth, long numComponents, long channel, float* fn_nullable outColor fn_noescape) {
    auto value = contents[channel];
    if (outColor) {
        *outColor = static_cast<float>(value) / 255.0f;
    }
    
    for (auto z = 0; z < depth; z++) {
        for (auto y = 0; y < height; y++) {
            auto pixel = contents + (y * width + z * width * height) * numComponents + channel;
            for (auto x = 0; x < width; x++) {
                if (*pixel != value) {
                    return false;
                }
                
                pixel += numComponents;
            }
        }
    }
    
    return true;
}


static bool check_flat_color_float16(_Float16* fn_nonnull contents fn_noescape, long width, long height, long depth, long numComponents, long channel, float* fn_nullable outColor fn_noescape) {
    auto value = contents[channel];
    if (outColor) {
        *outColor = static_cast<float>(value);
    }
    
    for (auto z = 0; z < depth; z++) {
        for (auto y = 0; y < height; y++) {
            auto pixel = contents + (y * width + z * width * height) * numComponents + channel;
            for (auto x = 0; x < width; x++) {
                if (*pixel != value) {
                    return false;
                }
                
                pixel += numComponents;
            }
        }
    }
    
    return true;
}


static bool check_flat_color_float32(float* fn_nonnull contents fn_noescape, long width, long height, long depth, long numComponents, long channel, float* fn_nullable outColor fn_noescape) {
    auto value = contents[channel];
    if (outColor) {
        *outColor = value;
    }
    
    for (auto z = 0; z < depth; z++) {
        for (auto y = 0; y < height; y++) {
            auto pixel = contents + (y * width + z * width * height) * numComponents + channel;
            for (auto x = 0; x < width; x++) {
                if (*pixel != value) {
                    return false;
                }
                
                pixel += numComponents;
            }
        }
    }
    
    return true;
}


bool ImageContainer::checkIfFlatColor(long channel, float* fn_nullable outColor fn_noescape) {
    if (channel < 0 || channel >= _pixelFormat.numComponents) {
        return false;
    }
    
    switch (_pixelFormat.componentType) {
        case PixelComponentType::uint8:
            return check_flat_color_uint8(_uint8Contents, _width, _height, _depth, _pixelFormat.numComponents, channel, outColor);
            
        case PixelComponentType::float16:
            return check_flat_color_float16(_float16Contents, _width, _height, _depth, _pixelFormat.numComponents, channel, outColor);
            
        case PixelComponentType::float32:
            return check_flat_color_float32(_float32Contents, _width, _height, _depth, _pixelFormat.numComponents, channel, outColor);
    }
}
