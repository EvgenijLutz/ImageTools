//
//  ImageContainer.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 30.10.25.
//

#include <ImageToolsC/ImageContainer.hpp>
#include <FastTGAC/FastTGAC.hpp>
#include <JPEGTurboC/JPEGTurboC.hpp>
#include <LibPNGC/LibPNGC.hpp>
#include <LCMS2C/LCMS2C.hpp>
#include "Threading.hpp"
#include "UInt8SRGBTable.hpp"
#include <assert.h>


#if __has_include(<TargetConditionals.h>)
#include <TargetConditionals.h>
#endif

#if defined __APPLE__
#define STBI_NEON
#else
// No NEON :(
#endif

#include "stb/stb_image.h"
#include "tinyexr/tinyexr.h"


// MARK: - Common functions

static long _calculateMipCount(long size) {
    auto count = 1;
    auto currentSize = size;
    while (currentSize > 1) {
        count += 1;
        currentSize /= 2;
    }
    return count;
}


static inline void clamp_xyz(long& x, long width, long& y, long height, long& z, long depth) {
#if 0
    // Clamp
    x = std::clamp(x, 0l, width - 1);
    y = std::clamp(y, 0l, height - 1);
    z = std::clamp(z, 0l, depth - 1);
#else
    // Works faster at least in the debug mode
    if (x < 0) { x = 0; } else if (x >= width) { x = width - 1; }
    if (y < 0) { y = 0; } else if (y >= height) { y = height - 1; }
    if (z < 0) { z = 0; } else if (z >= depth) { z = depth - 1; }
#endif
}


// MARK: - GetPixel

static inline ImagePixel _getPixel_general(long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents, long numComponents, PixelComponentType componentType) {
    auto pixel = ImagePixel();
    
    clamp_xyz(x, width, y, height, z, depth);
    
    auto index = (z * width * height + y * width + x) * numComponents;
    switch (componentType) {
        case PixelComponentType::uint8: {
            auto uint8Pixel = reinterpret_cast<uint8_t*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                pixel.contents[i] = static_cast<float>(uint8Pixel[i]) / std::numeric_limits<uint8_t>::max();
            }
            break;
        }
            
        case PixelComponentType::float16: {
            auto float16Pixel = reinterpret_cast<__fp16*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                pixel.contents[i] = static_cast<float>(float16Pixel[i]);
            }
            break;
        }
            
        case PixelComponentType::float32: {
            auto float32Pixel = reinterpret_cast<float*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                pixel.contents[i] = float32Pixel[i];
            }
            break;
        }
    }
    
    return pixel;
}


template <long numComponents>
static inline Float16Pixel _getPixel_float16(long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents) {
    auto pixel = Float16Pixel();
    
    clamp_xyz(x, width, y, height, z, depth);
    
    auto index = (z * width * height + y * width + x) * numComponents;
    
    auto float16Pixel = reinterpret_cast<__fp16*>(contents) + index;
    if constexpr (numComponents >= 1) {
        pixel.contents[0] = static_cast<float>(float16Pixel[0]);
    }
    if constexpr (numComponents >= 2) {
        pixel.contents[1] = static_cast<float>(float16Pixel[1]);
    }
    if constexpr (numComponents >= 3) {
        pixel.contents[2] = static_cast<float>(float16Pixel[2]);
    }
    if constexpr (numComponents >= 4) {
        pixel.contents[3] = static_cast<float>(float16Pixel[3]);
    }
    
    return pixel;
}


template <long numComponents>
static inline ImagePixel _getPixel_float32(long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents) {
    auto pixel = ImagePixel();
    
    clamp_xyz(x, width, y, height, z, depth);
    
    auto index = (z * width * height + y * width + x) * numComponents;
    
    auto float32Pixel = reinterpret_cast<float*>(contents) + index;
    if constexpr (numComponents >= 1) {
        pixel.contents[0] = static_cast<float>(float32Pixel[0]);
    }
    if constexpr (numComponents >= 2) {
        pixel.contents[1] = static_cast<float>(float32Pixel[1]);
    }
    if constexpr (numComponents >= 3) {
        pixel.contents[2] = static_cast<float>(float32Pixel[2]);
    }
    if constexpr (numComponents >= 4) {
        pixel.contents[3] = static_cast<float>(float32Pixel[3]);
    }
    
    return pixel;
}


// MARK: - SetPixel

static inline void _setPixel_general(ImagePixel pixel, long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents, long numComponents, PixelComponentType componentType) {
    if ((x < 0 || x >= width) || (y < 0 || y >= height) || (z < 0 || z >= depth)) {
        return;
    }
    
    auto index = (z * width * height + y * width + x) * numComponents;
    switch (componentType) {
        case PixelComponentType::uint8: {
            auto uint8Pixel = reinterpret_cast<uint8_t*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                auto converted = pixel.contents[i] * std::numeric_limits<uint8_t>::max();
                uint8Pixel[i] = static_cast<uint8_t>(std::min(255.0f, converted));
            }
            break;
        }
            
        case PixelComponentType::float16: {
            auto float16Pixel = reinterpret_cast<__fp16*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                float16Pixel[i] = static_cast<__fp16>(pixel.contents[i]);
            }
            break;
        }
            
        case PixelComponentType::float32: {
            auto float32Pixel = reinterpret_cast<float*>(contents) + index;
            for (auto i = 0; i < numComponents; i++) {
                float32Pixel[i] = pixel.contents[i];
            }
            break;
        }
    }
}


template <long numComponents>
static inline void _setPixel_float16(Float16Pixel pixel, long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents) {
    if ((x < 0 || x >= width) || (y < 0 || y >= height) || (z < 0 || z >= depth)) {
        return;
    }
    
    auto index = (z * width * height + y * width + x) * numComponents;
    
    auto float16Pixel = reinterpret_cast<__fp16*>(contents) + index;
    if constexpr (numComponents >= 1) {
        float16Pixel[0] = pixel.contents[0];
    }
    if constexpr (numComponents >= 2) {
        float16Pixel[1] = pixel.contents[1];
    }
    if constexpr (numComponents >= 3) {
        float16Pixel[2] = pixel.contents[2];
    }
    if constexpr (numComponents >= 4) {
        float16Pixel[3] = pixel.contents[3];
    }
}


template <long numComponents>
static inline void _setPixel_float32(ImagePixel pixel, long x, long y, long z, long width, long height, long depth, char* fn_nonnull contents) {
    if ((x < 0 || x >= width) || (y < 0 || y >= height) || (z < 0 || z >= depth)) {
        return;
    }
    
    auto index = (z * width * height + y * width + x) * numComponents;
    
    auto float32Pixel = reinterpret_cast<float*>(contents) + index;
    if constexpr (numComponents >= 1) {
        float32Pixel[0] = pixel.contents[0];
    }
    if constexpr (numComponents >= 2) {
        float32Pixel[1] = pixel.contents[1];
    }
    if constexpr (numComponents >= 3) {
        float32Pixel[2] = pixel.contents[2];
    }
    if constexpr (numComponents >= 4) {
        float32Pixel[3] = pixel.contents[3];
    }
}


static inline bool _convertColorProfile(LCMSColorProfile* fn_nullable colorProfile, LCMSColorProfile* fn_nullable sourceColorProfile, long width, long height, char* fn_nonnull contents, ImagePixelFormat pixelFormat, bool hdr) {
    // TODO: Create a noncopyable struct that stores LCMSImage with referenced image data to avoid unnecessary data copy
    // TODO: For instance, struct EphemeralLCMSImage { /* ... */ };
    // Create an image for colour conversion
    auto cmsImage = LCMSImage::createBorrowing(contents,
                                               width, height,
                                               pixelFormat.numComponents, pixelFormat.getComponentSize(),
                                               hdr,
                                               sourceColorProfile);
    if (cmsImage == nullptr) {
        return false;
    }
    
    // Convert colour profile
    auto converted = cmsImage->convertColorProfile(colorProfile);
    if (converted == false) {
        LCMSImageRelease(cmsImage);
        return false;
    }
    
    // Clean up
    LCMSImageRelease(cmsImage);
    
    // Success
    return true;
}


