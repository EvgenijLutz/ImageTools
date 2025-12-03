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
    
    static ImageEditor* fn_nullable load(const char* fn_nullable path fn_noescape, bool assumeSRGB, bool assumeLinear, LCMSColorProfile* fn_nullable assumedColorProfile) SWIFT_NAME(__loadUnsafe(_:_:_:_:)) SWIFT_RETURNS_RETAINED;
    
    void edit(ImageContainer* fn_nullable image fn_noescape);
    
    [[nodiscard("Don't forget to release the image using the ImageContainerRelease function.")]]
    ImageContainer* fn_nullable getImageCopy() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_RETAINED;
    
    bool getIsSRGB() const SWIFT_COMPUTED_PROPERTY;
    void setIsSRGB(bool value) SWIFT_COMPUTED_PROPERTY;
    
    bool getIsLinear() const SWIFT_COMPUTED_PROPERTY;
    void setIsLinear(bool value) SWIFT_COMPUTED_PROPERTY;
    
    bool getIsHDR() const SWIFT_COMPUTED_PROPERTY;
    void setIsHDR(bool value) SWIFT_COMPUTED_PROPERTY;
    
    bool setNumComponents(long numComponents, float fill, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setNumComponentsUnsafe(_:fill:error:));
    
    ImagePixel getPixel(long x, long y, long z = 0);
    void setPixel(ImagePixel pixel, long x, long y, long z);
    
    bool setChannel(long channelIndex, ImageContainer* fn_nonnull sourceImage fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setChannelUnsafe(_:_:_:_:));
    bool setChannel(long channelIndex, ImageEditor* fn_nonnull sourceEditor fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setChannelUnsafe(_:_:_:_:));
    
    void setColorProfile(LCMSColorProfile* fn_nullable colorProfile);
    bool convertColorProfile(LCMSColorProfile* fn_nullable colorProfile);
    
    void resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth);
} FN_SWIFT_INTERFACE(ImageEditor);


FN_DEFINE_SWIFT_INTERFACE(ImageEditor)
