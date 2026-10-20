//
//  Timing.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 20.10.26.
//

#include "Timing.hpp"

#if __APPLE__
#include <QuartzCore/QuartzCore.h>
#endif


double timing_get_current_media_time() {
#if __APPLE__
    return CACurrentMediaTime();
#else
    return 0
#endif
}
