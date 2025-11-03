//
//  ImageEditor.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>
#include <ImageToolsC/ImageContainer.hpp>


/// Image editor.
///
/// - Warning: This object is not thread-safe. Access this object only from one thread at a time.
class ImageEditor final {
private:
    std::atomic<size_t> _referenceCounter;
    
    ImageContainer* it_nullable _image;
    
    ImageEditor();
    ~ImageEditor();
    
    friend ImageEditor* it_nullable ImageEditorRetain(ImageEditor* it_nullable editor) SWIFT_RETURNS_UNRETAINED;
    friend void ImageEditorRelease(ImageEditor* it_nullable editor);
    
public:
    static ImageEditor* it_nonnull create() SWIFT_NAME(init()) SWIFT_RETURNS_RETAINED;
    
    static ImageEditor* it_nullable load(const char* it_nullable path, bool assumeSRGB) SWIFT_NAME(__loadUnsafe(_:_:)) SWIFT_RETURNS_RETAINED;
    
    void edit(ImageContainer* it_nullable image);
    
    [[nodiscard("Don't forget to release the image using the ImageContainerRelease function.")]]
    ImageContainer* it_nullable getImageCopy() SWIFT_COMPUTED_PROPERTY SWIFT_RETURNS_RETAINED;
    
    void setICCProfileData(const char* it_nullable iccProfileData, long iccProfileDataLength);
    bool convertICCProfileData(const char* it_nullable iccProfileData, long iccProfileDataLength);
    
    //void promoteToHomegeneousPixelFormat();
    
    /// Converts pixel format if possible.
    bool convertPixelFormat(ImagePixelFormat targetPixelFormat, void* it_nullable userInfo, ImageToolsProgressCallback it_nullable progressCallback);
    
//    /// Swizzles pixel components.
//    ///
//    /// - Returns: `false` if `targetPixelFormat`'s components do not match pixel format components of this image. `true` if succeeds.
//    bool swizzle(ImagePixelFormat targetPixelFormat, void* it_nullable userInfo, ImageToolsProgressCallback it_nullable progressCallback);
} SWIFT_SHARED_REFERENCE(ImageEditorRetain, ImageEditorRelease);
