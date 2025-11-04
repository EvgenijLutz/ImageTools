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
    
    // Same colour profile
    if (_image->_colorProfile == colorProfile) {
        return;
    }
    
    // Release old colour profile
    LCMSColorProfileRelease(_image->_colorProfile);
    
    // Assign new colour profile
    _image->_colorProfile = LCMSColorProfileRetain(colorProfile);
}


bool ImageEditor::convertColorProfile(LCMSColorProfile* fn_nullable colorProfile) {
    // No image to edit
    if (_image == nullptr) {
        return false;
    }
    
    // Same colour profile
    if (_image->_colorProfile == colorProfile) {
        return true;
    }
    
    // Create an image for colour conversion
    auto pixelFormat = _image->_pixelFormat;
    auto cmsImage = LCMSImage::create(_image->_contents,
                                      _image->_width, _image->_height,
                                      pixelFormat.numComponents, pixelFormat.getComponentSize(),
                                      _image->_hdr,
                                      _image->_colorProfile);
    if (cmsImage == nullptr) {
        return false;
    }
    
    // Convert colour profile
    auto converted = cmsImage->convertColorProfile(colorProfile);
    if (converted == false) {
        LCMSImageRelease(cmsImage);
        return false;
    }
    
    // Apply changes
    std::memcpy(_image->_contents, cmsImage->getData(), cmsImage->getDataSize());
    setColorProfile(colorProfile);
    
    // Clean up
    LCMSImageRelease(cmsImage);
    
    return true;
}
