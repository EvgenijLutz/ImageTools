//
//  ImageContainer.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 11.11.25.
//

import Foundation
import ImageToolsC
import ASTCEncoder


@_cdecl("ImageContainerCCallback")
fileprivate func imageContainerCCallback(_ userInfo: UnsafeMutableRawPointer?, _ progress: Float) -> Bool {
    userInfo?.withMemoryRebound(to: CallbackContext.self, capacity: 1) { pointer in
        pointer.pointee.progressCallback(progress)
    }
    
    return Task.isCancelled
}


public typealias ImageContainerCallback = (_ progress: Float) -> Void

fileprivate struct CallbackContext {
    var progressCallback: ImageContainerCallback
}

func withImageContainerCallback<T>(_ callback: ImageContainerCallback, action: (_ userInfo: UnsafeMutableRawPointer?) throws -> T) rethrows -> T {
    return try withoutActuallyEscaping(callback) { escapingClosure in
        var callbackContext = CallbackContext(progressCallback: escapingClosure)
        return try withUnsafeMutablePointer(to: &callbackContext) { pointer in
            try action(pointer)
        }
    }
}


public extension ImageContainer {
    /// Loads an image from a path.
    ///
    /// - Parameter path: Path to load image from.
    /// - Parameter assumeSRGB: Assume the color profile to be sRGB if it could not be determined during the image loading process.
    static func load(path: String, assumeSRGB: Bool = true, assumeLinear: Bool = false, assumedColorProfile: LCMSColorProfile? = nil) throws -> sending ImageContainer {
        let image: ImageContainer? = path.withCString { cString in
            ImageContainer.__loadUnsafe(cString, assumeSRGB, assumeLinear, assumedColorProfile)
        }
        
        guard let image else {
            throw ImageToolsError.other("Could not open image")
        }
        
        return image
    }
    
    
    static func load(path: String, assumeSRGB: Bool = true, assumeLinear: Bool = false, assumedColorProfile: LCMSColorProfile? = nil) async throws -> sending ImageContainer {
        try await Task {
            return try ImageContainer.load(path: path, assumeSRGB: assumeSRGB, assumeLinear: assumeLinear, assumedColorProfile: assumedColorProfile)
        }.value
    }
}


public extension ImageContainer {
    //func createResampled() {
    //
    //}
    
    func createResampled(_ algorithm: ResamplingAlgorithm, quality: Float,
                         width: Int, height: Int, depth: Int,
                         _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ImageContainer {
        return try withImageContainerCallback(progressCallback) { userInfo in
            var error = ImageToolsError()
            let image = __createResampledUnsafe(algorithm, quality: quality, width: width, height: height, depth: depth, error: &error, userInfo: userInfo, progressCallback: imageContainerCCallback)
            guard let image else {
                throw error.unwrapError()
            }
            return image
        }
    }
    
    
    func createDownsampled(_ algorithm: ResamplingAlgorithm, quality: Float,
                         _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ImageContainer {
        return try withImageContainerCallback(progressCallback) { userInfo in
            var error = ImageToolsError()
            let image = __createDownsampledUnsafe(algorithm, quality: quality, error: &error, userInfo: userInfo, progressCallback: imageContainerCCallback)
            guard let image else {
                throw error.unwrapError()
            }
            return image
        }
    }
    
    
    func createASTCCompressed(blockSize: ASTCBlockSize,
                              quality: ASTCCompressionQuality,
                              ldrAlpha: Bool,
                              _ progressCallback: ImageContainerCallback = { _ in }
    ) throws -> ASTCImage {
        return try withImageContainerCallback(progressCallback) { userInfo in
            let image = __createASTCCompressedUnsafe(blockSize: blockSize,
                                                     quality: quality,
                                                     ldrAlpha: ldrAlpha,
                                                     userInfo: userInfo,
                                                     progressCallback: imageContainerCCallback)
            
            guard let image else {
                throw ImageToolsError.other("Could not compress to ASTC image")
            }
            
            return image
        }
    }
    
}
