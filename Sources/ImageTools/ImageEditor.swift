//
//  ImageEditor.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 02.12.25.
//

import ImageToolsC


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
public extension ImageEditor {
    static func load(path: String, assumeSRGB: Bool = true, assumeLinear: Bool = false, assumedColorProfile: LCMSColorProfile? = nil) throws -> sending ImageEditor {
        var error = ImageToolsError()
        let image: ImageEditor? = path.withCString { cString in
            ImageEditor.__loadUnsafe(cString, assumeSRGB, assumeLinear, assumedColorProfile, &error)
        }
        guard let image else {
            throw error
        }
        
        return image
    }
    
    
    func setChannel(_ channelIndex: Int, image: ImageContainer, imageChannelIndex: Int) throws {
        var error = ImageToolsError()
        let success = __setChannelUnsafe(channelIndex, image, imageChannelIndex, &error)
        guard success else {
            throw error
        }
    }
    
    
    func setChannel(_ channelIndex: Int, editor: ImageContainer, editorImageChannelIndex: Int) throws {
        var error = ImageToolsError()
        let success = __setChannelUnsafe(channelIndex, editor, editorImageChannelIndex, &error)
        guard success else {
            throw error
        }
    }
    
    
    func setNumComponents(_ numComponents: Int, fill: Float) throws {
        var error = ImageToolsError()
        let success = __setNumComponentsUnsafe(numComponents, fill: fill, error: &error)
        guard success else {
            throw error
        }
    }
    
    
    func downsample(_ algorithm: ResamplingAlgorithm, quality: Float,
                           renormalize: Bool,
                           _ progressCallback: ImageContainerCallback = { _ in }
    ) throws {
        return try withImageContainerCallback(progressCallback) { userInfo in
            var error = ImageToolsError()
            let succeeded = __downsampleUnsafe(algorithm, quality: quality, renormalize: renormalize, error: &error, userInfo: userInfo, progressCallback: imageContainerCCallback)
            guard succeeded else {
                throw error.unwrapError()
            }
        }
    }
    
}
