//
//  ImageContainer.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 11.11.25.
//

import Foundation
import ImageToolsC
import ASTCEncoder


public typealias ImageContainerCallback = (_ progress: Float) -> Void

fileprivate struct CallbackContext {
    var progressCallback: ImageContainerCallback
}

@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
// Error: duplicated symbol
//@_cdecl("ImageContainerCCallback")
func imageContainerCCallback(_ userInfo: UnsafeMutableRawPointer?, _ progress: Float) -> Bool {
    userInfo?.withMemoryRebound(to: CallbackContext.self, capacity: 1) { pointer in
        pointer.pointee.progressCallback(progress)
    }
    
    return Task.isCancelled
}

@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
func withImageContainerCallback<T>(_ callback: ImageContainerCallback, action: (_ userInfo: UnsafeMutableRawPointer?) throws -> T) rethrows -> T {
    return try withoutActuallyEscaping(callback) { escapingClosure in
        var callbackContext = CallbackContext(progressCallback: escapingClosure)
        return try withUnsafeMutablePointer(to: &callbackContext) { pointer in
            try action(pointer)
        }
    }
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
public extension ImageContainer {
    /// Loads an image from a path.
    ///
    /// - Parameter path: Path to load image from.
    /// - Parameter assumeSRGB: Assume the color profile to be sRGB if it could not be determined during the image loading process.
    static func load(path: String, assumeSRGB: Bool = true, assumeLinear: Bool = false, assumedColorProfile: LCMSColorProfile? = nil) throws -> sending ImageContainer {
        var error = ImageToolsError()
        let image: ImageContainer? = path.withCString { cString in
            ImageContainer.__loadUnsafe(cString, assumeSRGB, assumeLinear, assumedColorProfile, &error)
        }
        guard let image else {
            throw error
        }
        
        return image
    }
    
    
    static func load(path: String, assumeSRGB: Bool = true, assumeLinear: Bool = false, assumedColorProfile: LCMSColorProfile? = nil) async throws -> sending ImageContainer {
        try await Task {
            return try ImageContainer.load(path: path, assumeSRGB: assumeSRGB, assumeLinear: assumeLinear, assumedColorProfile: assumedColorProfile)
        }.value
    }
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
public extension ImageContainer {
    //func createResampled() {
    //
    //}
    
    func createResampled(_ algorithm: ResamplingAlgorithm, quality: Float,
                         width: Int, height: Int, depth: Int,
                         renormalize: Bool,
                         _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ImageContainer {
        return try withImageContainerCallback(progressCallback) { userInfo in
            var error = ImageToolsError()
            let image = __createResampledUnsafe(algorithm, quality: quality, width: width, height: height, depth: depth, renormalize: renormalize, error: &error, userInfo: userInfo, progressCallback: imageContainerCCallback)
            guard let image else {
                throw error.unwrapError()
            }
            return image
        }
    }
    
    
    func createDownsampled(_ algorithm: ResamplingAlgorithm, quality: Float,
                           renormalize: Bool,
                           _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ImageContainer {
        return try withImageContainerCallback(progressCallback) { userInfo in
            var error = ImageToolsError()
            let image = __createDownsampledUnsafe(algorithm, quality: quality, renormalize: renormalize, error: &error, userInfo: userInfo, progressCallback: imageContainerCCallback)
            guard let image else {
                throw error.unwrapError()
            }
            return image
        }
    }
    
    
    func createASTCCompressed(blockSize: ASTCBlockSize,
                              quality: ASTCCompressionQuality,
                              containsAlpha: Bool,
                              ldrAlpha: Bool,
                              normalMap: Bool,
                              _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ASTCImage {
        return try withImageContainerCallback(progressCallback) { userInfo in
            let image = __createASTCCompressedUnsafe(blockSize: blockSize,
                                                     quality: quality,
                                                     containsAlpha: containsAlpha,
                                                     ldrAlpha: ldrAlpha,
                                                     normalMap: normalMap,
                                                     userInfo: userInfo,
                                                     progressCallback: imageContainerCCallback)
            
            guard let image else {
                throw ImageToolsError.other("Could not compress to ASTC image")
            }
            
            return image
        }
    }
    
}
