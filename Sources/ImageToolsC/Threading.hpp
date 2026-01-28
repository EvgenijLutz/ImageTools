//
//  Threading.hpp
//  ImageTools
//
//  Created by Evgenij Lutz on 28.01.26.
//

#pragma once

#include <ImageToolsC/Common.hpp>
#include <functional>

#define USE_CONCURRENT_LOOPS 1


void processConcurrentlyCommon(void* userInfo, long start, long end, void(* callback)(void* userInfo, long index));

template<typename ContextType>
void processConcurrently(ContextType& context, long start, long end, void(* callback)(ContextType& context, long index)) {
    struct TaskContext {
        ContextType* context;
        void(* callback)(ContextType& context, long index);
    };
    auto userInfo = TaskContext {
        .context = &context,
        .callback = callback
    };
    processConcurrentlyCommon(&userInfo, start, end, [](void* userInfo, long index) {
        auto taskContext = reinterpret_cast<TaskContext*>(userInfo);
        taskContext->callback(*(taskContext->context), index);
    });
}

template<typename ContextType>
void processConcurrently(ContextType&& context, long start, long end, void(* callback)(ContextType& context, long index)) {
    auto ctx = context;
    processConcurrently(ctx, start, end, callback);
}

void processConcurrently(long start, long end, std::function<void(long)> callback);

#if USE_CONCURRENT_LOOPS
#  define CONCURRENT_LOOP_START(_start, _end, _index_name) processConcurrently(_start, _end, [&](long _index_name)
#else
#  define CONCURRENT_LOOP_START(_start, _end, _index_name) for (auto _index_name = _start; _index_name < _end; _index_name++)
#endif

#if USE_CONCURRENT_LOOPS
#  define CONCURRENT_LOOP_END );
#else
#  define CONCURRENT_LOOP_END
#endif
