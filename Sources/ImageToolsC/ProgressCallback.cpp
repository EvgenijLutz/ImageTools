//
//  ProgressCallback.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 11.11.25.
//

#define _LIBCPP_REMOVE_TRANSITIVE_INCLUDES
#include <ImageToolsC/ProgressCallback.hpp>
#include <cstdio>


void ImageToolsError::set(ImageToolsErrorCode code, const char* fn_nonnull message fn_noescape) {
    _code = code;
    snprintf(_message, IMAGE_TOOLS_ERROR_MESSAGE_MAX_LENGTH, "%s", message);
}

const char* fn_nonnull ImageToolsError::getMessage() const {
    return _message;
}


void ImageToolsError::set(ImageToolsError* fn_nullable error fn_noescape, const char* fn_nonnull message fn_noescape) {
    if (error) {
        error->set(ImageToolsErrorCode::other, message);
    }
}
