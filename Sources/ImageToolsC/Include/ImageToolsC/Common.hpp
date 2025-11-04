//
//  Common.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 31.10.25.
//

#pragma once

#include <swift/bridging>
#include <atomic>
#include <assert.h>


#if !defined fn_nonnull
#define fn_nonnull __nonnull
#endif

#if !defined fn_nullable
#define fn_nullable __nullable
#endif


/// Progress callback.
///
/// Nofities about current operation's progress.
///
/// - Returns: `true` if the operation should be calcelled, otherwise `false`.
typedef bool (* ImageToolsProgressCallback)(void* fn_nullable userInfo, float progress);
