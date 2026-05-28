//
//  stb_image.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 03.11.25.
//

#if __has_include(<TargetConditionals.h>)
#include <TargetConditionals.h>
#endif

#if defined __APPLE__
#define STBI_NEON
#else
// No NEON :(
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
