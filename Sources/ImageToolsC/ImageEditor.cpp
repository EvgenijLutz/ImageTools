//
//  ImageEditor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#include <ImageToolsC/ImageEditor.hpp>


ImageEditor* nullable ImageEditorRetain(ImageEditor* nullable editor) {
    if (editor == nullptr) {
        return nullptr;
    }
    
    editor->_referenceCounter.fetch_add(1);
    return editor;
}


void ImageEditorRelease(ImageEditor* nullable editor) {
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


ImageEditor* nonnull ImageEditor::create() {
    return new ImageEditor();
}


void ImageEditor::edit(ImageContainer* nullable image) {
    ImageContainerRelease(_image);
    _image = ImageContainerRetain(image);
}


ImageContainer* nullable ImageEditor::getImageCopy() {
    if (_image == nullptr) {
        return nullptr;
    }
    
    return _image->copy();
}


void ImageEditor::setICCProfileData(const char* nullable iccProfileData, long iccProfileDataLength) {
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


bool ImageEditor::convertPixelFormat(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback) {
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
        
        void float16(long totalBitOffset, long totalBitLength, FetchFunction nonnull fetchFunction) {
            this->byteOffset = totalBitOffset / 16;
            this->bitOffset = totalBitOffset - byteOffset * 16;
            this->bitLength = totalBitLength;
            this->fetchFunction = fetchFunction;
            
            copyLength = (bitLength + 15) / 16;
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
