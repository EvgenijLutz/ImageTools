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
func imageContainerCCallback(_ userInfo: UnsafeMutableRawPointer?, _ progress: Float) -> Bool {
    userInfo?.withMemoryRebound(to: CallbackContext.self, capacity: 1) { pointer in
        pointer.pointee.progressCallback(progress)
    }
    
    return Task.isCancelled
}


fileprivate struct CallbackContext {
    var progressCallback: (Float) -> Void
}

public typealias ImageContainerCallback = (_ progress: Float) -> Void

func withImageContainerCallback<T>(_ callback: ImageContainerCallback, action: (_ userInfo: UnsafeMutableRawPointer?) throws -> T) rethrows -> T {
    return try withoutActuallyEscaping(callback) { escapingClosure in
        var callbackContext = CallbackContext(progressCallback: escapingClosure)
        return try withUnsafeMutablePointer(to: &callbackContext) { pointer in
            try action(pointer)
        }
    }
}


public extension ImageContainer {
    //func createResampled() {
    //
    //}
    
    
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
