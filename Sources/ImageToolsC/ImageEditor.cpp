//
//  ImageEditor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#include <ImageToolsC/ImageEditor.hpp>
#include <LCMS2C/LCMS2C.hpp>


ImageEditor::ImageEditor():
_referenceCounter(1),
_image(nullptr) {
    //
}


ImageEditor::~ImageEditor() {
    ImageContainerRelease(_image);
}


ImageEditor* fn_nonnull ImageEditor::create() {
    return new ImageEditor();
}


ImageEditor* fn_nullable ImageEditor::load(const char* fn_nullable path fn_noescape, bool assumeSRGB, bool assumeLinear, LCMSColorProfile* fn_nullable assumedColorProfile) SWIFT_RETURNS_RETAINED {
    auto image = ImageContainer::load(path, assumeSRGB, assumeLinear, assumedColorProfile);
    if (image == nullptr) {
        return nullptr;
    }
    
    // Create an editor
    auto editor = ImageEditor::create();
    
    // Capture the image without needing to copy it
    editor->_image = image;
    
    return editor;
}


void ImageEditor::edit(ImageContainer* fn_nullable image fn_noescape) {
    ImageContainerRelease(_image);
    _image = image->copy();
}


ImageContainer* fn_nullable ImageEditor::getImageCopy() {
    if (_image == nullptr) {
        return nullptr;
    }
    
    return _image->copy();
}


bool ImageEditor::getIsSRGB() const {
    return _image ? _image->_sRGB : false;
}

void ImageEditor::setIsSRGB(bool value) {
    if (_image) {
        _image->_sRGB = value;
    }
}


bool ImageEditor::getIsLinear() const {
    return _image ? _image->_linear : false;
}

void ImageEditor::setIsLinear(bool value) {
    if (_image) {
        _image->_linear = value;
    }
}


bool ImageEditor::getIsHDR() const {
    return _image ? _image->_hdr : false;
}

void ImageEditor::setIsHDR(bool value) {
    if (_image) {
        _image->_hdr = value;
    }
}


bool ImageEditor::setNumComponents(long numComponents, float fill, ImageToolsError* fn_nullable error fn_noescape) {
    if (_image == nullptr) {
        ImageToolsError::set(error, "Image is not set");
        return false;
    }
    
    return _image->_setNumComponents(numComponents, fill, error);
}


ImagePixel ImageEditor::getPixel(long x, long y, long z) {
    if (_image) {
        return _image->getPixel(x, y, z);
    }
    
    return ImagePixel();
}


void ImageEditor::setPixel(ImagePixel pixel, long x, long y, long z) {
    if (_image) {
        _image->_setPixel(pixel, x, y, z);
    }
}


bool ImageEditor::setChannel(long channelIndex, ImageContainer* fn_nonnull sourceImage fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape) {
    if (_image == nullptr) {
        ImageToolsError::set(error, "Image is not set");
        return false;
    }
    
    if (sourceImage == nullptr) {
        ImageToolsError::set(error, "Source image is not set");
        return false;
    }
    
    return _image->_setChannel(channelIndex, sourceImage, sourceChannelIndex, error);
}


bool ImageEditor::setChannel(long channelIndex, ImageEditor* fn_nonnull sourceEditor fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape) {
    if (_image == nullptr) {
        ImageToolsError::set(error, "Image is not set");
        return false;
    }
    
    if (sourceEditor == nullptr || sourceEditor->_image == nullptr) {
        ImageToolsError::set(error, "Source image is not set");
        return false;
    }
    
    return _image->_setChannel(channelIndex, sourceEditor->_image, sourceChannelIndex, error);
}


void ImageEditor::setColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    // No image to edit
    if (_image == nullptr) {
        return;
    }
    
    // Assign new colour profile
    _image->_assignColourProfile(colorProfile);
}


bool ImageEditor::convertColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    // No image to edit
    if (_image == nullptr) {
        return false;
    }
    
    // Convert colour profile
    return _image->_convertColourProfile(colorProfile);
}


void ImageEditor::resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth) {
    auto resampled = _image->createResampled(algorithm, quality, width, height, depth);
    ImageContainerRelease(_image);
    _image = resampled;
}


FN_IMPLEMENT_SWIFT_INTERFACE1(ImageEditor)
