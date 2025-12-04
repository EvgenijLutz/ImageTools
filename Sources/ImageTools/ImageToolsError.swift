//
//  ImageToolsError.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 12.11.25.
//

import ImageToolsC


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
extension ImageToolsError: @retroactive Error {
    func unwrapError() -> Error {
        if _code == .taskCancelled {
            return CancellationError()
        }
        
        return self
    }
    
    static func other(_ message: String) -> ImageToolsError {
        message.withCString { cString in
            return .init(.other, cString)
        }
    }
}


@available(macOS 13.3, iOS 16.4, tvOS 16.4, watchOS 9.4, visionOS 1.0, *)
extension ImageToolsError: @retroactive CustomStringConvertible {
    public var description: String {
        let errorDescription = String(cString: imageToolsErrorCodeDescription(_code))
        let message = String(cString: getMessage())
        if message.isEmpty {
            return errorDescription
        }
        
        if _code == .other {
            return message
        }
        
        return "\(errorDescription): \(message)"
    }
}
