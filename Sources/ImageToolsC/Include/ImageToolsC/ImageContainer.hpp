//
//  ImageContainer.hpp
//  ImageToolsC
//
//  Created by Evgenij Lutz on 04.09.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>


class ImageContainer;
class ImageEditor;


enum ImageContainerErrorCode: long {
    fileNotFound,
    other
};


struct ImageContainerError {
    ImageContainerErrorCode code;
    char description[64];
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
    /// Unsigned 8-bit integer.
    uint8 = 0,
    
    /// Half-precision floating point value.
    float16 = 1,
    
    /// Floating point value.
    float32 = 2
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
typedef bool (* ImageToolsProgressCallback)(void* it_nullable userInfo, float progress);


/// Image container.
///
/// - Note: This object is immutable and thus thread-safe. You can access its properties from any thread.
class ImageContainer final {
private:
    std::atomic<size_t> _referenceCounter;
    
    ImagePixelFormat _pixelFormat;
    bool _sRGB;
    bool _linear;
    bool _hdr;
    
    char* it_nonnull _contents;
    long _width;
    long _height;
    long _depth;
    
    /// ICC color profile data.
    ///
    /// If `null`, then it's assumed that image has the `sRGB` color space.
    char* it_nullable _iccProfileData;
    
    /// Size of the ICC color profile data ``ImageContainer/_iccData-property``.
    ///
    /// Ignored if ``ImageContainer/_iccData-property`` is `null`.
    long _iccProfileDataLength;
    
    
    static ImageContainer* it_nullable _tryLoadPNG(const char* it_nonnull path, bool assumeSRGB) SWIFT_RETURNS_RETAINED;
    static ImageContainer* it_nullable _tryLoadOpenEXR(const char* it_nonnull path, bool assumeSRGB) SWIFT_RETURNS_RETAINED;
    
    
    ImageContainer(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, char* it_nonnull contents, long width, long height, long depth, char* it_nullable iccProfileData, long iccProfileDataLength);
    ~ImageContainer();
    
    
    friend class ImageEditor;
    friend ImageContainer* it_nullable ImageContainerRetain(ImageContainer* it_nullable image) SWIFT_RETURNS_UNRETAINED;
    friend void ImageContainerRelease(ImageContainer* it_nullable image);
    
public:
    static ImageContainer* it_nonnull rgba8Unorm(long width, long height) SWIFT_RETURNS_RETAINED;
    static ImageContainer* it_nullable load(const char* it_nullable path, bool assumeSRGB) SWIFT_NAME(__loadUnsafe(_:_:)) SWIFT_RETURNS_RETAINED;
    
    ImagePixelFormat getPixelFormat() SWIFT_COMPUTED_PROPERTY { return _pixelFormat; }
    bool getIsSRGB() SWIFT_COMPUTED_PROPERTY { return _sRGB; }
    bool getLinear() SWIFT_COMPUTED_PROPERTY { return _linear; }
    bool getHDR() SWIFT_COMPUTED_PROPERTY { return _hdr; }
    
    char* it_nonnull getContents() SWIFT_COMPUTED_PROPERTY { return _contents; }
    long getContentsSize() SWIFT_COMPUTED_PROPERTY { return _width * _height * _depth * _pixelFormat.size; }
    long getWidth() SWIFT_COMPUTED_PROPERTY { return _width; }
    long getHeight() SWIFT_COMPUTED_PROPERTY { return _height; }
    long getDepth() SWIFT_COMPUTED_PROPERTY { return _depth; }
    
    // TODO: Implement std::span for swift 6.2
    const char* it_nullable getICCProfileData() SWIFT_COMPUTED_PROPERTY { return _iccProfileData; }
    long getICCProfileDataLength() SWIFT_COMPUTED_PROPERTY { return _iccProfileDataLength; }
    
    /// Creates a copy of the image container.
    [[nodiscard("Don't forget to release the copied object using the ImageContainerRelease function.")]]
    ImageContainer* it_nonnull copy() SWIFT_RETURNS_RETAINED;
}
SWIFT_SHARED_REFERENCE(ImageContainerRetain, ImageContainerRelease)
SWIFT_UNCHECKED_SENDABLE;