// MARK: - Lanczos

static inline float _sinc_float32(float x) {
    if (x == 0.0) return 1.0;
    x *= M_PI;
    return sin(x) / x;
}

static inline float _lanczos_float32(float x, float a) {
    if (fabs(x) >= a) return 0.0;
    return _sinc_float32(x) * _sinc_float32(x / a);
}


static inline __fp16 _sinc_float16(__fp16 x) {
    if (x == 0.0) return 1.0;
    x *= M_PI;
    return sin(x) / x;
}

static inline __fp16 _lanczos_float16(__fp16 x, __fp16 a) {
    if (fabs(x) >= a) return 0.0;
    return _sinc_float16(x) * _sinc_float16(x / a);
}


// MARK: - Lanczos X

static inline ImagePixel _sampleLanczosX_general(float x, float y, float z, float a, long width, long height, long depth, char* fn_nonnull contents, long numComponents, PixelComponentType componentType, bool renormalize) {
    long left = floor(x - a + 1);
    long right = floor(x + a);
    auto sum = ImagePixel();
    float totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        float w = _lanczos_float32(x - i, a);
        sum += _getPixel_general(i, y, z, width, height, depth, contents, numComponents, componentType) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}

template<long numComponents>
static inline Float16Pixel _sampleLanczosX_float16(__fp16 x, __fp16 y, __fp16 z, __fp16 a, long width, long height, long depth, char* fn_nonnull contents, bool renormalize) {
    long left = floor(x - a + 1);
    long right = floor(x + a);
    auto sum = Float16Pixel();
    __fp16 totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float16(x - i, a);
        sum += _getPixel_float16<numComponents>(i, y, z, width, height, depth, contents) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}

template<long numComponents>
static inline ImagePixel _sampleLanczosX_float32(float x, float y, float z, float a, long width, long height, long depth, char* fn_nonnull contents, bool renormalize) {
    long left = floor(x - a + 1);
    long right = floor(x + a);
    auto sum = ImagePixel();
    float totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float32(x - i, a);
        sum += _getPixel_float32<numComponents>(i, y, z, width, height, depth, contents) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}


// MARK: - Lanczos Y

static inline ImagePixel _sampleLanczosY_general(float x, float y, float z, float a, long width, long height, long depth, char* fn_nonnull contents, long numComponents, PixelComponentType componentType, bool renormalize) {
    long left = floor(y - a + 1);
    long right = floor(y + a);
    auto sum = ImagePixel();
    float totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float32(y - i, a);
        sum += _getPixel_general(x, i, z, width, height, depth, contents, numComponents, componentType) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}

template<long numComponents>
static inline Float16Pixel _sampleLanczosY_float16(__fp16 x, __fp16 y, __fp16 z, __fp16 a, long width, long height, long depth, char* fn_nonnull contents, bool renormalize) {
    long left = floor(y - a + 1);
    long right = floor(y + a);
    auto sum = Float16Pixel();
    __fp16 totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float16(y - i, a);
        sum += _getPixel_float16<numComponents>(x, i, z, width, height, depth, contents) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}

template<long numComponents>
static inline ImagePixel _sampleLanczosY_float32(float x, float y, float z, float a, long width, long height, long depth, char* fn_nonnull contents, bool renormalize) {
    long left = floor(y - a + 1);
    long right = floor(y + a);
    auto sum = ImagePixel();
    float totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float32(y - i, a);
        sum += _getPixel_float32<numComponents>(x, i, z, width, height, depth, contents) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}


// MARK: - Lanczos Z

static inline ImagePixel _sampleLanczosZ_general(float x, float y, float z, float a, long width, long height, long depth, char* fn_nonnull contents, long numComponents, PixelComponentType componentType, bool renormalize) {
    long left = floor(z - a + 1);
    long right = floor(z + a);
    auto sum = ImagePixel();
    float totalWeight = 0.0;
    for (auto i = left; i <= right; ++i) {
        auto w = _lanczos_float32(z - i, a);
        sum += _getPixel_general(x, y, i, width, height, depth, contents, numComponents, componentType) * w;
        totalWeight += w;
    }
    if (renormalize) {
        return (sum / totalWeight).normalized();
    }
    return sum / totalWeight;
}


// MARK: - PixelInfo

template <typename SourceType, typename DestinationType>
struct PixelInfo {
    union Value {
        SourceType sourceTypeValue;
        unsigned char bytes[sizeof(SourceType)];
    };
    Value _value;
    
    // Sanity check
    static_assert(sizeof(Value) == sizeof(SourceType), "Source and destination sizes should match");
    
    DestinationType convert(bool littleEndian) const {
        // Copy the value
        auto v = _value;
        
        // Swap bytes instead of manually bit shift to make the value little endian
        if (littleEndian == false) {
            constexpr auto numBytes = sizeof(SourceType);
            constexpr auto halfSize = numBytes / 2;
            constexpr auto lastByteIndex = numBytes - 1;
            for (auto byteIndex = 0; byteIndex < halfSize; byteIndex++) {
                auto byte0 = v.bytes[byteIndex];
                auto byte1 = v.bytes[lastByteIndex - byteIndex];
                v.bytes[byteIndex] = byte1;
                v.bytes[lastByteIndex - byteIndex] = byte0;
            }
        }
        
        // Cast to double - takes more space, produces less precision errors
        auto value = static_cast<double>(v.sourceTypeValue);
        
        // Cast to the destination type
        static auto sourceMax = std::numeric_limits<SourceType>::max();
        return static_cast<DestinationType>(value / static_cast<double>(sourceMax));
    }
};


long getPixelComponentTypeSize(PixelComponentType type) {
    long sizes[] = {
        1,
        2,
        4
    };
    
    return sizes[static_cast<long>(type)];
}


ImagePixelFormat::ImagePixelFormat(PixelComponentType componentType, long numComponents, bool hasAlpha):
componentType(componentType),
numComponents(numComponents),
hasAlpha(hasAlpha) { }


ImagePixelFormat::ImagePixelFormat(PixelComponentType componentType, long numComponents):
// Assume that GA or RGBA images treat last components as alpha channels
ImagePixelFormat(componentType, numComponents, numComponents == 2 || numComponents == 4) { }


ImagePixelFormat::~ImagePixelFormat() { }


const ImagePixelFormat ImagePixelFormat::rgba8Unorm = ImagePixelFormat(PixelComponentType::uint8, 4, true);


long ImagePixelFormat::getSize() const {
    return getPixelComponentTypeSize(componentType) * numComponents;
}


long ImagePixelFormat::getComponentSize() const {
    return getPixelComponentTypeSize(componentType);
}


bool ImagePixelFormat::operator == (const ImagePixelFormat& other) const {
    return numComponents == other.numComponents &&
    componentType == other.componentType &&
    hasAlpha == other.hasAlpha;
}


// MARK: - ImageContainerCollection

ImageContainerCollection::ImageContainerCollection():
_numImages(0) { }

ImageContainerCollection::ImageContainerCollection(const ImageContainerCollection& other):
_numImages(other._numImages) {
    for (auto i = 0; i < _numImages; i++) {
        _images[i] = ImageContainerRetain(other._images[i]);
    }
}

ImageContainerCollection::ImageContainerCollection(ImageContainerCollection&& other):
_numImages(std::exchange(other._numImages, 0)) {
    for (auto i = 0; i < _numImages; i++) {
        _images[i] = other._images[i];
    }
}

ImageContainerCollection::~ImageContainerCollection() {
    for (auto i = 0; i < _numImages; i++) {
        ImageContainerRelease(_images[i]);
    }
}

