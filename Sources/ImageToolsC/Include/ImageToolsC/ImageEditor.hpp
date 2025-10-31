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
    
    ImageContainer* nullable _image;
    
    ImageEditor();
    ~ImageEditor();
    
    friend ImageEditor* nullable ImageEditorRetain(ImageEditor* nullable editor) SWIFT_RETURNS_UNRETAINED;
    friend void ImageEditorRelease(ImageEditor* nullable editor);
    
public:
    static ImageEditor* nonnull create() SWIFT_NAME(init());
    
    void edit(ImageContainer* nullable image);
    
    [[nodiscard("Don't forget to release the image using the ImageContainerRelease function.")]]
    ImageContainer* nullable getImageCopy() SWIFT_COMPUTED_PROPERTY;
    
    void setICCProfileData(const char* nullable iccProfileData, long iccProfileDataLength);
    
    //void promoteToHomegeneousPixelFormat();
    
    /// Converts pixel format if possible.
    bool convertPixelFormat(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback);
    
//    /// Swizzles pixel components.
//    ///
//    /// - Returns: `false` if `targetPixelFormat`'s components do not match pixel format components of this image. `true` if succeeds.
//    bool swizzle(ImagePixelFormat targetPixelFormat, void* nullable userInfo, ImageToolsProgressCallback nullable progressCallback);
} SWIFT_SHARED_REFERENCE(ImageEditorRetain, ImageEditorRelease);
