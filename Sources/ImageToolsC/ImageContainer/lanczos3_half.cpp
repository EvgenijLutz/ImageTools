//
//  lanczos3_half.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 19.10.26.
//

#include <ImageToolsC/Common.hpp>
#include <ImageToolsC/ImageContainer.hpp>
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


static _Float16 w3Table[6] = { 0.024261, -0.135132, 0.607910, 0.607910, -0.135132, 0.024261 };

static inline simd_half4 _sampleLanczos_a3_float16_4(int left, int right, int width, simd_half4* fn_nonnull row fn_noescape) {
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
    
    return sum / totalWeight;
}


// Horizontal pass time: 0.6813s
// Vertical pass time: 0.3245s
// Downsampling time taken: 1.0058529166672088s

// Horizontal pass time: 0.1753s
// Vertical pass time: 0.0946s
// Downsampling time taken: 0.2699987083360611s

static inline simd_half3 _sampleLanczos_a3_float16_3(int left, int right, int width, _Float16* fn_nonnull row fn_noescape) {
    auto sum = simd_make_half3(0, 0, 0);
    auto value = simd_make_half3(0, 0, 0);
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
        std::memcpy(&value, row + 0*3, HALF3_PIXEL_SIZE);
    }
    while (left < pastLeftEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += value * w;
        totalWeight += w;
        
        left += 1;
    }
    auto mem = row + left*3;
    while (left <= right) {
        auto w = w3Table[pos];
        pos += 1;
        
        std::memcpy(&value, mem, HALF3_PIXEL_SIZE);
        sum += value * w;
        totalWeight += w;
        
        left += 1;
        mem += 3;
    }
    // Clamp right
    if (left <= pastRightEdge) {
        std::memcpy(&value, row + rightEdge*3, HALF3_PIXEL_SIZE);
    }
    while (left <= pastRightEdge) {
        auto w = w3Table[pos];
        pos += 1;
        
        sum += value * w;
        totalWeight += w;
        
        left += 1;
    }
    
    return sum / totalWeight;
}


static inline simd_half2 _sampleLanczos_a3_float16_2(int left, int right, int width, simd_half2* fn_nonnull row fn_noescape) {
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
    
    return sum / totalWeight;
}


static inline _Float16 _sampleLanczos_a3_float16_1(int left, int right, int width, _Float16* fn_nonnull row fn_noescape) {
    _Float16 sum = 0;
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
    
    return sum / totalWeight;
}


void ImageContainer::_resampleLanczos3_half(long width, long height, long depth) {
    // Prepare intermediate memory
    auto storageGuard = TemporaryStorageGuard(&__contents, _pixelFormat, _width, _height, _depth, _pixelFormat, width, height, depth);
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
    
    // Horizontal pass
    auto horizontalPassStart = timing_get_current_media_time();
    if (_pixelFormat.numComponents == 4) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                simd_half4* row = reinterpret_cast<simd_half4*>(sourceContents) + y * _width;
                simd_half4* outPlane = reinterpret_cast<simd_half4*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_4(left, left + 5, intWidth, row);
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
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _width * 3;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y * 3;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_3(left, left + 5, intWidth, row);
                    std::memcpy(outPlane, &pixel, HALF3_PIXEL_SIZE);
                    srcX += scaleX;
                    outPlane += _height * 3;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 2) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, _height, y) {
                simd_half2* row = reinterpret_cast<simd_half2*>(sourceContents) + y * _width;
                simd_half2* outPlane = reinterpret_cast<simd_half2*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_2(left, left + 5, intWidth, row);
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
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _width;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleX - 0.5);
                for (auto x = 0; x < width; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_1(left, left + 5, intWidth, row);
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
                simd_half4* row = reinterpret_cast<simd_half4*>(sourceContents) + y * _height;
                simd_half4* outPlane = reinterpret_cast<simd_half4*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_4(left, left + 5, intHeight, row);
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
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _height * 3;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y * 3;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_3(left, left + 5, intHeight, row);
                    std::memcpy(outPlane, &pixel, HALF3_PIXEL_SIZE);
                    srcX += scaleY;
                    outPlane += width * 3;
                }
            } LANCZOS_LOOP_END
        }
    }
    else if (_pixelFormat.numComponents == 2) {
        for (auto z = 0; z < depth; z++) {
            LANCZOS_LOOP_START(0, width, y) {
                simd_half2* row = reinterpret_cast<simd_half2*>(sourceContents) + y * _height;
                simd_half2* outPlane = reinterpret_cast<simd_half2*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_2(left, left + 5, intHeight, row);
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
                _Float16* row = reinterpret_cast<_Float16*>(sourceContents) + y * _height;
                _Float16* outPlane = reinterpret_cast<_Float16*>(destinationContents) + y;
                
                auto srcX = static_cast<float>((0 + 0.5) * scaleY - 0.5);
                for (auto x = 0; x < height; x++) {
                    int left = static_cast<int>(floor(srcX)) - 3 + 1;
                    auto pixel = _sampleLanczos_a3_float16_1(left, left + 5, intHeight, row);
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