void ImageContainerCollection::add(ImageContainer* fn_nonnull image) {
    assert((_numImages < IMAGE_CONTAINER_COLLECTION_MAX_IMAGES) && "Exceeded maximum number of images");
    _images[_numImages] = ImageContainerRetain(image);
    _numImages += 1;
}

long ImageContainerCollection::getNumImages() const SWIFT_COMPUTED_PROPERTY {
    return _numImages;
}

ImageContainer* fn_nonnull ImageContainerCollection::get(long index) const SWIFT_RETURNS_UNRETAINED {
    assert((index < _numImages) && "Index out of bounds");
    return _images[index];
}


// MARK: - ImageContainer

ImageContainer::ImageContainer(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, char* fn_nonnull contents, long width, long height, long depth, LCMSColorProfile* fn_nullable colorProfile):
_referenceCounter(1),
_pixelFormat(pixelFormat),
_colorProfile(colorProfile),
_sRGB(sRGB),
_linear(linear),
_hdr(hdr),
_contents(contents),
_width(width),
_height(height),
_depth(depth) {
    //
}


ImageContainer::~ImageContainer() {
    if (_contents) {
        std::free(_contents);
    }
    
    LCMSColorProfileRelease(_colorProfile);
    
    //printf("Byeee\n");
}


ImageContainer* fn_nonnull ImageContainer::create(const char* fn_nonnull contents, long width, long height, ImagePixelFormat pixelFormat) SWIFT_RETURNS_RETAINED {
    auto size = width * height * pixelFormat.getSize();
    auto buffer = new char[size];
    std::memcpy(buffer, contents, size);
    
    return new ImageContainer(pixelFormat,
                              false, true, false,
                              buffer, width, height, 1,
                              nullptr);
}


ImageContainer* fn_nonnull ImageContainer::create(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, long width, long height, long depth, LCMSColorProfile* fn_nullable colorProfile) {
    width = std::max(1l, width);
    height = std::max(1l, height);
    depth = std::max(1l, depth);
    auto contents = new char [width * height * depth * pixelFormat.getSize()];
    return new ImageContainer(pixelFormat, sRGB, linear, hdr, contents, width, height, depth, LCMSColorProfileRetain(colorProfile));
}


ImageContainer* fn_nonnull ImageContainer::createRGBA8Unorm(long width, long height) {
    auto pixelFormat = ImagePixelFormat::rgba8Unorm;
    auto contentsSize = width * height * 4;
    auto contents = reinterpret_cast<char*>(std::malloc(contentsSize));
    std::memset(contents, 0xFF, contentsSize);
    
    return new ImageContainer(pixelFormat, true, true, false, contents, width, height, 1, nullptr);
}


static const char* fn_nonnull _getName(const char* fn_nonnull path) {
    auto lastOccurance = -1;
    auto index = 0;
    auto size = strnlen(path, 10000);
    while (index + 1 < size) {
        if (path[index] == '/') {
            lastOccurance = index;
        }
        
        index += 1;
    }
    
    return path + lastOccurance + 1;
}


ImageContainer* fn_nullable ImageContainer::_tryLoadTGA(const char* fn_nonnull path fn_noescape) SWIFT_RETURNS_RETAINED {
    auto error = TGAError();
    
    auto isTGA = TGAImage::isTGA(path, &error);
    if (isTGA == false) {
        // TODO: Describe error
        return nullptr;
    }
    
    auto tga = TGAImage::load(path, &error);
    if (tga == nullptr) {
        // TODO: Describe error
        return nullptr;
    }
    
    auto numComponents = tga->getNumComponents();
    auto pixelFormat = ImagePixelFormat(PixelComponentType::uint8, numComponents);
    auto sRGB = true;
    auto linear = false;
    auto hdr = false;
    auto contentsSize = tga->getSize();
    auto contents = reinterpret_cast<char*>(std::malloc(contentsSize));
    std::memcpy(contents, tga->getContents(), contentsSize);
    auto width = tga->getWidth();
    auto height = tga->getHeight();
    TGAImageRelease(tga);
    
    // Extract TGA image contents
    return new ImageContainer(pixelFormat, sRGB, linear, hdr, contents, width, height, 1, nullptr);
}


ImageContainer* fn_nullable ImageContainer::_tryLoadJPEG(const char* fn_nonnull path fn_noescape) SWIFT_RETURNS_RETAINED {
    auto isJPEG = checkIfJPEG(path);
    if (isJPEG == false) {
        return nullptr;
    }
    
    auto jpeg = JPEGImage::load(path);
    if (jpeg == nullptr) {
        return nullptr;
    }
    
    // Extract jpeg image contents
    
    return nullptr;
}


ImageContainer* fn_nullable ImageContainer::_tryLoadPNG(const char* fn_nonnull path fn_noescape) SWIFT_RETURNS_RETAINED {
    auto isPng = PNGImage::checkIfPNG(path);
    if (isPng == false) {
        return nullptr;
    }
    
    auto png = PNGImage::open(path);
    if (png == nullptr) {
        return nullptr;
    }
    
    
    auto sRGB = png->getIsSRGB();
    
    auto numComponents = png->getNumComponents();
    auto bitsPerComponent = png->getBitsPerComponent();
    if (bitsPerComponent % 8 != 0) {
        printf("Unsupported bits per component: %ld\n", bitsPerComponent);
        PNGImageRelease(png);
        return nullptr;
    }
    
    auto componentSize = bitsPerComponent / 8;
    
    auto pixelComponentType = PixelComponentType::uint8;
    switch (componentSize) {
        case 1:
            pixelComponentType = PixelComponentType::uint8;
            break;
            
        case 2:
            pixelComponentType = PixelComponentType::float16;
            break;
            
        default:
            printf("Unsupported component size: %ld\n", componentSize);
            PNGImageRelease(png);
            return nullptr;
    }
    
    auto pixelFormat = ImagePixelFormat(pixelComponentType, numComponents);
    
    // Copy iCC profile data
    LCMSColorProfile* fn_nullable colorProfile = nullptr;
    long iccProfileDataLength = png->getICCPDataLength();
    if (iccProfileDataLength) {
        colorProfile = LCMSColorProfile::create(png->getICCPData(), iccProfileDataLength);
        
        // Check the sRGB setting
        auto colorProfileIsSRGB = colorProfile->checkIsSRGB();
        if (sRGB) {
            if (colorProfileIsSRGB) {
                printf("✅ PNG file is saying that its colour profile IS sRGB, LCMSColorProfile tells the same\n");
            }
            else {
                printf("⚠️ PNG file is saying that its colour profile IS sRGB, LCMSColorProfile tells the opposite\n");
            }
        }
        else {
            if (colorProfileIsSRGB) {
                printf("⚠️ PNG file is saying that its colour profile IS NOT sRGB, LCMSColorProfile tells the opposite\n");
            }
            else {
                printf("✅ PNG file is saying that its colour profile IS NOT sRGB, LCMSColorProfile tells the same\n");
            }
        }
    }
    
    
    // Copy and convert image data
    auto width = png->getWidth();
    auto height = png->getHeight();
    auto depth = 1;
    auto contents = reinterpret_cast<char*>(std::malloc(width * height * numComponents * componentSize));
    auto uint8Contents = reinterpret_cast<uint8_t*>(contents);
    auto uint16Contents = reinterpret_cast<__fp16*>(contents);
    std::memset(contents, 0xff, width * height * numComponents * componentSize);
    
    auto pngContents = png->getContents();
    auto pngUint8Contents = reinterpret_cast<const uint8_t*>(pngContents);
    auto pngUint16Contents = reinterpret_cast<const PixelInfo<uint16_t, float>*>(pngContents);
    
    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            auto index = (y * width + x) * numComponents;
            switch (componentSize) {
                case 1: {
                    auto color = uint8Contents + index;
                    auto pngColor = pngUint8Contents + index;
                    for (auto i = 0; i < numComponents; i++) {
                        color[i] = pngColor[i];
                    }
                    break;
                }
                    
                case 2: {
                    auto color = uint16Contents + index;
                    auto pngColor = pngUint16Contents + index;
                    for (auto i = 0; i < numComponents; i++) {
                        color[i] = pngColor[i].convert(false);
                    }
                    break;
                }
                    
                default:
                    break;
            }
        }
    }
    
    // Clean up
    PNGImageRelease(png);
    
    return new ImageContainer(pixelFormat, sRGB, false, false, contents, width, height, depth, colorProfile);
}


