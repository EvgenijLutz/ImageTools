//
//  ImageEditor.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>
#include <ImageToolsC/ImageContainer.hpp>
#include <LCMS2C/LCMS2C.hpp>


/// Image editor.
///
/// - Warning: This object is not thread-safe. Access this object only from one thread at a time.
class ImageEditor final {
private:
    std::atomic<size_t> _referenceCounter;
    
    ImageContainer* fn_nullable _image;
    
    ImageEditor();
    ~ImageEditor();
    
    friend ImageEditor* fn_nullable ImageEditorRetain(ImageEditor* fn_nullable editor) SWIFT_RETURNS_UNRETAINED;
    friend void ImageEditorRelease(ImageEditor* fn_nullable editor);
    
public:
    static ImageEditor* fn_nonnull create() SWIFT_NAME(init()) SWIFT_RETURNS_RETAINED;
    
    static ImageEditor* fn_nullable load(const char* fn_nullable path, bool assumeSRGB) SWIFT_NAME(__loadUnsafe(_:_:)) SWIFT_RETURNS_RETAINED;
    
    void edit(ImageContainer* fn_nullable image);
    
    [[nodiscard("Don't forget to release the image using the ImageContainerRelease function.")]]
    ImageContainer* fn_nullable getImageCopy() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_RETAINED;
    
    void setColorProfile(LCMSColorProfile* fn_nullable colorProfile);
    bool convertColorProfile(LCMSColorProfile* fn_nullable colorProfile);
    
    void resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth);
} SWIFT_SHARED_REFERENCE(ImageEditorRetain, ImageEditorRelease);
