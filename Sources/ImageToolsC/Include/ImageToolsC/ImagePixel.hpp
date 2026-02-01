//
//  ImagePixel.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 10.11.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>

#if defined(__APPLE__)
#define IS_APPLE 1
#include <simd/simd.h>
#else
#define IS_APPLE 0
#endif


struct PixelPosition final {
    union {
        float contents[3];
        struct {
            float x;
            float y;
            float z;
        };
    };
    
    PixelPosition(float x, float y, float z): x(x), y(y), z(z) { }
    
    PixelPosition operator + (const PixelPosition& other) const {
        return PixelPosition(x + other.x, y + other.y,  z + other.z);
    }
    
    PixelPosition operator + (float other) const {
        return PixelPosition(x + other, y + other, z + other);
    }
    
    PixelPosition operator += (const PixelPosition& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }
    
    PixelPosition operator - (const PixelPosition& other) const {
        return PixelPosition(x - other.x, y - other.y, z - other.z);
    }
    
    PixelPosition operator - (float other) const {
        return PixelPosition(x - other, y - other, z - other);
    }
    
    PixelPosition operator -= (const PixelPosition& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }
    
    PixelPosition operator * (const PixelPosition& other) const {
        return PixelPosition(x * other.x, y * other.y, z * other.z);
    }
    
    PixelPosition operator * (float other) const {
        return PixelPosition(x * other, y * other, z * other);
    }
    
    PixelPosition operator *= (float other) {
        x *= other;
        y *= other;
        z *= other;
        return *this;
    }
    
    PixelPosition operator / (float other) const {
        return PixelPosition(x / other, y / other, z / other);
    }
    
    PixelPosition operator /= (float other) {
        x /= other;
        y /= other;
        z /= other;
        return *this;
    }
};


struct ImagePixel final {
    union {
        float contents[4];
        struct {
            float r;
            float g;
            float b;
            float a;
        };
    };
    
    ImagePixel(float r, float g, float b, float a): r(r), g(g), b(b), a(a) { }
    
    ImagePixel(): ImagePixel(0, 0, 0, 0) { }
    
    float length() const {
        return std::sqrt(r*r + g*g + b*b);
    }
    
    ImagePixel normalized() const {
        auto len = length();
        auto pixel = *this;
        pixel.r /= len;
        pixel.g /= len;
        pixel.b /= len;
        return pixel;
    }
    
    ImagePixel operator + (const ImagePixel& other) const {
        return ImagePixel(r + other.r,
                          g + other.g,
                          b + other.b,
                          a + other.a);
    }
    
    ImagePixel operator += (const ImagePixel& other) {
        r += other.r;
        g += other.g;
        b += other.b;
        a += other.a;
        return *this;
    }
    
    ImagePixel operator - (const ImagePixel& other) const {
        return ImagePixel(r - other.r,
                          g - other.g,
                          b - other.b,
                          a - other.a);
    }
    
    ImagePixel operator -= (const ImagePixel& other) {
        r -= other.r;
        g -= other.g;
        b -= other.b;
        a -= other.a;
        return *this;
    }
    
    ImagePixel operator * (float other) const {
        return ImagePixel(r * other,
                          g * other,
                          b * other,
                          a * other);
    }
    
    ImagePixel operator *= (float other) {
        r *= other;
        g *= other;
        b *= other;
        a *= other;
        return *this;
    }
    
    ImagePixel operator / (float other) const {
        return ImagePixel(r / other,
                          g / other,
                          b / other,
                          a / other);
    }
    
    ImagePixel operator /= (float other) {
        r /= other;
        g /= other;
        b /= other;
        a /= other;
        return *this;
    }
};


struct Float16Pixel final {
    union {
#if IS_APPLE
        simd_half4 __half4;
#endif
        _Float16 contents[4];
        struct {
            _Float16 r;
            _Float16 g;
            _Float16 b;
            _Float16 a;
        };
    };
    
    Float16Pixel(_Float16 r, _Float16 g, _Float16 b, _Float16 a): r(r), g(g), b(b), a(a) { }
    
#if IS_APPLE
    Float16Pixel(simd_half4 vec): __half4(vec) { }
#endif
    
    Float16Pixel(): Float16Pixel(0, 0, 0, 0) { }
    
    _Float16 length() const {
#if IS_APPLE
        return simd_length(__half4);
#else
        return std::sqrt(r*r + g*g + b*b);
#endif
    }
    
    Float16Pixel normalized() const {
#if IS_APPLE
        return simd_normalize(__half4);
#else
        auto len = length();
        auto pixel = *this;
        pixel.r /= len;
        pixel.g /= len;
        pixel.b /= len;
        return pixel;
#endif
    }
    
    Float16Pixel operator + (const Float16Pixel& other) const {
#if IS_APPLE
        return __half4 + other.__half4;
#else
        return Float16Pixel(r + other.r,
                            g + other.g,
                            b + other.b,
                            a + other.a);
#endif
    }
    
    Float16Pixel operator += (const Float16Pixel& other) {
#if IS_APPLE
        __half4 += other.__half4;
#else
        r += other.r;
        g += other.g;
        b += other.b;
        a += other.a;
#endif
        return *this;
    }
    
    Float16Pixel operator - (const Float16Pixel& other) const {
#if IS_APPLE
        return __half4 - other.__half4;
#else
        return Float16Pixel(r - other.r,
                            g - other.g,
                            b - other.b,
                            a - other.a);
#endif
    }
    
    Float16Pixel operator -= (const Float16Pixel& other) {
#if IS_APPLE
        __half4 -= other.__half4;
#else
        r -= other.r;
        g -= other.g;
        b -= other.b;
        a -= other.a;
#endif
        return *this;
    }
    
    Float16Pixel operator * (_Float16 other) const {
#if IS_APPLE
        return __half4 * other;
#else
        return Float16Pixel(r * other,
                            g * other,
                            b * other,
                            a * other);
#endif
    }
    
    Float16Pixel operator *= (_Float16 other) {
#if IS_APPLE
        __half4 *= other;
#else
        r *= other;
        g *= other;
        b *= other;
        a *= other;
#endif
        return *this;
    }
    
    Float16Pixel operator / (_Float16 other) const {
#if IS_APPLE
        return __half4 / other;
#else
        return Float16Pixel(r / other,
                            g / other,
                            b / other,
                            a / other);
#endif
    }
    
    Float16Pixel operator /= (_Float16 other) {
#if IS_APPLE
        __half4 /= other;
#else
        r /= other;
        g /= other;
        b /= other;
        a /= other;
#endif
        return *this;
    }
};
