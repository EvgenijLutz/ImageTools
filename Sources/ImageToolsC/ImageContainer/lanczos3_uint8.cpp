//
//  lanczos3_uint8.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 22.10.26.
//

#include <ImageToolsC/Common.hpp>
#include <ImageToolsC/ImageContainer.hpp>
#include "../sRGB/UInt8SRGBTable.hpp"
#include "../Threading.hpp"
#include "../Storage.hpp"
#include "../Timing.hpp"
#include <assert.h>

// Optimize Memory Access Patterns using Loop Interchange and Cache Blocking Techniques
// https://www.intel.com/content/www/us/en/docs/advisor/cookbook/2023-1/optimize-memory-access-patterns.html
// https://www.intel.com/content/www/us/en/docs/advisor/cookbook/2024-2/optimize-memory-access-patterns.html

// Intel® Advisor Performance Optimization Cookbook
// https://www.intel.com/content/www/us/en/docs/advisor/cookbook/2024-2/overview.html

// Extensions for loop hint optimizations
// https://clang.llvm.org/docs/LanguageExtensions.html#langext-loop-hint-optimizations


#define HALF3_PIXEL_SIZE 6
#define CHAR3_PIXEL_SIZE 3


static _Float16 w3Table[6] = { 0.024261, -0.135132, 0.607910, 0.607910, -0.135132, 0.024261 };

static inline simd_half4 _sampleLanczos_a3_uint8_4(int left, int right, int width, simd_half4* fn_nonnull pixels fn_noescape) {
    auto sum = simd_make_half4(0, 0, 0, 0);
    _Float16 totalWeight = 0.0;
    
#pragma unroll
    for (auto i = 0; i < 6; i++) {
        auto w = w3Table[i];
        auto value = pixels[i];
        sum += value * w;
        totalWeight += w;
    }

    return sum / totalWeight;
}


static inline simd_half2 _sampleLanczos_a3_uint8_2(int left, int right, int width, simd_half2* fn_nonnull pixels fn_noescape) {
    auto sum = simd_make_half2(0, 0);
    _Float16 totalWeight = 0.0;
    
#pragma unroll
    for (auto i = 0; i < 6; i++) {
        auto w = w3Table[i];
        auto value = pixels[i];
        sum += value * w;
        totalWeight += w;
    }

    return sum / totalWeight;
}


static inline _Float16 _sampleLanczos_a3_uint8(int left, int right, int width, _Float16* fn_nonnull pixels fn_noescape) {
    _Float16 sum = 0.0;
    _Float16 totalWeight = 0.0;
    
#pragma unroll
    for (auto i = 0; i < 6; i++) {
        auto w = w3Table[i];
        auto value = pixels[i];
        sum += value * w;
        totalWeight += w;
    }

    return sum / totalWeight;
}


typedef simd_uchar4 (* fn_nonnull f16x4ToU8)(const simd_half4& f16);
static inline simd_uchar4 linear_f16x4ToU8(const simd_half4& f16) {
    constexpr _Float16 v = 255.0;
    auto value = f16 * v;
    return simd_make_uchar4(value.x, value.y, value.z, value.w);
}

static inline simd_uchar4 _sampleLanczos_a3_float16_4(int left, int right, int width, simd_half4* fn_nonnull row fn_noescape, f16x4ToU8 converter fn_noescape) {
    auto sum = simd_make_half4(0, 0, 0, 0);
    _Float16 totalWeight = 0.0;
    int pos = 0;
    
    // `left` is past the left edge of the row
    auto pastLeftEdge = left;
    if (left < 0) {
        pastLeftEdge = 0;
    }
    
    // `left` is past the right edge of the row
    auto pastRightEdge = right;
    auto rightEdge = width - 1;
    if (right > rightEdge) {
        right = rightEdge;
    }
    
    // Clamp left
    while (left < pastLeftEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[0] * w;
        totalWeight += w;
        
        left += 1;
    }
    while (left <= right) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[left] * w;
        totalWeight += w;
        
        left += 1;
    }
    // Clamp right
    while (left <= pastRightEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[rightEdge] * w;
        totalWeight += w;
        
        left += 1;
    }
    
    auto result = sum / totalWeight;
    return converter(result);
}


// Horizontal pass time: 0.6813s
// Vertical pass time: 0.3245s
// Downsampling time taken: 1.0058529166672088s

// Horizontal pass time: 0.1753s
// Vertical pass time: 0.0946s
// Downsampling time taken: 0.2699987083360611s

