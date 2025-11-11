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
