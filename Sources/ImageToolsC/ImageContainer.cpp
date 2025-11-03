//
//  ImageContainer.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 30.10.25.
//

#include <ImageToolsC/ImageContainer.hpp>
#include <LibPNGC/LibPNGC.hpp>
#include <LCMS2C/LCMS2C.hpp>


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


static char* it_nonnull copyData(const void* it_nonnull source, long size) {
    auto copy = new char[size];
    std::memcpy(copy, source, size);
    return copy;
}


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


bool ImagePixelComponent::operator == (const ImagePixelComponent& other) const {
    return channel == other.channel && type == other.type;
}


const ImagePixelFormat ImagePixelFormat::rgba8Unorm = {
    .numComponents = 4,
    .components = {
        {
            .channel = ImagePixelChannel::r,
            .type = PixelComponentType::uint8
        },
        {
            .channel = ImagePixelChannel::g,
            .type = PixelComponentType::uint8
        },
        {
            .channel = ImagePixelChannel::b,
            .type = PixelComponentType::uint8
        },
        {
            .channel = ImagePixelChannel::a,
            .type = PixelComponentType::uint8
        }
    },
        .size = 32
};


bool ImagePixelFormat::validate() const {
    // Invalid number of components
    if (numComponents < 1 || numComponents > 4) {
        return false;
    }
    
    struct Validator {
        bool valid = false;
        long bits = 0;
        bool usedChannels[4] = { false, false, false, false };
        
        /// Accumulates size and checks if the channel was already taken.
        bool accumulateSizeAndValidateChannel(ImagePixelComponent channel) {
            // Check if channel is already used
            auto channelIndex = static_cast<long>(channel.channel);
            if (usedChannels[channelIndex]) {
                valid = false;
                return true;
            }
            
            // Update used channel and pixel size
            usedChannels[channelIndex] = true;
            bits += getPixelComponentTypeSize(channel.type) * 8;
            
            // So far so good
            valid = true;
            return false;
        }
        
        bool validateSize() {
            return (valid) && (bits % 8 == 0);
        }
    };
    
    // Check occupied components
    auto validator = Validator();
    for (auto componentIndex = 0; componentIndex < numComponents; componentIndex++) {
        if (validator.accumulateSizeAndValidateChannel(components[componentIndex]) == false) {
            return false;
        }
    }
    
    // Check size
    if (validator.validateSize() == false) {
        return false;
    }
    
    
    return validator.bits / 8 == size;
}


bool ImagePixelFormat::isHomogeneous() const {
    // Invalid number of components
    if (numComponents < 1 || numComponents > 4) {
        return true;
    }
    
    // Check if all components are equal
    auto firstComponent = components[0];
    for (auto componentIndex = 1; componentIndex < numComponents; componentIndex++) {
        auto& component = components[componentIndex];
        
        // Return false if some of components are not equal
        if (firstComponent.type != component.type) {
            return false;
        }
    }
    
    // All components are equal
    return true;
}


bool ImagePixelFormat::operator == (const ImagePixelFormat& other) const {
    // Check number of components
    if (numComponents != other.numComponents) {
        return false;
    }
    
    // TODO: Unwrap loop for faster comparison
    // Compare components
    for (auto i = 0; i < numComponents; i++) {
        if (components[i] != other.components[i]) {
            return false;
        }
    }
    
    // Pixel formats are identical
    return true;
}


ImageContainer* it_nullable ImageContainerRetain(ImageContainer* it_nullable image) {
    if (image) {
        image->_referenceCounter.fetch_add(1);
    }
    
    return image;
}


void ImageContainerRelease(ImageContainer* it_nullable image) {
    if (image && image->_referenceCounter.fetch_sub(1) == 1) {
        delete image;
    }
}


ImageContainer::ImageContainer(ImagePixelFormat pixelFormat, bool sRGB, bool linear, bool hdr, char* it_nonnull contents, long width, long height, long depth, char* it_nullable iccProfileData, long iccProfileDataLength):
_referenceCounter(1),
_pixelFormat(pixelFormat),
_sRGB(sRGB),
_linear(linear),
_hdr(hdr),
_contents(contents),
_width(width),
_height(height),
_depth(depth),
_iccProfileData(iccProfileData),
_iccProfileDataLength(iccProfileDataLength) {
    //
}


ImageContainer::~ImageContainer() {
    if (_contents) {
        delete [] _contents;
    }
    
    if (_iccProfileData) {
        delete [] _iccProfileData;
    }
    
    //printf("Byeee\n");
}


ImageContainer* it_nonnull ImageContainer::rgba8Unorm(long width, long height) {
    auto pixelFormat = ImagePixelFormat::rgba8Unorm;
    auto contentsSize = width * height * 4;
    auto contents = new char[contentsSize];
    std::memset(contents, 0xFF, contentsSize);
    
    return new ImageContainer(pixelFormat, true, true, false, contents, width, height, 1, nullptr, 0);
}


