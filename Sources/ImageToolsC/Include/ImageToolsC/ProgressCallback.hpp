//
//  ProgressCallback.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 11.11.25.
//

#pragma once

#include <ImageToolsC/Common.hpp>


/// Progress callback.
///
/// Nofities about current operation's progress.
///
/// - Returns: `true` if the operation should be calcelled, otherwise `false`.
typedef bool (* ImageToolsProgressCallback)(void* fn_nullable userInfo, float progress);


enum class ImageToolsErrorCode: long {
    unknown = 0,
    taskCancelled,
    other
};


#define IMAGE_TOOLS_ERROR_MESSAGE_MAX_LENGTH 128


static inline const char* fn_nonnull imageToolsErrorCodeDescription(ImageToolsErrorCode code) {
    switch (code) {
        case ImageToolsErrorCode::unknown: return "Unknown error";
        case ImageToolsErrorCode::taskCancelled: return "Task cancelled";
        case ImageToolsErrorCode::other: return "Other error";
    }
}


struct ImageToolsError {
    ImageToolsErrorCode _code;
    char _message[IMAGE_TOOLS_ERROR_MESSAGE_MAX_LENGTH];
    
    ImageToolsError() {
        set(ImageToolsErrorCode::unknown);
    }
    
    ImageToolsError(ImageToolsErrorCode code) {
        set(code);
    }
    
    ImageToolsError(ImageToolsErrorCode code, const char* fn_nonnull message fn_noescape) {
        set(code, message);
    }
    
    void set(ImageToolsErrorCode code) {
        _code = code;
        _message[0] = 0;
    }
    
    void set(ImageToolsErrorCode code, const char* fn_nonnull message fn_noescape) {
        _code = code;
        snprintf(_message, IMAGE_TOOLS_ERROR_MESSAGE_MAX_LENGTH, "%s", message);
    }
    
    const char* fn_nonnull getMessage() const {
        return _message;
    }
    
    
    static void set(ImageToolsError* fn_nullable error fn_noescape, const char* fn_nonnull message fn_noescape) {
        if (error) {
            error->set(ImageToolsErrorCode::other, message);
        }
    }
};
