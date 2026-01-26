//
//  ImagePixel.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 10.11.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>


struct PixelPosition {
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


struct ImagePixel {
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
