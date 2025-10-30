//
//  ImageContainer.hpp
//  ImageToolsC
//
//  Created by Evgenij Lutz on 04.09.25.
//

#pragma once

#include <swift/bridging>
#include <atomic>


#if !defined nonnull
#define nonnull __nonnull
#endif

#if !defined nullable
#define nullable __nullable
#endif


enum class ImageContainerColorSpace: long {
    unknown = 0,
    sRGB = 1,
    p3 = 2,
    customICCProfile = 3
};


enum class ImagePixelChannel: long {
    /// Red.
    r = 0,
    
    /// Green.
    g = 1,
    
    /// Bliue.
    b = 2,
    
    /// Alpha.
    a = 3
};


enum class PixelComponentType: long {
    /// Signed 8-bit integer.
    sint8 = 0,
    
    /// Unsigned 8-bit integer.
    uint8,
    
    float16
};


/// Returns ``PixelComponentType``'s size in bytes
long getPixelComponentTypeSize(PixelComponentType type);


struct ImagePixelComponent {
    /// Channel name.
    ImagePixelChannel channel;
    
    /// Component type.
    PixelComponentType type;
    
    
    bool operator == (const ImagePixelComponent& other) const;
};


/// Pixel format.
///
/// Contains information of every channel.
struct ImagePixelFormat {
    /// Number of components
    long numComponents;
    
    /// Components
    ImagePixelComponent components[4];
    
    /// How many bytes does one pixel take.
    long size;
    
    static const ImagePixelFormat rgba8Unorm;
    
    bool validate() const;
    
    /// Checks if the pixel format consists of components of the same type.
    bool isHomogeneous() const;
    
    
    bool operator == (const ImagePixelFormat& other) const;
};


/// Progress callback.
///
/// Nofities about current operation's progress.
///
/// - Returns: `true` if the operation should be calcelled, otherwise `false`.
typedef bool (* ImageToolsProgressCallback)(void* nullable userInfo, float progress);


/// Image container.
class ImageContainer final {
private:
    std::atomic<size_t> _referenceCounter;
    
    ImageContainerColorSpace _colorSpace;
    
    ImagePixelFormat _pixelFormat;
    bool _linear;
    bool _hdr;
    
    char* nonnull _contents;
    long _width;
    long _height;
    long _depth;
    
    /// ICC color profile data.
    ///
    /// If `null`, then it's assumed that image has the `sRGB` color space.
    char* nullable _iccProfileData;
    
    /// Size of the ICC color profile data ``ImageContainer/_iccData-property``.
    ///
    /// Ignored if ``ImageContainer/_iccData-property`` is `null`.
    long _iccProfileDataLength;
    
    
    friend ImageContainer* nullable ImageContainerRetain(ImageContainer* nullable image) SWIFT_RETURNS_UNRETAINED;
    friend void ImageContainerRelease(ImageContainer* nullable image);
    
    ImageContainer(ImageContainerColorSpace colorSpace, ImagePixelFormat pixelFormat, bool linear, bool hdr, char* nonnull contents, long width, long height, long depth, char* nullable iccProfileData, long iccProfileDataLength);
    ~ImageContainer();
    
public:
    static ImageContainer* nonnull rgba8Unorm(long width, long height) SWIFT_RETURNS_RETAINED;
    static ImageContainer* nullable load(const char* nullable path) SWIFT_NAME(__loadUnsafe(_:)) SWIFT_RETURNS_RETAINED;
    
    ImageContainerColorSpace getColorSpace() SWIFT_COMPUTED_PROPERTY { return _colorSpace; }
    
    ImagePixelFormat getPixelFormat() SWIFT_COMPUTED_PROPERTY { return _pixelFormat; }
    bool getLinear() SWIFT_COMPUTED_PROPERTY { return _linear; }
    bool getHDR() SWIFT_COMPUTED_PROPERTY { return _hdr; }
    
    char* nonnull getContents() SWIFT_COMPUTED_PROPERTY { return _contents; }
    long getContentsSize() SWIFT_COMPUTED_PROPERTY { return _width * _height * _depth * _pixelFormat.size; }
    long getWidth() SWIFT_COMPUTED_PROPERTY { return _width; }
    long getHeight() SWIFT_COMPUTED_PROPERTY { return _height; }
    long getDepth() SWIFT_COMPUTED_PROPERTY { return _depth; }
    
    // TODO: Implement std::span for swift 6.2
    const char* nullable getICCProfileData() SWIFT_COMPUTED_PROPERTY { return _iccProfileData; }
    long getICCProfileDataLength() SWIFT_COMPUTED_PROPERTY { return _iccProfileDataLength; }
    void setICCProfileData(const char* nullable iccProfileData, long iccProfileDataLength);
    
    /// Creates a copy of the image container.
    [[nodiscard("Don't forget to release the copied object using the ImageContainerRelease function.")]]
    ImageContainer* nonnull copy() SWIFT_RETURNS_RETAINED;
    
    
    /// Converts pixel format if possible.
    bool convertPixelFormat(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback);
    
//    /// Swizzles pixel components.
//    ///
//    /// - Returns: `false` if `targetPixelFormat`'s components do not match pixel format components of this image. `true` if succeeds.
//    bool swizzle(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback);
} SWIFT_SHARED_REFERENCE(ImageContainerRetain, ImageContainerRelease);
