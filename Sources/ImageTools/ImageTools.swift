//
//  ImageTools.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 04.09.25.
//

import Foundation
@_exported import ImageToolsC


enum ImageToolsError: Error {
    case other(_ message: String)
}


public extension PixelComponentType {
    /// Returns size in bytes
    var size: Int {
        getPixelComponentTypeSize(self)
    }
}


public extension ImagePixelComponent {
    /// Returns size in bytes
    var size: Int {
        getPixelComponentTypeSize(type)
    }
}


public extension ImageContainer {
    /// Loads an image from a path.
    ///
    /// - Parameter path: Path to load image from.
    /// - Parameter assumeSRGB: Assume the color profile to be sRGB if it could not be determined during the image loading process.
    static func load(path: String, assumeSRGB: Bool = true) throws -> sending ImageContainer {
        let image: ImageContainer? = path.withCString { cString in
            ImageContainer.__loadUnsafe(cString, assumeSRGB)
        }
        
        guard let image else {
            throw ImageToolsError.other("Could not open image")
        }
        
        return image
    }
    
    
    static func load(path: String) async throws -> sending ImageContainer {
        try await Task {
            return try ImageContainer.load(path: path)
        }.value
    }
}


#if canImport(CoreGraphics)

import CoreGraphics


public extension ImageContainer {
    var cgImage: CGImage  {
        get throws {
            if pixelFormat.isHomogeneous() == false {
                throw ImageToolsError.other("Unsupported pixel format")
            }
            
            let contents = Data(bytes: contents, count: contentsSize)
            guard let dataProvider = CGDataProvider(data: contents as CFData) else {
                throw ImageToolsError.other("Something is wrong with image data")
            }
            
            let componentType = pixelFormat.components.0.type
            
            let colorProfile: CGColorSpace = try {
                // Check if there is iCC profile
                if let iccProfileData, iccProfileDataLength > 0 {
                    let iccProfileData = Data(bytes: iccProfileData, count: iccProfileDataLength)
                    if let colorProfile = CGColorSpace(iccData: iccProfileData as CFData) {
                        if hdr {
                            if let extendedColorProfile = CGColorSpaceCreateExtended(colorProfile) {
                                return extendedColorProfile
                            }
                        }
                        
                        return colorProfile
                    }
                }
                
                let colorSpaceName: CFString = {
                    struct ColorSpaceNames {
                        let linearHdr: CFString
                        let linear: CFString
                        let gammaHdr: CFString
                        let gamma: CFString
                    }
                    
                    let sRGB = ColorSpaceNames(
                        linearHdr: CGColorSpace.extendedLinearSRGB,
                        linear: CGColorSpace.linearSRGB,
                        gammaHdr: CGColorSpace.extendedSRGB,
                        gamma: CGColorSpace.sRGB
                    )
                    
                    let unknown = ColorSpaceNames(
                        linearHdr: CGColorSpace.genericRGBLinear,
                        linear: CGColorSpace.genericRGBLinear,
                        gammaHdr: CGColorSpace.genericRGBLinear,
                        gamma: CGColorSpace.genericRGBLinear
                    )
                    
                    let names = issrgb ? sRGB : unknown
                    
                    if linear {
                        if hdr {
                            return names.linearHdr
                        }
                        else {
                            return names.linear
                        }
                    }
                    else {
                        if hdr {
                            return names.gammaHdr
                        }
                        else {
                            return names.gamma
                        }
                    }
                }()
                
                guard let colorProfile = CGColorSpace(name: colorSpaceName) else {
                    throw ImageToolsError.other("No color space :(")
                }
                
                return colorProfile
            }()
            
            let numComponents = Int(pixelFormat.numComponents)
            let alphaMask: UInt32
            switch numComponents {
            case 1: alphaMask = CGImageAlphaInfo.none.rawValue
            case 2: alphaMask = CGImageAlphaInfo.premultipliedLast.rawValue
            case 3: alphaMask = CGImageAlphaInfo.none.rawValue
            case 4: alphaMask = CGImageAlphaInfo.premultipliedLast.rawValue
            default: throw ImageToolsError.other("Unsupported number of pixel components: \(numComponents)")
            }
            
            let componentMask: UInt32
            let orderMask: UInt32
            switch componentType {
            case .uint8:
                componentMask = CGImageComponentInfo.integer.rawValue
                orderMask = CGImageByteOrderInfo.orderDefault.rawValue
                
            case .float16:
                componentMask = CGImageComponentInfo.float.rawValue
                orderMask = CGImageByteOrderInfo.order16Little.rawValue
                
                
            default:
                throw ImageToolsError.other("Unknown pixel component type")
            }
            
            let componentSize = Int(pixelFormat.size) / numComponents
            
            let image = CGImage(
                width: width,
                height: height,
                bitsPerComponent: componentSize * 8,
                bitsPerPixel: componentSize * 8 * numComponents,
                bytesPerRow: width * componentSize * numComponents,
                space: colorProfile,
                bitmapInfo: .init(rawValue: componentMask | orderMask | alphaMask),
                provider: dataProvider,
                decode: nil,
                shouldInterpolate: true,
                intent: .defaultIntent
            )
            guard let image else {
                throw ImageToolsError.other("Could not create CGImage")
            }
            
            return image
        }
    }
}

#endif