ImageContainer* fn_nullable ImageContainer::_tryLoadOpenEXR(const char* fn_nonnull path fn_noescape) SWIFT_RETURNS_RETAINED {
    // Check if it's an EXR file
    if (IsEXR(path) != TINYEXR_SUCCESS) {
        return nullptr;
    }
    
    // Get EXR version
    EXRVersion version;
    auto result = ParseEXRVersionFromFile(&version, path);
    if (result != TINYEXR_SUCCESS) {
        printf("Could not parse EXR version\n");
        return nullptr;
    }
    
    // Get EXR header
    EXRHeader header;
    const char* err = nullptr;
    result = ParseEXRHeaderFromFile(&header, &version, path, &err);
    if (result != TINYEXR_SUCCESS) {
        if (err) {
            fprintf(stderr, "ERR : %s\n", err);
            FreeEXRErrorMessage(err);
        }
        return nullptr;
    }
    
    // Read the file
    // width * height * RGBA
    float* exrContents = nullptr;
    int width = 0;
    int height = 0;
    int depth = 1;
    result = LoadEXR(&exrContents, &width, &height, path, &err);
    if (result != TINYEXR_SUCCESS) {
        if (err) {
            fprintf(stderr, "ERR : %s\n", err);
            FreeEXRErrorMessage(err);
            FreeEXRHeader(&header);
        }
        return nullptr;
    }
    
    // Cast image data to float16
    auto imageContents = new __fp16[width * height * header.num_channels];
    auto contents = reinterpret_cast<char*>(imageContents);
    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            for (auto i = 0; i < header.num_channels; i++) {
                auto source = (y * width + x) * 4 + i;
                auto destination = (y * width + x) * header.num_channels + i;
                imageContents[destination] = exrContents[source];
            }
        }
    }
    
    // Create image container
    auto pixelFormat = ImagePixelFormat(PixelComponentType::float16, header.num_channels);
    
    // Assume Rec. 709 color profile
    auto rec709 = LCMSColorProfile::createRec709();
    
    // Create container
    auto container = new ImageContainer(pixelFormat, false, false, true, contents, width, height, depth, rec709);
    
    // Clean up
    std::free(exrContents);
    FreeEXRHeader(&header);
    
    return container;
}


ImageContainer* fn_nullable ImageContainer::load(const char* fn_nullable path fn_noescape, bool assumeSRGB, bool assumeLinear, LCMSColorProfile* fn_nullable assumedColorProfile, ImageToolsError* fn_nullable error fn_noescape) {
    // Get image name
    auto imageName = _getName(path);
    
    // Try to load as a TGA image
    {
        auto tga = _tryLoadTGA(path);
        if (tga) {
            //tga->_sRGBToLinear(true);
            //tga->_linearToSRGB(true);
            printf("Image \"%s\" is loaded using FastTGA - %ld bytes per component\n", imageName, tga->_pixelFormat.getComponentSize());
            return tga;
        }
    }
    
    // Try to load as a JPEG image
    {
        auto jpeg = _tryLoadJPEG(path);
        if (jpeg) {
            printf("Image \"%s\" is loaded using JPEGTurbo - %ld bytes per component\n", imageName, jpeg->_pixelFormat.getComponentSize());
            return jpeg;
        }
    }
    
    // Try to load as a PNG image
    {
        auto png = _tryLoadPNG(path);
        if (png) {
            printf("Image \"%s\" is loaded using LibPNG - %ld bytes per component\n", imageName, png->_pixelFormat.getComponentSize());
            return png;
        }
    }
    
    // Try to load as an OpenEXR image
    {
        auto exr = _tryLoadOpenEXR(path);
        if (exr) {
            printf("Image \"%s\" is loaded using tinyexr - %ld bytes per component\n", imageName, exr->_pixelFormat.getComponentSize());
            return exr;
        }
    }
    
    // Assumptions
    auto sRGB = assumeSRGB;
    auto linear = assumeLinear;
    auto isHdr = false;
    
    // Use stb_image to try to load the image
    auto is16Bit = stbi_is_16_bit(path);
    isHdr = stbi_is_hdr(path);
    if (isHdr) {
        sRGB = false;
        is16Bit = true;
    }
    
    int width = 0;
    int height = 0;
    int numComponents = 0;
    auto components = stbi_loadf(path, &width, &height, &numComponents, 0);
    if (components == nullptr) {
        auto reason = stbi_failure_reason();
        if (reason) {
            ImageToolsError::set(error, reason);
        }
        else {
            ImageToolsError::set(error, "Could not open image for some unknown reason");
        }
        return nullptr;
    }
    
    auto pixelFormat = ImagePixelFormat(is16Bit ? PixelComponentType::float16 : PixelComponentType::uint8, numComponents);
    
    // Copy pixel information
    auto contentsSize = width * height * numComponents * (is16Bit ? 2 : 1);
    auto contents = reinterpret_cast<char*>(std::malloc(contentsSize));
    std::memset(contents, 0xff, contentsSize);
    auto ucharComponents = reinterpret_cast<unsigned char*>(contents);
    auto halfComponents = reinterpret_cast<__fp16*>(contents);
    for (auto y = 0; y < height; y++) {
        for (auto x = 0; x < width; x++) {
            auto index = (y * width + x) * numComponents;
            auto rgba = components + index;
            
            for (auto i = 0; i < numComponents; i++) {
                auto value = rgba[i];
                //value = pow(value, 1 / 2.2);
                if (is16Bit) {
                    halfComponents[index + i] = static_cast<__fp16>(value);
                }
                else {
                    ucharComponents[index + i] = static_cast<unsigned char>(255.0f * value);
                }
            }
        }
    }
    
    
    stbi_image_free(components);
    
    // Take the assumed colour profile if specified, since stb_image does not provide it
    LCMSColorProfile* fn_nullable colorProfile = LCMSColorProfileRetain(assumedColorProfile);
    
    // Override linear setting if colourProfile is presented
    if (colorProfile) {
        linear = colorProfile->checkIsLinear();
    }
    
    printf("Image \"%s\" is loaded using stb_image - %ld bytes per component\n", imageName, pixelFormat.getComponentSize());
    return new ImageContainer(pixelFormat, sRGB, linear, isHdr, contents, width, height, 1, colorProfile);
}


void ImageContainer::_assignColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    // Same colour profile
    if (_colorProfile == colorProfile) {
        return;
    }
    
    // Release old colour profile
    LCMSColorProfileRelease(_colorProfile);
    
    // Assign new colour profile
    _colorProfile = LCMSColorProfileRetain(colorProfile);
    
    // Check if colour profile is sRGB
    if (_colorProfile) {
        _sRGB = _colorProfile->checkIsSRGB();
    }
    
    // Check if colour profile is linear
    if (_colorProfile) {
        _linear = _colorProfile->checkIsLinear();
    }
}


bool ImageContainer::_convertColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    // Same colour profile
    if (_colorProfile == colorProfile) {
        return true;
    }
    
    ::_convertColorProfile(colorProfile, _colorProfile, _width, _height, _contents, _pixelFormat, _hdr);
    
    // Apply colour profile
    _assignColorProfile(colorProfile);
    
    // Success
    return true;
}


