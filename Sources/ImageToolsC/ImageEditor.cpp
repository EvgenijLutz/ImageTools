//
//  ImageEditor.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#include <ImageToolsC/ImageEditor.hpp>
#include <LCMS2C/LCMS2C.hpp>


ImageEditor* fn_nullable ImageEditorRetain(ImageEditor* fn_nullable editor) {
    if (editor == nullptr) {
        return nullptr;
    }
    
    editor->_referenceCounter.fetch_add(1);
    return editor;
}


void ImageEditorRelease(ImageEditor* fn_nullable editor) {
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


ImageEditor* fn_nonnull ImageEditor::create() {
    return new ImageEditor();
}


ImageEditor* fn_nullable ImageEditor::load(const char* fn_nullable path, bool assumeSRGB) SWIFT_RETURNS_RETAINED {
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


void ImageEditor::edit(ImageContainer* fn_nullable image) {
    ImageContainerRelease(_image);
    _image = ImageContainerRetain(image);
}


ImageContainer* fn_nullable ImageEditor::getImageCopy() {
    if (_image == nullptr) {
        return nullptr;
    }
    
    return _image->copy();
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
