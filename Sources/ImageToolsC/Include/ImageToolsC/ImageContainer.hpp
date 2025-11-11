//
//  ImageContainer.hpp
//  ImageToolsC
//
//  Created by Evgenij Lutz on 04.09.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>
#include <ImageToolsC/ImagePixel.hpp>
#include <LCMS2C/LCMS2C.hpp>
#include <ASTCEncoderC/ASTCEncoderC.hpp>


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


/// Pixel format.
///
/// Contains information of every channel.
struct ImagePixelFormat {
    /// Component type.
    PixelComponentType componentType;
    
    /// Number of components
    long numComponents;
    
    /// Determines if the last component serves as an alpha channel.
    ///
    /// Usable for color space conversion to determine if the last component should be treated as a regular pixel component value or skipped and leaved as is.
    bool hasAlpha;
    
    ImagePixelFormat(PixelComponentType componentType, long numComponents, bool hasAlpha);
    ImagePixelFormat(PixelComponentType componentType, long numComponents);
    ~ImagePixelFormat();
    
    static const ImagePixelFormat rgba8Unorm;
    
    /// How many bytes does one pixel take.
    long getSize() const SWIFT_COMPUTED_PROPERTY;
    
    /// Component size.
    long getComponentSize() const SWIFT_COMPUTED_PROPERTY;
    
    
    bool operator == (const ImagePixelFormat& other) const;
};


enum class ResamplingAlgorithm: long {
    lanczos = 0
};


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
    
    char* fn_nonnull _contents;
    long _width;
    long _height;
    long _depth;
    
    /// Color profile data.
    ///
    /// If `null`, then it's assumed that image has the `sRGB` colour profile.
    LCMSColorProfile* fn_nullable _colorProfile;
    
    
    static ImageContainer* fn_nullable _tryLoadPNG(const char* fn_nonnull path, bool assumeSRGB) SWIFT_RETURNS_RETAINED;
    static ImageContainer* fn_nullable _tryLoadOpenEXR(const char* fn_nonnull path) SWIFT_RETURNS_RETAINED;
    
    
    ImageContainer(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, char* fn_nonnull contents, long width, long height, long depth, LCMSColorProfile* fn_nullable colorProfile);
    ~ImageContainer();
    
    
    friend class ImageEditor;
    FN_FRIEND_SWIFT_INTERFACE(ImageContainer)
    
    void _assignColourProfile(LCMSColorProfile* fn_nullable colorProfile);
    bool _convertColourProfile(LCMSColorProfile* fn_nullable colorProfile);
    void _setPixel(ImagePixel pixel, long x, long y, long z);
    
public:
    static ImageContainer* fn_nonnull create(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, long width, long height, long depth, LCMSColorProfile* fn_nullable colorProfile) SWIFT_RETURNS_RETAINED;
    // TODO: Implement conversion from ASTCRawImage or ASTCImage
    //static ImageContainer* fn_nonnull create(ASTCRawImage* fn_nonnull decompressedImage, ImageContainer* fn_nonnull originalImage);
    static ImageContainer* fn_nonnull rgba8Unorm(long width, long height) SWIFT_RETURNS_RETAINED;
    static ImageContainer* fn_nullable load(const char* fn_nullable path, bool assumeSRGB) SWIFT_NAME(__loadUnsafe(_:_:)) SWIFT_RETURNS_RETAINED;
    
    ImagePixelFormat getPixelFormat() SWIFT_COMPUTED_PROPERTY { return _pixelFormat; }
    bool getIsSRGB() SWIFT_COMPUTED_PROPERTY { return _sRGB; }
    bool getLinear() SWIFT_COMPUTED_PROPERTY { return _linear; }
    bool getHDR() SWIFT_COMPUTED_PROPERTY { return _hdr; }
    
    char* fn_nonnull getContents() SWIFT_COMPUTED_PROPERTY { return _contents; }
    long getContentsSize() SWIFT_COMPUTED_PROPERTY { return _width * _height * _depth * _pixelFormat.getSize(); }
    long getWidth() SWIFT_COMPUTED_PROPERTY { return _width; }
    long getHeight() SWIFT_COMPUTED_PROPERTY { return _height; }
    long getDepth() SWIFT_COMPUTED_PROPERTY { return _depth; }
    
    LCMSColorProfile* fn_nullable getColorProfile() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_UNRETAINED { return _colorProfile; }
    
    ImagePixel getPixel(long x, long y, long z = 0);
    
    /// Creates a copy of the image container.
    [[nodiscard("Don't forget to release the copied object using the ImageContainerRelease function.")]]
    ImageContainer* fn_nonnull copy() SWIFT_RETURNS_RETAINED;
    
    ImageContainer* fn_nonnull createPromoted(PixelComponentType componentType) SWIFT_RETURNS_RETAINED;
    
    /// Estimates number of possible mip levels.
    long calculateMipLevelCount();
    ImageContainer* fn_nonnull createResampled(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth) SWIFT_RETURNS_RETAINED SWIFT_NAME(createResampled(_:quality:width:height:depth:));
    ImageContainer* fn_nonnull createDownsampled(ResamplingAlgorithm algorithm, float quality) SWIFT_RETURNS_RETAINED SWIFT_NAME(createDownsampled(_:quality:));
    
    //void generateCubeMap();
    
    [[nodiscard("Don't forget to release the image using the ASTCImageRelease function.")]]
    ASTCImage* fn_nullable createASTCCompressed(ASTCBlockSize blockSize, float quality, bool ldrAlpha = true, void* fn_nullable userInfo fn_noescape = nullptr, ASTCEncoderProgressCallback fn_nullable progressCallback fn_noescape = nullptr) SWIFT_NAME(__createASTCCompressedUnsafe(blockSize:quality:ldrAlpha:userInfo:progressCallback:)) SWIFT_RETURNS_RETAINED;
}
FN_SWIFT_INTERFACE(ImageContainer)
SWIFT_UNCHECKED_SENDABLE;


FN_DEFINE_SWIFT_INTERFACE(ImageContainer)