static inline simd_uchar4 _sampleLanczos_a3_float16_3(int left, int right, int width, _Float16* fn_nonnull row fn_noescape, f16x4ToU8 converter fn_noescape) {
    auto sum = simd_make_half4(0, 0, 0, 1);
    auto tmpValue = simd_make_half4(0, 0, 0, 1);
    _Float16 totalWeight = 0.0;
    int pos = 0;
    
    // `left` is past the left edge of the row
    auto pastLeftEdge = left;
    if (left < 0) {
        pastLeftEdge = 0;
    }
    
    // `left` is past the right edge of the row
    auto pastRightEdge = right;
    auto rightEdge = width - 1;
    if (right > rightEdge) {
        right = rightEdge;
    }
    
    // Clamp left
    if (left < pastLeftEdge) {
        std::memcpy(&tmpValue, row + 0*3, HALF3_PIXEL_SIZE);
    }
    while (left < pastLeftEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += tmpValue * w;
        totalWeight += w;
        
        left += 1;
    }
    auto mem = row + left*3;
    while (left <= right) {
        auto w = w3Table[pos];
        pos += 1;
        
        std::memcpy(&tmpValue, mem, HALF3_PIXEL_SIZE);
        sum += tmpValue * w;
        totalWeight += w;
        
        left += 1;
        mem += 3;
    }
    // Clamp right
    if (left <= pastRightEdge) {
        std::memcpy(&tmpValue, row + rightEdge*3, HALF3_PIXEL_SIZE);
    }
    while (left <= pastRightEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += tmpValue * w;
        totalWeight += w;
        
        left += 1;
    }
    
    auto result = sum / totalWeight;
    auto value = converter(result);
    return value;
}


typedef simd_uchar2 (* fn_nonnull f16x2ToU8)(const simd_half2& f16);
static inline simd_uchar2 linear_f16x2ToU8(const simd_half2& f16) {
    constexpr _Float16 v = 255.0;
    auto value = f16 * v;
    return simd_make_uchar2(value.x, value.y);
}

static inline simd_uchar2 _sampleLanczos_a3_float16_2(int left, int right, int width, simd_half2* fn_nonnull row fn_noescape, f16x2ToU8 converter fn_noescape) {
    auto sum = simd_make_half2(0, 0);
    _Float16 totalWeight = 0.0;
    int pos = 0;
    
    // `left` is past the left edge of the row
    auto pastLeftEdge = left;
    if (left < 0) {
        pastLeftEdge = 0;
    }
    
    // `left` is past the right edge of the row
    auto pastRightEdge = right;
    auto rightEdge = width - 1;
    if (right > rightEdge) {
        right = rightEdge;
    }
    
    // Clamp left
    while (left < pastLeftEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[0] * w;
        totalWeight += w;
        
        left += 1;
    }
    while (left <= right) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[left] * w;
        totalWeight += w;
        
        left += 1;
    }
    // Clamp right
    while (left <= pastRightEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[rightEdge] * w;
        totalWeight += w;
        
        left += 1;
    }
    
    auto result = sum / totalWeight;
    return converter(result);
}


typedef uint8_t (* fn_nonnull f16ToU8)(float f16);
static inline uint8_t linear_f16ToU8(float f16) {
    constexpr _Float16 v = 255.0;
    return f16 * v;
}

static inline uint8_t _sampleLanczos_a3_float16(int left, int right, int width, _Float16* fn_nonnull row fn_noescape, f16ToU8 converter fn_noescape) {
    _Float16 sum = 0.0;
    _Float16 totalWeight = 0.0;
    int pos = 0;
    
    // `left` is past the left edge of the row
    auto pastLeftEdge = left;
    if (left < 0) {
        pastLeftEdge = 0;
    }
    
    // `left` is past the right edge of the row
    auto pastRightEdge = right;
    auto rightEdge = width - 1;
    if (right > rightEdge) {
        right = rightEdge;
    }
    
    // Clamp left
    while (left < pastLeftEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[0] * w;
        totalWeight += w;
        
        left += 1;
    }
    while (left <= right) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[left] * w;
        totalWeight += w;
        
        left += 1;
    }
    // Clamp right
    while (left <= pastRightEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += row[rightEdge] * w;
        totalWeight += w;
        
        left += 1;
    }
    
    auto result = sum / totalWeight;
    return converter(result);
}


