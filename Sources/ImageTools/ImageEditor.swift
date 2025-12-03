//
//  ImageEditor.swift
//  ImageTools
//
//  Created by Evgenij Lutz on 02.12.25.
//

import ImageToolsC


public extension ImageEditor {
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
    
}
