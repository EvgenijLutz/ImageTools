# ImageTools

Collection of tools to load and process image data. Available on `macOS 14`, `iOS 17`, `tvOS 17` and `visionOS 1`.

ImageTools uses the following libraries:
- [LibPNG](https://github.com/EvgenijLutz/LibPNG) (precompiled [libpng](https://github.com/pnggroup/libpng)) to load _.png_ images.
- [tinyexr](https://github.com/syoyo/tinyexr) v1.0.12 to load _.exr_ images.
- [stb](https://github.com/nothings/stb)'s stb_image.h to load other commonly used image files like _.jpg_, _.png_, _.tga_, _.bmp_, _.psd_, _.gif_, _.hdr_ and _.pic_.
- [LittleCMS](https://github.com/EvgenijLutz/LittleCMS) (precompiled [Little-CMS](https://github.com/mm2/Little-CMS)) to convert between color profiles.
- [ASTCEncoder](https://github.com/EvgenijLutz/ASTCEncoder) to convert loaded images into ASTC-compressed textures.

tinyexr is used without its zlib implementation in _miniz.c_ and _miniz.h_. Instead, ImageTools uses system provided zlib that is already linked by LibPNG.

Although both libpng and stb_image support loading _.png_ images, libpng is prioritised, since it also loads ICC profile data that gives more color accuracy and allows precise color profile conversion with LittleCMS.


## Integration

You can add this package directly to your Xcode project:

1. In Xcode, open **File -> Add Package Dependencies...**
2. Enter the package URL: 
```plain
https://github.com/EvgenijLutz/ImageTools.git
```
3. Choose your dependency rule and you're good to go!


## Interface

ImageTools' interface is written in C++ and is available in the `ImageToolsC` module:
```Cxx
#include <ImageToolsC/ImageToolsC.hpp>
```

ImageTools is also bridged to `Swift` using [Swift C++ interoperability](https://www.swift.org/documentation/cxx-interop/) and is available in the `ImageTools` module:
```Swift
import ImageTools
```
It extends the C++ interface by providing asynchronous interfaces and helper methods to create `CoreGraphics`' `CGImage`:
```Swift
@MainActor
var uiImage: UIImage? // or NSImage

@MainActor
func loadImage(path: String) async throws -> CGImage {
    let image = try await ImageContainer.load(path: path)
    uiImage = try .init(cgImage: image.cgImage)
}
 
```


## Architecture

In the heart of the package lays the `ImageContainer`. This object is thread-safe. 

`ImageEditor` allows you to perform mutations to a copy of an `ImageContainer`.


## TODO:
Include the following libraries:
- [OpenEXR](https://github.com/AcademySoftwareFoundation/openexr) or is it overkill and tinyexr is enough?
- [ImageMagick](https://github.com/ImageMagick/ImageMagick) ?
