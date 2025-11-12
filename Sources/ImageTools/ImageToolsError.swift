//
//  ImageToolsError.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 12.11.25.
//

import ImageToolsC


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