void ImageContainer::_setComponentType(PixelComponentType componentType) {
    // Calculate size for the new buffer
    //auto sourcePixelSize = _pixelFormat.getSize();
    auto destinationPixelSize = _pixelFormat.numComponents * getPixelComponentTypeSize(componentType);
    auto newSize = _width * _height * _depth * destinationPixelSize;
    auto newContents = reinterpret_cast<char*>(std::malloc(newSize));
    //memset(newContents, 0xFF, newSize);
    
    union Content {
        char* contents;
        uint8_t* uint8;
        __fp16* float16;
        float* float32;
    };
    Content src = { .contents = _contents };
    Content dst = { .contents = newContents };
    
    if (_pixelFormat.componentType == PixelComponentType::uint8 && componentType == PixelComponentType::float16) {
        // uint8 to float16
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.uint8 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.float16 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    *dstValue = uint8Table[*srcValue].fp16Value;
                    //*dstValue = static_cast<__fp16>(*srcValue) / 255;
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    else if (_pixelFormat.componentType == PixelComponentType::uint8 && componentType == PixelComponentType::float32) {
        // uint8 to float32
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.uint8 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.float32 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    *dstValue = uint8Table[*srcValue].fp32Value;
                    //*dstValue = static_cast<float>(*srcValue) / 255;
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    
    
    else if (_pixelFormat.componentType == PixelComponentType::float16 && componentType == PixelComponentType::uint8) {
        // float16 to uint8
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.float16 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.uint8 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    const auto fpMax = static_cast<__fp16>(255);
                    *dstValue = static_cast<uint8_t>(std::min(fpMax, static_cast<__fp16>(*srcValue * 255)));
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    else if (_pixelFormat.componentType == PixelComponentType::float16 && componentType == PixelComponentType::float32) {
        // float16 to float32
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.float16 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.float32 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    *dstValue = static_cast<float>(*srcValue);
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    
    
    else if (_pixelFormat.componentType == PixelComponentType::float32 && componentType == PixelComponentType::uint8) {
        // float16 to uint8
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.float32 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.uint8 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    *dstValue = static_cast<uint8_t>(std::min(255.0f, *srcValue * 255.0f));
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    else if (_pixelFormat.componentType == PixelComponentType::float32 && componentType == PixelComponentType::float16) {
        // float16 to float32
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                auto srcValue = src.float32 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto dstValue = dst.float16 + (z * _width * _height + y * _width) * _pixelFormat.numComponents;
                auto numValues = _width * _pixelFormat.numComponents;
                for (auto x = 0; x < numValues; x++) {
                    *dstValue = static_cast<__fp16>(*srcValue);
                    srcValue += 1;
                    dstValue += 1;
                }
            } CONCURRENT_LOOP_END
        }
    }
    
    
    else {
        // General case
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                for (auto x = 0; x < _width; x++) {
                    // Read pixel as float32
                    auto pixel = _getPixel_general(x, y, z,
                                                   _width, _height, _depth,
                                                   _contents, _pixelFormat.numComponents, _pixelFormat.componentType);
                    
                    // Write pixel in target pixel format
                    _setPixel_general(pixel, x, y, z,
                                      _width, _height, _depth,
                                      newContents, _pixelFormat.numComponents, componentType);
                }
            } CONCURRENT_LOOP_END
        }
    }
    
    // Apply changes
    _pixelFormat.componentType = componentType;
    std::free(_contents);
    _contents = newContents;
}


bool ImageContainer::_setNumComponents(long numComponents, float fill, ImageToolsError* fn_nullable error fn_noescape) {
    // Check if the number of components is correct
    if (numComponents < 1 || numComponents > 4) {
        ImageToolsError::set(error, "Invalid number of components");
        return false;
    }
    
    // Don't do anything if the number of components is already matches
    if (numComponents == _pixelFormat.numComponents) {
        return true;
    }
    
    // Calculate the new size
    auto newSize = _width * _height * _depth * numComponents * _pixelFormat.getComponentSize();
    
    // Modify pixel data
    if (numComponents > _pixelFormat.numComponents) {
        // In case of increasing the number of components - reallocate memory first
        _contents = reinterpret_cast<char*>(std::realloc(_contents, newSize));
        
        // And then modify pixel data
        for (long z = _depth - 1; z >= 0; z--) {
            for (long y = _height - 1; y >= 0; y--) {
                for (long x = _width - 1; x >= 0; x--) {
                    // Get pixel data from old place
                    auto pixel = _getPixel(x, y, z, _pixelFormat.numComponents, _pixelFormat.componentType);
                    
                    // Fill new channels with the fill value
                    for (auto i = _pixelFormat.numComponents; i < numComponents; i++) {
                        pixel.contents[i] = fill;
                    }
                    
                    // Put pixel data into new space
                    _setPixel(pixel, x, y, z, numComponents, _pixelFormat.componentType);
                }
            }
        }
    }
    else {
        // In case of decreasing the number of components - modify pixel data first
        for (auto z = 0; z < _depth; z++) {
            for (auto y = 0; y < _height; y++) {
                for (auto x = 0; x < _width; x++) {
                    // Get pixel data from old place
                    auto pixel = _getPixel(x, y, z, _pixelFormat.numComponents, _pixelFormat.componentType);
                    // Put truncated pixel data into new space
                    _setPixel(pixel, x, y, z, numComponents, _pixelFormat.componentType);
                }
            }
        }
        
        // And then truncate memory
        _contents = reinterpret_cast<char*>(std::realloc(_contents, newSize));
    }
    
    // Apply changes
    _pixelFormat.numComponents = numComponents;
    
    return true;
}


ImagePixel ImageContainer::_getPixel(long x, long y, long z, long numComponents, PixelComponentType componentType) {
    return _getPixel_general(x, y, z, _width, _height, _depth, _contents, numComponents, componentType);
}


void ImageContainer::_setPixel(ImagePixel pixel, long x, long y, long z, long numComponents, PixelComponentType componentType) {
    _setPixel_general(pixel, x, y, z, _width, _height, _depth, _contents, numComponents, componentType);
}


void ImageContainer::_setPixel(ImagePixel pixel, long x, long y, long z) {
    _setPixel_general(pixel, x, y, z, _width, _height, _depth, _contents, _pixelFormat.numComponents, _pixelFormat.componentType);
}


bool ImageContainer::_setChannel(long channelIndex, ImageContainer* fn_nonnull sourceImage fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape) {
    // The same channel wants to be assigned to the same image - no effect, save time by doing nothing
    if (this == sourceImage && channelIndex == sourceChannelIndex) {
        return true;
    }
    
    // Image sizes are not equal
    if (_width != sourceImage->_width || _height != sourceImage->_height || _depth != sourceImage->_depth) {
        ImageToolsError::set(error, "Image sizes are not equal. Resize the source or destination image first to match the sizes");
        return false;
    }
    
    // Source channel index out ouf bounds
    if (channelIndex >= _pixelFormat.numComponents) {
        ImageToolsError::set(error, "Source channel index out ouf bounds");
        return false;
    }
    
    // Destination channel index out ouf bounds
    if (sourceChannelIndex >= sourceImage->_pixelFormat.numComponents) {
        ImageToolsError::set(error, "Destination channel index out ouf bounds");
        return false;
    }
    
    // Set the specified component of every pixel
    for (auto z = 0; z < _depth; z++) {
        for (auto y = 0; y < _height; y++) {
            for (auto x = 0; x < _width; x++) {
                auto destinationPixel = getPixel(x, y, z);
                auto sourcePixel = sourceImage->getPixel(x, y, z);
                destinationPixel.contents[channelIndex] = sourcePixel.contents[sourceChannelIndex];
                _setPixel(destinationPixel, x, y, z);
            }
        }
    }
    
    // Success
    return true;
}


void ImageContainer::_resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth, bool renormalize, void* fn_nullable userInfo fn_noescape, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape) {
    // Correct dimensions if wrong
    width = std::max(1l, width);
    height = std::max(1l, height);
    depth = std::max(1l, depth);
    
    // Don't do anything if the target size already equals to the original size
    if (width == _width && height == _height && depth == _depth) {
        // Notify callback
        if (progressCallback) {
            progressCallback(userInfo, 1);
        }
        
        // Resampling completed
        return;
    }
    
    // Prepare progress/error handler
    struct ProgressHandler {
        void* fn_nullable userInfo;
        ImageToolsProgressCallback fn_nullable progressCallback;
        
        long numSteps;
        long currentStep;
        long stepDistance;
        
        void notifyProgress() {
            // Don't do anything if
            if (progressCallback == nullptr) {
                return;
            }
            
            currentStep = std::min(currentStep + 1, numSteps);
            
            // Continue task
            if (currentStep % stepDistance != 0) {
                return;
            }
            
            // Notify about process milestone
            auto progress = 1.0f / static_cast<float>(numSteps) * static_cast<float>(currentStep);
            progressCallback(userInfo, progress);
        }
    };
    auto phase1Steps = _height * _depth;
    auto phase2Steps = height * _depth;
    auto phase3Steps = depth > 1 ? (height * depth) : (0);
    auto totalSteps = phase1Steps + phase2Steps + phase3Steps;
    auto progressHandler = ProgressHandler {
        .userInfo = userInfo,
        .progressCallback = progressCallback,
        .numSteps = totalSteps,
        .currentStep = 0,
        .stepDistance = totalSteps / 10
    };
        
    // Convert pixels to linear colour profile
    LCMSColorProfile* linearProfile = nullptr;
    if (_colorProfile) {
        // Check if the colour profile should be converted at all
        if (_colorProfile->checkIsLinear()) {
            //printf("Colour profile is already linear\n");
        }
        else {
            linearProfile = _colorProfile->createLinear();
            if (linearProfile == nullptr) {
                // This should never happen
                printf("Could not create linear colour profile\n");
            }
        }
    }
    else if (_sRGB) {
        // Check if the colour profile should be converted at all
        if (_linear) {
            //printf("Colour profile is already linear\n");
        }
        else {
            auto sRGBProfile = LCMSColorProfile::createSRGB();
            linearProfile = sRGBProfile->createLinear();
            if (linearProfile == nullptr) {
                // This should never happen
                printf("Could not create linear colour profile\n");
            }
            LCMSColorProfileRelease(sRGBProfile);
        }
    }
    
    // Create source image with linear color profile
    if (linearProfile != nullptr) {
        //printf("Convert colour profile to linear\n");
        _convertColorProfile(linearProfile);
    }
    
    // Calculate intermediate memory
    auto sourceSize = _width * _height * _depth * _pixelFormat.getSize();
    auto targetSize = width * height * depth * _pixelFormat.getSize();
    auto intermediateSize = width * _height * _depth * _pixelFormat.getSize();
    auto temporarySize = std::max(intermediateSize, targetSize);
    auto currentSize = sourceSize;
    
    // Expand buffer size if needed
    if (temporarySize > sourceSize) {
        currentSize = temporarySize;
        _contents = reinterpret_cast<char*>(std::realloc(_contents, temporarySize));
    }
    
    // Intermediate buffer
    auto tmpBuffer = reinterpret_cast<char*>(std::malloc(intermediateSize));
    
    // Source and destination contents for resampling passes
    auto sourceContents = _contents;
    auto destinationContents = tmpBuffer;
    
    // Prepare some often used variables for resampling passes
    auto numComponents = _pixelFormat.numComponents;
    auto componentType = _pixelFormat.componentType;
    auto scale = PixelPosition(static_cast<float>(_width) / width,
                               static_cast<float>(_height) / height,
                               static_cast<float>(_depth) / depth);
    
    // Horizontal pass
#define resample_x_func(_type_, _nc_) \
for (auto z = 0; z < _depth; z++) { \
    CONCURRENT_LOOP_START(0, _height, y) { \
        for (auto x = 0; x < width; x++) { \
            auto srcX = (x + 0.5) * scale.x - 0.5; \
            auto pixel = _sampleLanczosX_##_type_<_nc_>(srcX, y, z, quality, _width, _height, _depth, sourceContents, renormalize); \
            _setPixel_##_type_<_nc_>(pixel, x, y, z, width, _height, _depth, destinationContents); \
        } \
    } CONCURRENT_LOOP_END \
}
    //for (auto z = 0; z < _depth; z++) {
    //    CONCURRENT_LOOP_START(0, _height, y) {
    //        for (auto x = 0; x < width; x++) {
    //            auto srcX = (x + 0.5) * scale.x - 0.5;
    //            auto pixel = _sampleLanczosX_float16<4>(srcX, y, z, quality, _width, _height, _depth, sourceContents, renormalize);
    //            _setPixel_float16<4>(pixel, x, y, z, width, _height, _depth, destinationContents);
    //        }
    //    } CONCURRENT_LOOP_END
    //}
    if (componentType == PixelComponentType::float16 && numComponents == 1) { resample_x_func(float16, 1) }
    else if (componentType == PixelComponentType::float16 && numComponents == 2) { resample_x_func(float16, 2) }
    else if (componentType == PixelComponentType::float16 && numComponents == 3) { resample_x_func(float16, 3) }
    else if (componentType == PixelComponentType::float16 && numComponents == 4) { resample_x_func(float16, 4) }
    else if (componentType == PixelComponentType::float32 && numComponents == 1) { resample_x_func(float32, 1) }
    else if (componentType == PixelComponentType::float32 && numComponents == 2) { resample_x_func(float32, 2) }
    else if (componentType == PixelComponentType::float32 && numComponents == 3) { resample_x_func(float32, 3) }
    else if (componentType == PixelComponentType::float32 && numComponents == 4) { resample_x_func(float32, 4) }
    else {
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, _height, y) {
                for (auto x = 0; x < width; x++) {
                    auto srcX = (x + 0.5) * scale.x - 0.5;
                    auto pixel = _sampleLanczosX_general(srcX, y, z, quality, _width, _height, _depth, sourceContents, numComponents, componentType, renormalize);
                    _setPixel_general(pixel, x, y, z, width, _height, _depth, destinationContents, numComponents, componentType);
                }
            } CONCURRENT_LOOP_END
        }
    }
    // Prepare source and destination contents for further processing
    std::swap(sourceContents, destinationContents);
