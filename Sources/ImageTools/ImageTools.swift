//
//  ImageTools.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 04.09.25.
//

import ImageToolsC
#if canImport(CoreGraphics)
import CoreGraphics
#endif


enum ImageToolsError: Error {
    case other(_ message: String)
}


#if canImport(CoreGraphics)

public extension ImageContainer {
    var cgImage: CGImage  {
        get throws {
            throw ImageToolsError.other("Not implemented")
//            let contents = Data(bytes: data, count: dataSize)
//            guard let dataProvider = CGDataProvider(data: contents as CFData) else {
//                throw ImageToolsError.other("No data provider :(")
//            }
//            
//            //guard let colorSpace = CGColorSpace(name: CGColorSpace.extendedDisplayP3) else {
//            //guard let colorSpace = CGColorSpace(name: CGColorSpace.displayP3) else {
//            guard let colorSpace = CGColorSpace(name: CGColorSpace.sRGB) else {
//                throw ImageToolsError.other("No color space :(")
//            }
//            
//            let image = CGImage(
//                width: width,
//                height: height,
//                bitsPerComponent: componentSize * 8,
//                bitsPerPixel: componentSize * 8 * 4,
//                bytesPerRow: width * componentSize * 4,
//                space: colorSpace,
//                bitmapInfo: .init(rawValue: CGImageAlphaInfo.premultipliedLast.rawValue | CGBitmapInfo.byteOrderDefault.rawValue),
//                provider: dataProvider,
//                decode: nil,
//                shouldInterpolate: true,
//                intent: .defaultIntent
//            )
//            guard let image else {
//                throw ImageToolsError.other("Could not create CGImage")
//            }
//            
//            return image
        }
    }
}

#endif
