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
    
    ImageContainer* fn_nonnull _image;
    
    ImageEditor(ImageContainer* fn_nonnull image);
    ~ImageEditor();
    
    friend ImageEditor* fn_nullable ImageEditorRetain(ImageEditor* fn_nullable editor) SWIFT_RETURNS_UNRETAINED;
    friend void ImageEditorRelease(ImageEditor* fn_nullable editor);
    
public:
    static ImageEditor* fn_nonnull create(ImageContainer* fn_nonnull image fn_noescape) SWIFT_NAME(init(_:)) SWIFT_RETURNS_RETAINED;
    static ImageEditor* fn_nonnull createConsuming(ImageContainer* fn_nonnull image) SWIFT_NAME(__createConsumingUnsafe(_:)) SWIFT_UNSAFE SWIFT_RETURNS_RETAINED;
    static ImageEditor* fn_nonnull create(ImageEditor* fn_nonnull editor fn_noescape) SWIFT_NAME(init(_:)) SWIFT_RETURNS_RETAINED;
    
    static ImageEditor* fn_nullable load(const char* fn_nullable path fn_noescape, LCMSColorProfile* fn_nullable assumedColorProfile, bool assumeSRGB, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__loadUnsafe(path:_:_:_:)) SWIFT_RETURNS_RETAINED;
    static ImageEditor* fn_nullable load(const char* fn_nullable buffer fn_noescape, long bufferSize, LCMSColorProfile* fn_nullable assumedColorProfile, bool assumeSRGB, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__loadUnsafe(buffer:size:_:_:_:)) SWIFT_RETURNS_RETAINED;
    
    /// Takes image copy for editing.
    void edit(ImageContainer* fn_nonnull image);
    
    /// Consumes image for editing.
    ///
    /// The editor takes ownership of the specified image and mutates it directly, making the image
    /// no longer thread-safe. In C++, you may still access the image on the same thread as the editor,
    /// but must not call ImageContainerRelease for the reference transferred to the editor.
    ///
    /// To keep accessing the image after the editor releases it (when destroyed or when replacing its
    /// image), call ImageContainerRetain before that release and later release your retained reference.
    /// Otherwise, do not access the image after its destruction. Retaining it only extends its lifetime;
    /// it does not make concurrent access safe.
    ///
    /// The Swift wrapper uses a consuming parameter to express this ownership transfer.
    void editConsuming(ImageContainer* fn_nonnull image) SWIFT_NAME(__editConsumingUnsafe(_:));
    
    /// Edits a copy of the specified editor's image.
    void edit(ImageEditor* fn_nonnull editor);
    
    [[nodiscard("Don't forget to release the image using the ImageContainerRelease function.")]]
    ImageContainer* fn_nonnull getImageCopy() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_RETAINED;
    
    long getWidth() SWIFT_COMPUTED_PROPERTY;
    long getHeight() SWIFT_COMPUTED_PROPERTY;
    long getDepth() SWIFT_COMPUTED_PROPERTY;
    
    bool getSRGB() const SWIFT_COMPUTED_PROPERTY;
    void setSRGB(bool value) SWIFT_COMPUTED_PROPERTY;
    
    bool getHDR() const SWIFT_COMPUTED_PROPERTY;
    void setHDR(bool value) SWIFT_COMPUTED_PROPERTY;
    
    bool getLinear() SWIFT_COMPUTED_PROPERTY;
    
    ImagePixelFormat getPixelFormat() SWIFT_COMPUTED_PROPERTY;
    void setComponentType(PixelComponentType componentType);
    bool setNumComponents(long numComponents, float fill, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setNumComponentsUnsafe(_:fill:error:));
    
    ImagePixel getPixel(long x, long y, long z = 0);
    void setPixel(ImagePixel pixel, long x, long y, long z);
    
    /// Checks if the selected channel purely consists of a single colour.
    bool checkIfFlatColor(long channel, float* fn_nullable outColor fn_noescape);
    
    ImageContainer* fn_nullable extractImageChannel(long channel) SWIFT_RETURNS_RETAINED;
    ImageEditor* fn_nullable extractEditorChannel(long channel) SWIFT_RETURNS_RETAINED;
    
    bool flipChannel(long channelIndex, ImageToolsError* fn_nullable error fn_noescape) SWIFT_NAME(__flipChannelUnsafe(_:_:));
    
    bool setChannel(long channelIndex, ImageContainer* fn_nonnull sourceImage fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setChannelUnsafe(_:_:_:_:));
    bool setChannel(long channelIndex, ImageEditor* fn_nonnull sourceEditor fn_noescape, long sourceChannelIndex, ImageToolsError* fn_nullable error fn_noescape = nullptr) SWIFT_NAME(__setChannelUnsafe(_:_:_:_:));
    
    LCMSColorProfile* fn_nullable getColorProfile() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_UNRETAINED;
    void setColorProfile(LCMSColorProfile* fn_nullable colorProfile) SWIFT_COMPUTED_PROPERTY;
    bool convertColorProfile(LCMSColorProfile* fn_nullable colorProfile);
    
    long calculateMipLevelCount();
    
    void resample(ResamplingAlgorithm algorithm, float quality, long width, long height, long depth, bool renormalize = false, void* fn_nullable userInfo fn_noescape = nullptr, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape = nullptr) SWIFT_NAME(__resampleUnsafe(_:quality:width:height:depth:renormalize:userInfo:progressCallback:));
    void downsample(ResamplingAlgorithm algorithm, float quality, bool renormalize = false, void* fn_nullable userInfo fn_noescape = nullptr, ImageToolsProgressCallback fn_nullable progressCallback fn_noescape = nullptr) SWIFT_NAME(__downsampleUnsafe(_:quality:renormalize:userInfo:progressCallback:));
    
    void sRGBToLinear(bool preserveAlpha) SWIFT_NAME(sRGBToLinear(preserveAlpha:));
    void linearToSRGB(bool preserveAlpha) SWIFT_NAME(linearToSRGB(preserveAlpha:));
    void linearToUInt8sRGB(bool preserveAlpha) SWIFT_NAME(linearToUInt8sRGB(preserveAlpha:));
}
FN_SWIFT_INTERFACE(ImageEditor)
SWIFT_SAFE;


FN_DEFINE_SWIFT_INTERFACE(ImageEditor)