#undef resample_x_func
    
    
    // Vertical pass
#define resample_y_func(_type_, _nc_) \
for (auto z = 0; z < _depth; z++) { \
    CONCURRENT_LOOP_START(0, height, y) { \
        for (auto x = 0; x < width; x++) { \
            auto srcY = (y + 0.5) * scale.y - 0.5; \
            auto pixel = _sampleLanczosY_##_type_<_nc_>(x, srcY, z, quality, width, _height, _depth, sourceContents, renormalize); \
            _setPixel_##_type_<_nc_>(pixel, x, y, z, width, height, _depth, destinationContents); \
        } \
        progressHandler.notifyProgress(); \
    } CONCURRENT_LOOP_END \
}
    //for (auto z = 0; z < _depth; z++) {
    //    CONCURRENT_LOOP_START(0, height, y) {
    //        for (auto x = 0; x < width; x++) {
    //            auto srcY = (y + 0.5) * scale.y - 0.5;
    //            auto pixel = _sampleLanczosY_float16<4>(x, srcY, z, quality, width, _height, _depth, sourceContents, renormalize);
    //            _setPixel_float16<4>(pixel, x, y, z, width, height, _depth, destinationContents);
    //        }
    //        // Check cancellation
    //        progressHandler.notifyProgress();
    //    } CONCURRENT_LOOP_END
    //}
    if (componentType == PixelComponentType::float16 && numComponents == 1) { resample_y_func(float16, 1) }
    else if (componentType == PixelComponentType::float16 && numComponents == 2) { resample_y_func(float16, 2) }
    else if (componentType == PixelComponentType::float16 && numComponents == 3) { resample_y_func(float16, 3) }
    else if (componentType == PixelComponentType::float16 && numComponents == 4) { resample_y_func(float16, 4) }
    else if (componentType == PixelComponentType::float32 && numComponents == 1) { resample_y_func(float32, 1) }
    else if (componentType == PixelComponentType::float32 && numComponents == 2) { resample_y_func(float32, 2) }
    else if (componentType == PixelComponentType::float32 && numComponents == 3) { resample_y_func(float32, 3) }
    else if (componentType == PixelComponentType::float32 && numComponents == 4) { resample_y_func(float32, 4) }
    else {
        for (auto z = 0; z < _depth; z++) {
            CONCURRENT_LOOP_START(0, height, y) {
                for (auto x = 0; x < width; x++) {
                    auto srcY = (y + 0.5) * scale.y - 0.5;
                    auto pixel = _sampleLanczosY_general(x, srcY, z, quality, width, _height, _depth, sourceContents, numComponents, componentType, renormalize);
                    _setPixel_general(pixel, x, y, z, width, height, _depth, destinationContents, numComponents, componentType);
                }
                // Check cancellation
                progressHandler.notifyProgress();
            } CONCURRENT_LOOP_END
        }
    }
    // Prepare source and destination contents for further processing
    std::swap(sourceContents, destinationContents);
