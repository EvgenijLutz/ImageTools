//
//  ImageTools.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 04.09.25.
//

import Foundation
import LittleCMS
@_exported import ImageToolsC


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
public extension PixelComponentType {
    /// Returns size in bytes
    var size: Int {
        getPixelComponentTypeSize(self)
    }
}


#if canImport(CoreGraphics)

import CoreGraphics


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
public extension ImageContainer {
    var cgImage: CGImage  {
        get throws {
            let contents = Data(bytes: contents, count: contentsSize)
            guard let dataProvider = CGDataProvider(data: contents as CFData) else {
                throw ImageToolsError.other("Something is wrong with image data")
            }
            
            let componentType = pixelFormat.componentType
            
            let cgColorProfile: CGColorSpace = try {
                // Check if there is iCC profile
                if let colorProfile = colorProfile?.colorSpace {
                    if hdr {
                        if let extendedColorProfile = CGColorSpaceCreateExtended(colorProfile) {
                            return extendedColorProfile
                        }
                    }
                    
                    return colorProfile
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
                    
                    let names = srgb ? sRGB : unknown
                    
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
                space: cgColorProfile,
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
