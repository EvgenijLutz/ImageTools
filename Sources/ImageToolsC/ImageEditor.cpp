//
//  ImageEditor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#include <ImageToolsC/ImageEditor.hpp>
#include <LCMS2C/LCMS2C.hpp>


ImageEditor* it_nullable ImageEditorRetain(ImageEditor* it_nullable editor) {
    if (editor == nullptr) {
        return nullptr;
    }
    
    editor->_referenceCounter.fetch_add(1);
    return editor;
}


void ImageEditorRelease(ImageEditor* it_nullable editor) {
    if (editor == nullptr) {
        return;
    }
    
    if (editor->_referenceCounter.fetch_sub(1) == 1) {
        delete editor;
    }
}


ImageEditor::ImageEditor():
_referenceCounter(1),
_image(nullptr) {
    //
}


ImageEditor::~ImageEditor() {
    ImageContainerRelease(_image);
}


ImageEditor* it_nonnull ImageEditor::create() {
    return new ImageEditor();
}


ImageEditor* it_nullable ImageEditor::load(const char* it_nullable path, bool assumeSRGB) SWIFT_RETURNS_RETAINED {
    auto image = ImageContainer::load(path, assumeSRGB);
    if (image == nullptr) {
        return nullptr;
    }
    
    // Create an editor
    auto editor = ImageEditor::create();
    
    // Capture the image without needing to copy it
    editor->_image = image;
    
    return editor;
}


void ImageEditor::edit(ImageContainer* it_nullable image) {
    ImageContainerRelease(_image);
    _image = ImageContainerRetain(image);
}


ImageContainer* it_nullable ImageEditor::getImageCopy() {
    if (_image == nullptr) {
        return nullptr;
    }
    
    return _image->copy();
}


void ImageEditor::setICCProfileData(const char* it_nullable iccProfileData, long iccProfileDataLength) {
    // No image to edit
    if (_image == nullptr) {
        return;
    }
    
    // Image alias
    auto& img = *_image;
    
    // Remove old ICC profile data
    if (img._iccProfileData) {
        delete [] img._iccProfileData;
        img._iccProfileData = nullptr;
        img._iccProfileDataLength = 0;
    }
    
    // Copy new ICC profile data if presented
    if (iccProfileData && iccProfileDataLength > 0) {
        img._iccProfileData = new char[iccProfileDataLength];
        std::memcpy(img._iccProfileData, iccProfileData, iccProfileDataLength);
        img._iccProfileDataLength = iccProfileDataLength;
    }
}


static LCMSColorProfile* it_nullable createColorProfile(const char* it_nullable iccProfileData, long iccProfileDataLength) {
    if (iccProfileData) {
        return LCMSColorProfile::create(iccProfileData, iccProfileDataLength);
    }
    
    return nullptr;
}


bool ImageEditor::convertICCProfileData(const char* it_nullable iccProfileData, long iccProfileDataLength) {
    if (_image == nullptr) {
        return false;
    }
    
    auto pixelFormat = _image->_pixelFormat;
    auto sourceColorProfile = createColorProfile(_image->_iccProfileData, _image->_iccProfileDataLength);
    auto cmsImage = LCMSImage::create(_image->_contents,
                                      _image->_width, _image->_height,
                                      pixelFormat.numComponents, pixelFormat.size / pixelFormat.numComponents,
                                      _image->_hdr,
                                      sourceColorProfile);
    if (cmsImage == nullptr) {
        LCMSColorProfileRelease(sourceColorProfile);
        return false;
    }
    
    auto colorProfile = createColorProfile(iccProfileData, iccProfileDataLength);
    auto converted = cmsImage->convertColorProfile(colorProfile);
    if (converted == false) {
        LCMSColorProfileRelease(colorProfile);
        LCMSColorProfileRelease(sourceColorProfile);
        LCMSImageRelease(cmsImage);
        return false;
    }
    
    // Apply changes
    std::memcpy(_image->_contents, cmsImage->getData(), cmsImage->getDataSize());
    setICCProfileData(iccProfileData, iccProfileDataLength);
    
    // Clean up
    LCMSColorProfileRelease(colorProfile);
    LCMSColorProfileRelease(sourceColorProfile);
    LCMSImageRelease(cmsImage);
    
    return true;
}