#undef resample_y_func
    
    
    // Depth pass if needed
    if (depth > 1) {
        for (auto z = 0; z < depth; z++) {
            CONCURRENT_LOOP_START(0, height, y) {
                for (auto x = 0; x < width; x++) {
                    auto srcZ = (z + 0.5) * scale.z - 0.5;
                    auto pixel = _sampleLanczosZ_general(x, y, srcZ, quality, width, height, _depth, sourceContents, numComponents, componentType, renormalize);
                    _setPixel_general(pixel, x, y, z, width, height, depth, destinationContents, numComponents, componentType);
                }
                
                // Check cancellation
                progressHandler.notifyProgress();
            } CONCURRENT_LOOP_END
        }
        // Prepare source and destination contents for further processing
        std::swap(sourceContents, destinationContents);
    }
    
    // Copy temporary buffer to original if needed
    if (_contents != sourceContents) {
        std::memcpy(_contents, sourceContents, targetSize);
    }
    
    // Apply size
    _width = width;
    _height = height;
    _depth = depth;
    
    // Reduce buffer size if needed
    if (targetSize < currentSize) {
        _contents = reinterpret_cast<char*>(std::realloc(_contents, targetSize));
    }
    
    // Convert back colour profile if needed
    if (linearProfile != nullptr) {
        //printf("Convert colour profile back to non-linear\n");
        // TODO: Report progress
        _convertColorProfile(_colorProfile);
    }
    
    // Clean up
    std::free(tmpBuffer);
    LCMSColorProfileRelease(linearProfile);
    
    // Notify callback
    if (progressCallback) {
        progressCallback(userInfo, 1);
    }
}


void ImageContainer::_sRGBToLinear(bool preserveAlpha) {
    LCMSColorProfileRelease(_colorProfile);
    _colorProfile = nullptr;
    _sRGB = false;
    _linear = true;
    
    auto numPixels = _width * _height * _depth;
    auto totalComponents = _width * _height * _depth * _pixelFormat.numComponents;
    
    if (_pixelFormat.componentType == PixelComponentType::uint8) {
        uint8_t* uintData = reinterpret_cast<uint8_t*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
#if USE_UINT8_TABLE
                uintData[0] = uint8Table[uintData[0]].linear;
                uintData[1] = uint8Table[uintData[1]].linear;
                uintData[2] = uint8Table[uintData[2]].linear;
#else
                auto r = static_cast<float>(uintData[0]) / std::numeric_limits<uint8_t>::max();
                auto g = static_cast<float>(uintData[1]) / std::numeric_limits<uint8_t>::max();
                auto b = static_cast<float>(uintData[2]) / std::numeric_limits<uint8_t>::max();
                
                r = fromSRGBToLinear(r);
                g = fromSRGBToLinear(g);
                b = fromSRGBToLinear(b);
                
                uintData[0] = static_cast<uint8_t>(std::min(255.0f, r * std::numeric_limits<uint8_t>::max()));
                uintData[1] = static_cast<uint8_t>(std::min(255.0f, g * std::numeric_limits<uint8_t>::max()));
                uintData[2] = static_cast<uint8_t>(std::min(255.0f, b * std::numeric_limits<uint8_t>::max()));
#endif
                
                uintData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
#if USE_UINT8_TABLE
                *uintData = uint8Table[*uintData].linear;
#else
                auto c = static_cast<float>(*uintData) / std::numeric_limits<uint8_t>::max();
                c = fromSRGBToLinear(c);
                *uintData = static_cast<uint8_t>(std::min(255.0f, c * std::numeric_limits<uint8_t>::max()));
#endif
                uintData += 1;
            }
        }
        
        return;
    }
    
    
    if (_pixelFormat.componentType == PixelComponentType::float16) {
        __fp16* halfData = reinterpret_cast<__fp16*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
                halfData[0] = fromSRGBToLinear(halfData[0]);
                halfData[1] = fromSRGBToLinear(halfData[1]);
                halfData[2] = fromSRGBToLinear(halfData[2]);
                halfData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
                *halfData = fromSRGBToLinear(*halfData);
                halfData += 1;
            }
        }
        
        return;
    }
    
    
    if (_pixelFormat.componentType == PixelComponentType::float32) {
        float* floatData = reinterpret_cast<float*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
                floatData[0] = fromSRGBToLinear(floatData[0]);
                floatData[1] = fromSRGBToLinear(floatData[1]);
                floatData[2] = fromSRGBToLinear(floatData[2]);
                floatData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
                *floatData = fromSRGBToLinear(*floatData);
                floatData += 1;
            }
        }
        
        return;
    }
    
    
    // General case. This actually should never happen, since every possible component type was processed earlier
    for (auto z = 0; z < _depth; z++) {
#if 0
        struct Context {
            ImageContainer* imageContainer;
            long z;
            bool preserveAlpha;
        };
        processConcurrently<Context>({
            .imageContainer = this,
            .z = z,
            .preserveAlpha = preserveAlpha
        }, 0, _height, [](Context& context, long y) {
            auto img = context.imageContainer;
            auto z = context.z;
            auto preserveAlpha = context.preserveAlpha;
            auto width = context.imageContainer->_width;
            
            for (auto x = 0; x < width; x++) {
                auto pixel = img->getPixel(x, y, z);
                pixel.r = fromSRGBToLinear(pixel.r);
                pixel.g = fromSRGBToLinear(pixel.g);
                pixel.b = fromSRGBToLinear(pixel.b);
                if (!preserveAlpha) {
                    pixel.a = fromSRGBToLinear(pixel.a);
                }
                img->_setPixel(pixel, x, y, z);
            }
        });
#else
        CONCURRENT_LOOP_START(0, _height, y) {
            for (auto x = 0; x < _width; x++) {
                auto pixel = getPixel(x, y, z);
                pixel.r = fromSRGBToLinear(pixel.r);
                pixel.g = fromSRGBToLinear(pixel.g);
                pixel.b = fromSRGBToLinear(pixel.b);
                if (!preserveAlpha) {
                    pixel.a = fromSRGBToLinear(pixel.a);
                }
                _setPixel(pixel, x, y, z);
            }
        } CONCURRENT_LOOP_END
#endif
    }
}


