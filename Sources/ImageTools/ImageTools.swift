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


public extension ImageContainer {
    static func load(path: String) throws -> ImageContainer {
        let image: ImageContainer? = path.withCString { cString in
            ImageContainer.load(cString)
        }
        
        guard let image else {
            throw ImageToolsError.other("Could not open image")
        }
        
        return image
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
                throw ImageToolsError.other("No data provider :(")
            }
            
            let componentType = pixelFormat.components.0.type
            let colorSpaceName: CFString = try {
                switch componentType {
                case .sint8, .uint8:
                    if linear {
                        if hdr {
                            return CGColorSpace.extendedLinearSRGB
                        }
                        else {
                            return CGColorSpace.linearSRGB
                        }
                    }
                    else {
                        if hdr {
                            return CGColorSpace.extendedSRGB
                        }
                        else {
                            return CGColorSpace.sRGB
                        }
                    }
                    
                case .float16:
                    if linear {
                        if hdr {
                            return CGColorSpace.extendedLinearDisplayP3
                        }
                        else {
                            return CGColorSpace.linearDisplayP3
                        }
                    }
                    else {
                        if hdr {
                            return CGColorSpace.extendedDisplayP3
                        }
                        else {
                            return CGColorSpace.displayP3
                        }
                    }
                    
                default:
                    throw ImageToolsError.other("Unknown component type: \(componentType.rawValue)")
                }
            }()
            
            guard let colorSpace = CGColorSpace(name: colorSpaceName) else {
                throw ImageToolsError.other("No color space :(")
            }
            
            let numComponents = Int(pixelFormat.numComponents)
            let alphaMask: UInt32
            switch numComponents {
            case 1:
                alphaMask = CGImageAlphaInfo.none.rawValue
                
            case 2:
                alphaMask = CGImageAlphaInfo.premultipliedLast.rawValue
                
            case 3:
                alphaMask = CGImageAlphaInfo.none.rawValue
                
            case 4:
                alphaMask = CGImageAlphaInfo.premultipliedLast.rawValue
                
            default:
                throw ImageToolsError.other("Unsupported number of pixel components: \(numComponents)")
            }
            
            let componentMask: UInt32
            let orderMask: UInt32
            switch componentType {
            case .sint8, .uint8:
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
                space: colorSpace,
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
