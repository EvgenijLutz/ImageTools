//
//  ImageContainerC.cpp
//  ImageContainerC
//
//  Created by Evgenij Lutz on 04.09.25.
//

#include <ImageToolsC/ImageContainer.hpp>
#include <assert.h>


long getPixelComponentTypeSize(PixelComponentType type) {
    long sizes[] = {
        8,
        8
    };
    
    return sizes[static_cast<long>(type)];
}


const ImagePixelFormat ImagePixelFormat::rgba8Unorm = {
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
        .numComponents = 4,
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
            bits += getPixelComponentTypeSize(channel.type);
            
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


ImageContainer* nullable ImageContainerRetain(ImageContainer* nullable image) {
    if (image) {
        image->_referenceCounter.fetch_add(1);
    }
    
    return image;
}


void ImageContainerRelease(ImageContainer* nullable image) {
    if (image && image->_referenceCounter.fetch_sub(1) == 1) {
        delete image;
    }
}


ImageContainer::ImageContainer(ImagePixelFormat pixelFormat, char* nonnull contents, long width, long height, long depth, char* nullable iccProfileData, long iccProfileDataLength):
_referenceCounter(1),
_pixelFormat(pixelFormat),
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
}


ImageContainer* nonnull ImageContainer::rgba8Unorm(long width, long height) {
    auto pixelFormat = ImagePixelFormat::rgba8Unorm;
    auto contentsSize = width * height * 4;
    auto contents = new char[contentsSize];
    std::memset(contents, 0xFF, contentsSize);
    
    return new ImageContainer(pixelFormat, contents, width, height, 1, nullptr, 0);
}


void ImageContainer::setICCProfileData(const char* nullable iccProfileData, long iccProfileDataLength) {
    // Remove old ICC profile data
    if (_iccProfileData) {
        delete [] _iccProfileData;
        _iccProfileData = nullptr;
        _iccProfileDataLength = 0;
    }
    
    // Copy new ICC profile data if presented
    if (iccProfileData && iccProfileDataLength > 0) {
        _iccProfileData = new char[iccProfileDataLength];
        std::memcpy(_iccProfileData, iccProfileData, iccProfileDataLength);
        _iccProfileDataLength = iccProfileDataLength;
    }
}


ImageContainer* nonnull ImageContainer::copy() {
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
    return new ImageContainer(_pixelFormat, contentsCopy, _width, _height, _depth, iccProfileCopy, _iccProfileDataLength);
}


bool ImageContainer::convertPixelFormat(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback) {
    // Check if target pixel format is valid
    if (targetPixelFormat.validate() == false) {
        return false;
    }
    
    
    // Build a source table to fetch pixel data
    struct FetchStep {
        typedef double (* FetchFunction)(const char* nonnull data);
        
        long byteOffset;
        /// Up to 7 bits.
        long bitOffset;
        long bitLength;
        FetchFunction nonnull fetchFunction;
        
        /// How much bytes should be copied based on ``bitLength``.
        long copyLength;
        
        void init(long totalBitOffset, long totalBitLength, FetchFunction nonnull fetchFunction) {
            this->byteOffset = totalBitOffset / 8;
            this->bitOffset = totalBitOffset - byteOffset * 8;
            this->bitLength = totalBitLength;
            this->fetchFunction = fetchFunction;
            
            copyLength = (bitLength + 7) / 8;
        }
        
        double fetch(const char* nonnull data) {
            char componentData[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
            memcpy(componentData, data + byteOffset, copyLength);
            if (bitOffset) {
                // TODO: Also respect endianness
                assert(0 && "Bit offset is not yet supported. Implement this");
            }
            
            return fetchFunction(data);
        }
    };
    FetchStep fetchSteps[4];
    {
        long bitOffset = 0;
        
        for (auto componentIndex = 0; componentIndex < _pixelFormat.numComponents; componentIndex++) {
            auto& component = _pixelFormat.components[componentIndex];
            auto componentBitSize = getPixelComponentTypeSize(component.type);
            switch (component.type) {
                case PixelComponentType::sint8:
                    fetchSteps[componentIndex].init(bitOffset, componentBitSize, [](auto data) {
                        return static_cast<double>(data[0]) / 127.0;
                    });
                    break;
                    
                case PixelComponentType::uint8:
                    fetchSteps[componentIndex].init(bitOffset, componentBitSize, [](auto data) {
                        auto buffer = reinterpret_cast<const unsigned char*>(data);
                        return static_cast<double>(buffer[0]) / 255.0;
                    });
                    break;
            }
            
            bitOffset += componentBitSize;
        }
    }
    
    
    // Build a destination table to set pixel data
    
    
    // Prepare target buffer
    auto newContentsSize = _width * _height * _depth * targetPixelFormat.size;
    auto newContents = new char[newContentsSize];
    for (auto k = 0; k < _depth; k++) {
        for (auto j = 0; j < _height; j++) {
            for (auto i = 0; i < _width; i++) {
                // Current pixel addresses
                auto sourcePixel = _contents + (k*_height + j*_width + i)*_pixelFormat.size;
                auto destinationPixel = _contents + (k*_height + j*_width + i)*targetPixelFormat.size;
                
                // Fetch all color components
                double colorComponents[5];
                for (auto componentIndex = 0; componentIndex < _pixelFormat.numComponents; componentIndex++) {
                    auto channelIndex = static_cast<long>(_pixelFormat.components[componentIndex].channel);
                    colorComponents[channelIndex] = fetchSteps[componentIndex].fetch(sourcePixel);
                }
                
                // Copy all component data
                for (auto componentIndex = 0; componentIndex < targetPixelFormat.numComponents; componentIndex++) {
                    // TODO: Finished here
                    //writeSteps[componentIndex].write(destinationPixel, colorComponents);
                }
            }
        }
    }
    
    _pixelFormat = targetPixelFormat;
    return true;
}