void ImageContainer::_linearToSRGB(bool preserveAlpha) {
    LCMSColorProfileRelease(_colorProfile);
    _colorProfile = nullptr;
    _sRGB = true;
    _linear = false;
    
    //printUInt8Table();
    
    auto numPixels = _width * _height * _depth;
    auto totalComponents = _width * _height * _depth * _pixelFormat.numComponents;
    
    if (_pixelFormat.componentType == PixelComponentType::uint8) {
        uint8_t* uintData = reinterpret_cast<uint8_t*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
#if USE_UINT8_TABLE
                uintData[0] = uint8Table[uintData[0]].srgb;
                uintData[1] = uint8Table[uintData[1]].srgb;
                uintData[2] = uint8Table[uintData[2]].srgb;
#else
                auto r = static_cast<float>(uintData[0]) / std::numeric_limits<uint8_t>::max();
                auto g = static_cast<float>(uintData[1]) / std::numeric_limits<uint8_t>::max();
                auto b = static_cast<float>(uintData[2]) / std::numeric_limits<uint8_t>::max();
                
                r = fromLinearToSRGB(r);
                g = fromLinearToSRGB(g);
                b = fromLinearToSRGB(b);
                
                uintData[0] = static_cast<uint8_t>(std::min(255.0f, r * std::numeric_limits<uint8_t>::max()));
                uintData[1] = static_cast<uint8_t>(std::min(255.0f, g * std::numeric_limits<uint8_t>::max()));
                uintData[2] = static_cast<uint8_t>(std::min(255.0f, b * std::numeric_limits<uint8_t>::max()));
#endif
                
                uintData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
#if USE_UINT8_TABLE
                *uintData = uint8Table[*uintData].srgb;
#else
                auto c = static_cast<float>(*uintData) / std::numeric_limits<uint8_t>::max();
                c = fromLinearToSRGB(c);
                *uintData = static_cast<uint8_t>(std::min(255.0f, c * std::numeric_limits<uint8_t>::max()));
#endif
                uintData += 1;
            }
        }
        
        return;
    }
    
    
    if (_pixelFormat.componentType == PixelComponentType::float16) {
        __fp16* halfData = reinterpret_cast<__fp16*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
                halfData[0] = fromLinearToSRGB(halfData[0]);
                halfData[1] = fromLinearToSRGB(halfData[1]);
                halfData[2] = fromLinearToSRGB(halfData[2]);
                halfData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
                *halfData = fromLinearToSRGB(*halfData);
                halfData += 1;
            }
        }
        
        return;
    }
    
    
    if (_pixelFormat.componentType == PixelComponentType::float32) {
        float* floatData = reinterpret_cast<float*>(_contents);
        
        if (_pixelFormat.numComponents == 4 && preserveAlpha) {
            for (auto index = 0; index < numPixels; index++) {
                floatData[0] = fromLinearToSRGB(floatData[0]);
                floatData[1] = fromLinearToSRGB(floatData[1]);
                floatData[2] = fromLinearToSRGB(floatData[2]);
                floatData += 4;
            }
        }
        else {
            for (auto index = 0; index < totalComponents; index++) {
                *floatData = fromLinearToSRGB(*floatData);
                floatData += 1;
            }
        }
        
        return;
    }
    
    // General case. This actually should never happen, since every possible component type was processed earlier
    for (auto z = 0; z < _depth; z++) {
        CONCURRENT_LOOP_START(0, _height, y) {
            for (auto x = 0; x < _width; x++) {
                auto pixel = getPixel(x, y, z);
                pixel.r = fromLinearToSRGB(pixel.r);
                pixel.g = fromLinearToSRGB(pixel.g);
                pixel.b = fromLinearToSRGB(pixel.b);
                _setPixel(pixel, x, y, z);
            }
        } CONCURRENT_LOOP_END
    }
    
}


ImagePixel ImageContainer::getPixel(long x, long y, long z) {
    return _getPixel(x, y, z, _pixelFormat.numComponents, _pixelFormat.componentType);
}


ImageContainer* fn_nonnull ImageContainer::copy() {
    // Copy contents
    auto contentsCopySize = _width * _height * _depth * _pixelFormat.getSize();
    auto contentsCopy = reinterpret_cast<char*>(std::malloc(contentsCopySize));
    std::memcpy(contentsCopy, _contents, contentsCopySize);
    
    // Retain ICC profile
    auto colorProfile = LCMSColorProfileRetain(_colorProfile);
    
    // Create a new ImageContainer instance
    return new ImageContainer(_pixelFormat, _sRGB, _linear, _hdr, contentsCopy, _width, _height, _depth, colorProfile);
}


ImageContainer* fn_nonnull ImageContainer::createPromoted(PixelComponentType componentType) {
    // Don't modify component type
    if (_pixelFormat.componentType == componentType) {
        return ImageContainerRetain(this);
    }
    
    auto imgCopy = copy();
    imgCopy->_setComponentType(componentType);
    return imgCopy;
}


ImageContainer* fn_nullable ImageContainer::createResampled(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth, bool renormalize, ImageToolsError* fn_nullable error fn_noescape, void* fn_nullable userInfo fn_noescape, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape) {
    auto imageCopy = copy();
    imageCopy->_resample(algorithm, quality, width, height, depth, renormalize, userInfo, progressCallback);
    return imageCopy;
}


ImageContainer* fn_nullable ImageContainer::createDownsampled(ResamplingAlgorithm algorithm, float quality, bool renormalize, ImageToolsError* fn_nullable error fn_noescape, void* fn_nullable userInfo fn_noescape, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape) {
    return createResampled(algorithm, quality, _width / 2, _height / 2, _depth / 2, renormalize, error, userInfo, progressCallback);
}


ImageContainer* fn_nonnull ImageContainer::createSRGBToLinearConverted(bool preserveAlpha) SWIFT_RETURNS_RETAINED {
    auto imgCopy = copy();
    imgCopy->_sRGBToLinear(preserveAlpha);
    return imgCopy;
}


ImageContainer* fn_nonnull ImageContainer::createLinearToSRGBConverted(bool preserveAlpha) SWIFT_RETURNS_RETAINED {
    auto imgCopy = copy();
    imgCopy->_linearToSRGB(preserveAlpha);
    return imgCopy;
}


long ImageContainer::calculateMipLevelCount() {
    auto widthCount = _calculateMipCount(_width);
    auto heightCount = _calculateMipCount(_height);
    auto depthCount = _calculateMipCount(_depth);
    return std::max(std::max(widthCount, heightCount), depthCount);
}


//void ImageContainer::generateCubeMap() {
//    // https://stackoverflow.com/questions/29678510/convert-21-equirectangular-panorama-to-cube-map
//}



ASTCImage* fn_nullable ImageContainer::createASTCCompressed(ASTCBlockSize blockSize, float quality, bool containsAlpha, bool ldrAlpha, bool normalMap, void* fn_nullable userInfo fn_noescape, ASTCEncoderProgressCallback fn_nullable progressCallback fn_noescape) {
    // Handle errors
    auto error = ASTCError();
    
    // Create an image to compress
    auto integerComponents = _pixelFormat.componentType == PixelComponentType::uint8;
    auto rawImage = ASTCRawImage::create(_contents, _width, _height, _depth, _pixelFormat.numComponents, _pixelFormat.getComponentSize(), integerComponents, true, _linear, _hdr, containsAlpha, ldrAlpha, normalMap, error);
    if (rawImage == nullptr) {
        printf("Could not create an ASTCRawImage: %s\n", error.getErrorMessage());
        return nullptr;
    }
    
    // Compress image
    auto astcImage = rawImage->compress(blockSize, quality, error, userInfo, progressCallback);
    if (rawImage == nullptr) {
        printf("Could not compress an ASTCRawImage: %s\n", error.getErrorMessage());
        ASTCRawImageRelease(rawImage);
        return nullptr;
    }
    
    // Clean up
    ASTCRawImageRelease(rawImage);
    
    return astcImage;
}


FN_IMPLEMENT_SWIFT_INTERFACE1(ImageContainer)