void ImageContainer::_resampleLanczos3_uint8(long width, long height, long depth) {
    // Prepare intermediate memory
    auto tmpFormat = _pixelFormat;
    tmpFormat.componentType = PixelComponentType::float16;
    auto storageGuard = TemporaryStorageGuard(&__contents, _pixelFormat, _width, _height, _depth, tmpFormat, width, height, depth);
    auto tmpBuffer = static_cast<std::byte*>(storageGuard._intermediateBuffer);
        
    // Source and destination contents for resampling passes
    auto sourceContents = _contents;
    auto destinationContents = tmpBuffer;
    
    // Prepare some often used variables for resampling passes
    auto scaleX = static_cast<float>(_width) / width;
    auto scaleY = static_cast<float>(_height) / height;
    auto scaleZ = static_cast<float>(_depth) / depth;
    
    auto intWidth = static_cast<int>(_width);
    auto intHeight = static_cast<int>(_height);
    
    auto rgb8ToF16 = _sRGB ? uint8_srgb_to_float16_linear : uint8_to_float16;
    auto a8ToF16 = uint8_to_float16;
    
    // Horizontal pass: fastest 0.3s before caching, now 0.17s
    auto horizontalPassStart = timing_get_current_media_time();
    if (_pixelFormat.numComponents == 4) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                simd_half4 pixels[6];
                simd_uchar4* row = reinterpret_cast<simd_uchar4*>(sourceContents) + y * _width;
                simd_half4* outPlane = reinterpret_cast<simd_half4*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                auto lastLeft = static_cast<int>(floor(srcX)) - 3 + 1;
                lastLeft -= 6;
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    // Shift pixels by srcX offset
                    auto shift = left - lastLeft;
                    for (auto i = shift; i < 6; i++) {
                        pixels[i - shift] = pixels[i];
                    }
                    for (auto i = 6-shift; i < 6; i++) {
                        auto w = i + lastLeft;
                        if (w < 0) { w = 0; }
                        else if (w >= intWidth) { w = intWidth - 1; }
                        auto pixel = row[w];
                        auto r = rgb8ToF16[pixel.x];
                        auto g = rgb8ToF16[pixel.y];
                        auto b = rgb8ToF16[pixel.z];
                        auto a = a8ToF16[pixel.w];
                        pixels[i] = simd_make_half4(r, g, b, a);
                    }
                    lastLeft = left;
                    
                    auto pixel = _sampleLanczos_a3_uint8_4(left, left + 5, intWidth, pixels);
                    *outPlane = pixel;
                    srcX += scaleX;
                    outPlane += _height;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 3) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                simd_half4 pixels[6];
                uint8_t* row = reinterpret_cast<uint8_t*>(sourceContents) + y * _width * 3;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y * 3;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                auto lastLeft = static_cast<int>(floor(srcX)) - 3 + 1;
                lastLeft -= 6;
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    // Shift pixels by srcX offset
                    auto shift = left - lastLeft;
                    for (auto i = shift; i < 6; i++) {
                        pixels[i - shift] = pixels[i];
                    }
                    for (auto i = 6-shift; i < 6; i++) {
                        auto w = i + lastLeft;
                        if (w < 0) { w = 0; }
                        else if (w >= intWidth) { w = intWidth - 1; }
                        auto pixel = row + w*3;
                        auto r = rgb8ToF16[pixel[0]];
                        auto g = rgb8ToF16[pixel[1]];
                        auto b = rgb8ToF16[pixel[2]];
                        auto a = 1;
                        pixels[i] = simd_make_half4(r, g, b, a);
                    }
                    lastLeft = left;
                    
                    auto pixel = _sampleLanczos_a3_uint8_4(left, left + 5, intWidth, pixels);
                    outPlane[0] = pixel.x;
                    outPlane[1] = pixel.y;
                    outPlane[2] = pixel.z;
                    srcX += scaleX;
                    outPlane += _height * 3;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 2) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                simd_half2 pixels[6];
                simd_uchar2* row = reinterpret_cast<simd_uchar2*>(sourceContents) + y * _width;
                simd_half2* outPlane = reinterpret_cast<simd_half2*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                auto lastLeft = static_cast<int>(floor(srcX)) - 3 + 1;
                lastLeft -= 6;
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    // Shift pixels by srcX offset
                    auto shift = left - lastLeft;
                    for (auto i = shift; i < 6; i++) {
                        pixels[i - shift] = pixels[i];
                    }
                    for (auto i = 6-shift; i < 6; i++) {
                        auto w = i + lastLeft;
                        if (w < 0) { w = 0; }
                        else if (w >= intWidth) { w = intWidth - 1; }
                        auto pixel = row[w];
                        auto r = rgb8ToF16[pixel.x];
                        auto g = rgb8ToF16[pixel.y];
                        pixels[i] = simd_make_half2(r, g);
                    }
                    lastLeft = left;
                    
                    auto pixel = _sampleLanczos_a3_uint8_2(left, left + 5, intWidth, pixels);
                    *outPlane = pixel;
                    srcX += scaleX;
                    outPlane += _height;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 1) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                _Float16 pixels[6];
                uint8_t* row = reinterpret_cast<uint8_t*>(sourceContents) + y * _width;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                auto lastLeft = static_cast<int>(floor(srcX)) - 3 + 1;
                lastLeft -= 6;
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    // Shift pixels by srcX offset
                    auto shift = left - lastLeft;
                    for (auto i = shift; i < 6; i++) {
                        pixels[i - shift] = pixels[i];
                    }
                    for (auto i = 6-shift; i < 6; i++) {
                        auto w = i + lastLeft;
                        if (w < 0) { w = 0; }
                        else if (w >= intWidth) { w = intWidth - 1; }
                        auto pixel = row[w];
                        pixels[i] = rgb8ToF16[pixel];
                    }
                    lastLeft = left;
                    
                    auto pixel = _sampleLanczos_a3_uint8(left, left + 5, intWidth, pixels);
                    *outPlane = pixel;
                    srcX += scaleX;
                    outPlane += _height;
                }
            } LANCZOS_LOOP_END
        }
    }
    else {
        assert(0 && "Unsupported number of components");
    }
    // Prepare source and destination contents for further processing
    std::swap(sourceContents, destinationContents);
    auto horizontalPassElapsed = timing_get_current_media_time() - horizontalPassStart;
    printf("Horizontal pass time: %.4fs\n", horizontalPassElapsed);
    
    
    // Vertical pass
    auto verticalPassStart = timing_get_current_media_time();
    if (_pixelFormat.numComponents == 4) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, width, y) {
                f16x4ToU8 converter = _sRGB ? float16x4_to_srgb8 : linear_f16x4ToU8;
                simd_half4* row = reinterpret_cast<simd_half4*>(sourceContents) + y * _height;
                simd_uchar4* outPlane = reinterpret_cast<simd_uchar4*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_4(left, left + 5, intHeight, row, converter);
                    *outPlane = pixel;
                    srcX += scaleY;
                    outPlane += width;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 3) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, width, y) {
                f16x4ToU8 converter = _sRGB ? float16x4_to_srgb8 : linear_f16x4ToU8;
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _height * 3;
                uint8_t* outPlane = reinterpret_cast<uint8_t*>(destinationContents) + y * 3;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_3(left, left + 5, intHeight, row, converter);
                    std::memcpy(outPlane, &pixel, CHAR3_PIXEL_SIZE);
                    srcX += scaleY;
                    outPlane += width * 3;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 2) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, width, y) {
                f16x2ToU8 converter = _sRGB ? float16x2_to_srgb8 : linear_f16x2ToU8;
                simd_half2* row = reinterpret_cast<simd_half2*>(sourceContents) + y * _height;
                simd_uchar2* outPlane = reinterpret_cast<simd_uchar2*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_2(left, left + 5, intHeight, row, converter);
                    *outPlane = pixel;
                    srcX += scaleY;
                    outPlane += width;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 1) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, width, y) {
                f16ToU8 converter = _sRGB ? float_to_srgb8_var2 : linear_f16ToU8;
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _height;
                uint8_t* outPlane = reinterpret_cast<uint8_t*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16(left, left + 5, intHeight, row, converter);
                    *outPlane = pixel;
                    srcX += scaleY;
                    outPlane += width;
                }
            } LANCZOS_LOOP_END
        }
    }
    else {
        assert(0 && "Unsupported number of components");
    }
    // Prepare source and destination contents for further processing
    std::swap(sourceContents, destinationContents);
    auto verticalPassElapsed = timing_get_current_media_time() - verticalPassStart;
    printf("Vertical pass time: %.4fs\n", verticalPassElapsed);
    
    
    // Depth pass if needed
    if (depth != _depth) {
        for (auto z = 0; z < depth; z++) {
            CONCURRENT_LOOP_START(0, height, y) {
                for (auto x = 0; x < width; x++) {
                    auto srcZ = (z + 0.5) * scaleZ - 0.5;
                    auto pixel = _sampleLanczosZ_general(x, y, srcZ, 3, width, height, _depth, sourceContents, _pixelFormat.numComponents, _pixelFormat.componentType, false);
                    _setPixel_general(pixel, x, y, z, width, height, depth, destinationContents, _pixelFormat.numComponents, _pixelFormat.componentType);
                }
            } CONCURRENT_LOOP_END
        }
        // Prepare source and destination contents for further processing
        std::swap(sourceContents, destinationContents);
    }
    

    // Copy temporary buffer to original if needed
    if (_contents != sourceContents) {
        storageGuard.copyFinalTargetContents();
    }
    
    // Apply size
    _width = width;
    _height = height;
}