bool ImageEditor::convertPixelFormat(ImagePixelFormat targetPixelFormat, void* it_nullable userInfo, ImageToolsProgressCallback it_nullable progressCallback) {
    // No image to edit
    if (_image == nullptr) {
        return false;
    }
    
    // Image alias
    auto& img = *_image;
    
    // Don't do anything if pixel formats are identical
    if (img._pixelFormat == targetPixelFormat) {
        return true;
    }
    
    // Check if target pixel format is valid
    if (targetPixelFormat.validate() == false) {
        return false;
    }
    
    
    // Build a source table to fetch pixel data
    struct FetchStep {
        typedef double (* FetchFunction)(const char* it_nonnull data);
        
        long byteOffset;
        /// Up to 7 bits.
        long bitOffset;
        long bitLength;
        FetchFunction it_nonnull fetchFunction;
        
        /// How much bytes should be copied based on ``bitLength``.
        long copyLength;
        
        void init(long totalBitOffset, long totalBitLength, FetchFunction it_nonnull fetchFunction) {
            this->byteOffset = totalBitOffset / 8;
            this->bitOffset = totalBitOffset - byteOffset * 8;
            this->bitLength = totalBitLength;
            this->fetchFunction = fetchFunction;
            
            copyLength = (bitLength + 7) / 8;
        }
        
        void float16(long totalBitOffset, long totalBitLength, FetchFunction it_nonnull fetchFunction) {
            this->byteOffset = totalBitOffset / 16;
            this->bitOffset = totalBitOffset - byteOffset * 16;
            this->bitLength = totalBitLength;
            this->fetchFunction = fetchFunction;
            
            copyLength = (bitLength + 15) / 16;
        }
        
        void float32(long totalBitOffset, long totalBitLength, FetchFunction it_nonnull fetchFunction) {
            this->byteOffset = totalBitOffset / 32;
            this->bitOffset = totalBitOffset - byteOffset * 32;
            this->bitLength = totalBitLength;
            this->fetchFunction = fetchFunction;
            
            copyLength = (bitLength + 31) / 32;
        }
        
        double fetch(const char* it_nonnull data) {
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
        
        for (auto componentIndex = 0; componentIndex < img._pixelFormat.numComponents; componentIndex++) {
            auto& component = img._pixelFormat.components[componentIndex];
            auto componentBitSize = getPixelComponentTypeSize(component.type) * 8;
            switch (component.type) {
                case PixelComponentType::uint8:
                    fetchSteps[componentIndex].init(bitOffset, componentBitSize, [](auto data) {
                        auto buffer = reinterpret_cast<const unsigned char*>(data);
                        return static_cast<double>(buffer[0]) / 255.0;
                    });
                    break;
                    
                case PixelComponentType::float16:
                    fetchSteps[componentIndex].float16(bitOffset, componentBitSize, [](auto data) {
                        auto buffer = reinterpret_cast<const __fp16*>(data);
                        return static_cast<double>(buffer[0]);
                    });
                    break;
                    
                case PixelComponentType::float32:
                    fetchSteps[componentIndex].float32(bitOffset, componentBitSize, [](auto data) {
                        auto buffer = reinterpret_cast<const float*>(data);
                        return static_cast<double>(buffer[0]);
                    });
                    break;
            }
            
            bitOffset += componentBitSize;
        }
    }
    
    
    // Build a destination table to set pixel data
    
    
    // Prepare target buffer
    //auto newContentsSize = _width * _height * _depth * targetPixelFormat.size;
    //auto newContents = new char[newContentsSize];
    for (auto k = 0; k < img._depth; k++) {
        for (auto j = 0; j < img._height; j++) {
            for (auto i = 0; i < img._width; i++) {
                // Current pixel addresses
                auto sourcePixel = img._contents + (k*img._height + j*img._width + i)*img._pixelFormat.size;
                //auto destinationPixel = _contents + (k*_height + j*_width + i)*targetPixelFormat.size;
                
                // Fetch all color components
                double colorComponents[5];
                for (auto componentIndex = 0; componentIndex < img._pixelFormat.numComponents; componentIndex++) {
                    auto channelIndex = static_cast<long>(img._pixelFormat.components[componentIndex].channel);
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
    
    img._pixelFormat = targetPixelFormat;
    return true;
}
