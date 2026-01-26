//
//  ImageEditor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#include <ImageToolsC/ImageEditor.hpp>
#include <LCMS2C/LCMS2C.hpp>


ImageEditor::ImageEditor(ImageContainer* fn_nonnull image):
_referenceCounter(1),
_image(image) {
    //
}


ImageEditor::~ImageEditor() {
    ImageContainerRelease(_image);
}


ImageEditor* fn_nonnull ImageEditor::create(ImageContainer* fn_nonnull image) {
    return new ImageEditor(ImageContainerRetain(image));
}


ImageEditor* fn_nullable ImageEditor::load(const char* fn_nullable path fn_noescape, bool assumeSRGB, bool assumeLinear, LCMSColorProfile* fn_nullable assumedColorProfile, ImageToolsError* fn_nullable error fn_noescape) SWIFT_RETURNS_RETAINED {
    auto image = ImageContainer::load(path, assumeSRGB, assumeLinear, assumedColorProfile, error);
    if (image == nullptr) {
        return nullptr;
    }
    
    // Create an editor
    auto editor = ImageEditor::create(image);
    
    // Clean up
    ImageContainerRelease(image);
    
    return editor;
}


void ImageEditor::edit(ImageContainer* fn_nonnull image) {
    if (_image == image) {
        return;
    }
    
    ImageContainerRelease(_image);
    _image = image->copy();
}


ImageContainer* fn_nonnull ImageEditor::getImageCopy() {
    return _image->copy();
}


long ImageEditor::getWidth() {
    if (_image) {
        return _image->_width;
    }
    
    return 0;
}


long ImageEditor::getHeight() {
    return _image->_height;
}


long ImageEditor::getDepth() {
    return _image->_depth;
}


bool ImageEditor::getSRGB() const {
    return _image->_sRGB;
}

void ImageEditor::setSRGB(bool value) {
    _image->_sRGB = value;
}


bool ImageEditor::getLinear() const {
    return _image->_linear;
}

void ImageEditor::setLinear(bool value) {
    _image->_linear = value;
}


bool ImageEditor::getHDR() const {
    return _image->_hdr;
}

void ImageEditor::setHDR(bool value) {
    _image->_hdr = value;
}


ImagePixelFormat ImageEditor::getPixelFormat() SWIFT_COMPUTED_PROPERTY {
    return _image->_pixelFormat;
}


void ImageEditor::setComponentType(PixelComponentType componentType) {
    _image->_setComponentType(componentType, nullptr);
}


bool ImageEditor::setNumComponents(long numComponents, float fill, ImageToolsError* fn_nullable error fn_noescape) {
    return _image->_setNumComponents(numComponents, fill, error);
}


ImagePixel ImageEditor::getPixel(long x, long y, long z) {
    return _image->getPixel(x, y, z);
}


void ImageEditor::setPixel(ImagePixel pixel, long x, long y, long z) {
    _image->_setPixel(pixel, x, y, z);
}


bool ImageEditor::setChannel(long channelIndex, ImageContainer* fn_nonnull sourceImage fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape) {
    if (sourceImage == nullptr) {
        ImageToolsError::set(error, "Source image is not set");
        return false;
    }
    
    return _image->_setChannel(channelIndex, sourceImage, sourceChannelIndex, error);
}


bool ImageEditor::setChannel(long channelIndex, ImageEditor* fn_nonnull sourceEditor fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape) {
    if (sourceEditor == nullptr || sourceEditor->_image == nullptr) {
        ImageToolsError::set(error, "Source image is not set");
        return false;
    }
    
    return _image->_setChannel(channelIndex, sourceEditor->_image, sourceChannelIndex, error);
}


LCMSColorProfile* fn_nullable ImageEditor::getColorProfile() SWIFT_COMPUTED_PROPERTY {
    return _image->_colorProfile;
}


void ImageEditor::setColorProfile(LCMSColorProfile* fn_nullable colorProfile) SWIFT_COMPUTED_PROPERTY {
    _image->_assignColourProfile(colorProfile);
}


bool ImageEditor::convertColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    return _image->_convertColourProfile(colorProfile);
}


long ImageEditor::calculateMipLevelCount() {
    return _image->calculateMipLevelCount();
}


void ImageEditor::resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth, bool renormalize, void* fn_nullable userInfo fn_noescape, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape) {
    _image->_resample(algorithm, quality, width, height, depth, renormalize, userInfo, progressCallback);
}


void ImageEditor::downsample(ResamplingAlgorithm algorithm, float quality, bool renormalize, void* fn_nullable userInfo fn_noescape, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape) {
    _image->_resample(algorithm, quality, _image->_width / 2, _image->_height / 2, _image->_depth / 2, renormalize, userInfo, progressCallback);
}


void ImageEditor::sRGBToLinear(bool preserveAlpha) {
    _image->_sRGBToLinear(preserveAlpha);
}

void ImageEditor::linearToSRGB(bool preserveAlpha) {
    _image->_linearToSRGB(preserveAlpha);
}


FN_IMPLEMENT_SWIFT_INTERFACE1(ImageEditor)