ImageContainer* it_nullable ImageContainer::_tryLoadPNG(const char* it_nonnull path, bool assumeSRGB) SWIFT_RETURNS_RETAINED {
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
    
    ImagePixelFormat pixelFormat = { };
    pixelFormat.numComponents = numComponents;
    pixelFormat.size = numComponents * componentSize;
    for (auto i = 0; i < numComponents; i++) {
        pixelFormat.components[i].channel = static_cast<ImagePixelChannel>(i);
        pixelFormat.components[i].type = pixelComponentType;
    }
    
    
    // Copy iCC profile data
    char* iccProfileData = nullptr;
    long iccProfileDataLength = png->getICCPDataLength();
    if (iccProfileDataLength) {
        iccProfileData = new char[iccProfileDataLength];
        memcpy(iccProfileData, png->getICCPData(), iccProfileDataLength);
    }
    
    
    // Copy and convert image data
    auto width = png->getWidth();
    auto height = png->getHeight();
    auto depth = 1;
    auto contents = new char[width * height * numComponents * componentSize];
    auto uint8Contents = reinterpret_cast<uint8_t*>(contents);
    auto uint16Contents = reinterpret_cast<__fp16*>(contents);
    memset(contents, 0xff, width * height * numComponents * componentSize);
    
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
    
    return new ImageContainer(pixelFormat, sRGB, false, false, contents, width, height, depth, iccProfileData, iccProfileDataLength);
}


ImageContainer* it_nullable ImageContainer::_tryLoadOpenEXR(const char* it_nonnull path, bool assumeSRGB) SWIFT_RETURNS_RETAINED {
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
    auto pixelFormat = ImagePixelFormat();
    pixelFormat.numComponents = header.num_channels;
    for (auto i = 0; i < header.num_channels; i++) {
        pixelFormat.components[i].channel = static_cast<ImagePixelChannel>(i);
        pixelFormat.components[i].type = PixelComponentType::float16;
    }
    pixelFormat.size = header.num_channels * 2;
    // Assume Rec. 709 color profile
    auto rec709 = LCMSColorProfile::createRec709();
    auto profileSize = rec709->getSize();
    auto profileData = copyData(rec709->getData(), profileSize);
    
    //auto linear = rec709->createLinear();
    //auto profileSize = linear->getSize();
    //auto profileData = copyData(linear->getData(), profileSize);
    //LCMSColorProfileRelease(linear);
    
    LCMSColorProfileRelease(rec709);
    auto container = new ImageContainer(pixelFormat, false, false, true, contents, width, height, depth, profileData, profileSize);
    
    // Clean up
    free(exrContents);
    FreeEXRHeader(&header);
    
    return container;
}


ImageContainer* it_nullable ImageContainer::load(const char* it_nullable path, bool assumeSRGB) {
    // Try to load as a PNG image
    {
        auto png = _tryLoadPNG(path, assumeSRGB);
        if (png) {
            printf("Image is loaded using LibPNG\n");
            return png;
        }
    }
    
    // Try to load as an OpenEXR image
    {
        auto exr = _tryLoadOpenEXR(path, assumeSRGB);
        if (exr) {
            printf("Image is loaded using tinyexr\n");
            return exr;
        }
    }
    
    // Use stb_image to try to load the image
    auto is16Bit = stbi_is_16_bit(path);
    auto isHdr = stbi_is_hdr(path);
    if (isHdr) {
        is16Bit = true;
    }
    
    int width = 0;
    int height = 0;
    int numComponents = 0;
    auto components = stbi_loadf(path, &width, &height, &numComponents, 0);
    if (components == nullptr) {
        auto reason = stbi_failure_reason();
        if (reason) {
            printf("%s\n", reason);
        }
        else {
            printf("Could not open image for some unknown reason\n");
        }
        return nullptr;
    }
    
    ImagePixelFormat pixelFormat = {};
    pixelFormat.numComponents = numComponents;
    pixelFormat.size = numComponents * (is16Bit ? 2 : 1);
    for (auto i = 0; i < numComponents; i++) {
        pixelFormat.components[i].channel = static_cast<ImagePixelChannel>(i);
        pixelFormat.components[i].type = is16Bit ? PixelComponentType::float16 : PixelComponentType::uint8;
    }
    
    // Copy pixel information
    auto contentsSize = width * height * numComponents * (is16Bit ? 2 : 1);
    auto contents = new char[contentsSize];
    memset(contents, 0xff, contentsSize);
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
    
    printf("Image is loaded using stb_image\n");
    return new ImageContainer(pixelFormat, assumeSRGB, true, isHdr, contents, width, height, 1, nullptr, 0);
}


ImageContainer* it_nonnull ImageContainer::copy() {
    // Copy contents
    auto contentsCopySize = _width * _height * _depth * _pixelFormat.size;
    auto contentsCopy = new char[contentsCopySize];
    std::memcpy(contentsCopy, _contents, contentsCopySize);
    
    // Copy ICC profile data if presented
    char* iccProfileCopy = nullptr;
    if (_iccProfileData && _iccProfileDataLength > 0) {
        iccProfileCopy = new char[_iccProfileDataLength];
        std::memcpy(iccProfileCopy, _iccProfileData, _iccProfileDataLength);
    }
    
    // Create a new ImageContainer instance
    return new ImageContainer(_pixelFormat, _sRGB, _linear, _hdr, contentsCopy, _width, _height, _depth, iccProfileCopy, _iccProfileDataLength);
}
