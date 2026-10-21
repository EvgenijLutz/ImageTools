# ImageTools
Gracefully load and process images to get delightful results. _Because you also deserve to be treated like a princess_ ✨
This Swift Package provides a collection of tools to load and process image data. Available on `macOS 12`, `iOS 15`, `tvOS 15`, `visionOS 1` and even `watchOS 9`.
ImageTools uses the following libraries:
- [FastTGA](https://github.com/EvgenijLutz/FastTGA) to load _.tga_ images.
- [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo) to load _JPEG_ images.
- [LibPNG](https://github.com/EvgenijLutz/LibPNG) (precompiled [libpng](https://github.com/pnggroup/libpng)) to load and save _.png_ images.
- [tinyexr](https://github.com/syoyo/tinyexr) v3.2.0 (vendored) to load _.exr_ images.
- [stb](https://github.com/nothings/stb) 's stb_image.h v2.3 (vendored) to load other commonly used image files like _.jpg_, _.png_, _.tga_, _.bmp_, _.psd_, _.gif_, _.hdr_ and _.pic_.
- [LittleCMS](https://github.com/EvgenijLutz/LittleCMS) (precompiled [Little-CMS](https://github.com/mm2/Little-CMS)) to convert between color profiles.
- [ASTCEncoder](https://github.com/EvgenijLutz/ASTCEncoder) (precompiled [astc-encoder](https://github.com/ARM-software/astc-encoder)) to convert loaded images into ASTC-compressed textures.

tinyexr is used without its zlib implementation in _miniz.c_ and _miniz.h_. Instead, ImageTools uses system provided zlib that is already linked by LibPNG.

Although both libpng and stb_image support loading _.png_ images, libpng is prioritised, since it also loads ICC profile data that gives more color accuracy and allows precise color profile conversion with LittleCMS.

Some of ImageTools code also uses [rygorous](https://gist.github.com/rygorous)'s [linear float32 <-> uint sRGB](https://gist.github.com/rygorous/2203834) components convertsion distributed under public domain. The original file was modified to merge into ImageTools APIs.

Sometimes you need to clear the SwiftPM cache at `~/Library/org.swift.swiftpm`  because some dependency tags were re-released.


## Getting started

### Prerequisites
- Swift version: Swift 6.4+
- Enabled Swift/C++ interoperability mode

### Adding as a dependency
Add the package to your Xcode project:
1. In Xcode, open **File -> Add Package Dependencies...**
2. Enter the package URL: 
```plain
https://github.com/EvgenijLutz/ImageTools.git
```
3. Choose your dependency rule and you're good to go!

Alternatively, add the package as a dependency in your `Package.swift` file:
```swift
dependencies: [
    .package(url: "https://github.com/EvgenijLutz/ImageTools.git", from: "1.0.18")
]
```

### Interface
ImageTools' interface is written in C++ and is available in the `ImageToolsC` module:
```cpp
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
At the heart of the package is `ImageContainer`, which exposes a read-only, immutable interface and is thread-safe as long as it is not consumed by an editor. `ImageEditor` mutates an `ImageContainer` and is not thread-safe.

Creating an editor from an image or calling `edit` or `create` edits a copy. Calling `editConsuming` or `createConsuming` instead transfers ownership of the original `ImageContainer` instance to the editor. The editor then mutates that instance directly, so the image is no longer thread-safe either.

In C++, you can still access the transferred image on the same thread as the editor, but must not call `ImageContainerRelease` for the reference whose ownership was transferred. To keep accessing the image after the editor releases it (when the editor is destroyed or replaces its image), call `ImageContainerRetain` before that release and later balance your retained reference with `ImageContainerRelease`. Otherwise, stop accessing the image before it is destroyed. Retaining the image extends its lifetime; it does not make concurrent access safe.

The Swift wrapper declares the image parameter of `editConsuming` as `consuming` to express the ownership transfer.

## TODOs
Include the following libraries:
- [libavif](https://github.com/aomediacodec/libavif)
- [libwebp](https://github.com/webmproject/libwebp)
- [OpenEXR](https://github.com/AcademySoftwareFoundation/openexr) or is it overkill and tinyexr is enough?
- [ImageMagick](https://github.com/ImageMagick/ImageMagick) or is it overkill and ImageTools already provides the needed interface?

## Seealso
[gitattributes overrides](https://github.com/github-linguist/linguist/blob/main/docs/overrides.md)
