//
//  Threading.cpp
//  ImageTools
//
//  Created by Evgenij Lutz on 28.01.26.
//

#include "Threading.hpp"

#include <semaphore>
#include <thread>

#define MAX_THREADS 64

struct ThreadInfo {
    std::mutex mutex;
    bool finished;
};


// MARK: Test threading

struct TestThreadContext {
    std::atomic<long> atomic;
};

void ImageTools_testThread(ThreadInfo* threadInfo, TestThreadContext* context) {
    context->atomic.fetch_add(1);
    
    // Notify finished
    threadInfo->mutex.lock();
    threadInfo->finished = true;
    threadInfo->mutex.unlock();
}

void ImageTools_testThreadSpawning() {
    auto numCores = std::min(static_cast<long>(std::thread::hardware_concurrency()),
                             static_cast<long>(MAX_THREADS));
    printf("Num cores: %ld\n", numCores);
    
    ThreadInfo threads[MAX_THREADS];
    
    TestThreadContext context = {
        .atomic = std::atomic<long>()
    };
    
    for (auto i = 0; i < numCores; i++) {
        auto threadInfo = &(threads[i]);
        threadInfo->finished = false;
        
        std::thread thread(ImageTools_testThread, threadInfo, &context);
        thread.detach();
    }
    
    // Wait for all threads to finish
    for (auto i = 0; i < numCores; i++) {
        auto threadInfo = &(threads[i]);
        
        bool finished = false;
        do {
            threadInfo->mutex.lock();
            finished = threadInfo->finished;
            threadInfo->mutex.unlock();
        } while (finished == false);
    }
    
    printf("Result: %ld/%ld\n", context.atomic.load(), numCores);
}


// MARK: - Process concurrently

struct ConcurrentTaskContext {
    std::atomic<long> currentIndex;
    const long endIndex;
    
    void* userInfo;
    void(* callback)(void* userInfo, long index);
};

static void concurrentTaskThread(ThreadInfo* threadInfo, ConcurrentTaskContext* context) {
    auto currentIndex = &(context->currentIndex);
    auto endIndex = context->endIndex;
    auto userInfo = context->userInfo;
    auto callback = context->callback;
    
    while (true) {
        // Get task index
        auto index = currentIndex->fetch_add(1);
        
        // Finish if all tasks completed
        if (index >= endIndex) {
            break;
        }
        
        // Execute task
        callback(userInfo, index);
    }
    
    // Notify finished
    threadInfo->mutex.lock();
    threadInfo->finished = true;
    threadInfo->mutex.unlock();
}

void processConcurrentlyCommon(void* userInfo, long start, long end, void(* callback)(void* userInfo, long index)) {
    // Get number of cores
    auto numCores = std::min(static_cast<long>(std::thread::hardware_concurrency()),
                             static_cast<long>(MAX_THREADS));
    
    // Prepare thread infos
    ThreadInfo threads[MAX_THREADS];
    
    // Prepare task context
    ConcurrentTaskContext context = {
        .currentIndex = std::atomic<long>(start),
        .endIndex = end,
        .userInfo = userInfo,
        .callback = callback
    };
    
    // Spawn threads
    for (auto i = 0; i < numCores; i++) {
        auto threadInfo = &(threads[i]);
        threadInfo->finished = false;
        
        std::thread thread(concurrentTaskThread, threadInfo, &context);
        thread.detach();
    }
    
    // Wait for all threads to finish
    for (auto i = 0; i < numCores; i++) {
        auto threadInfo = &(threads[i]);
        
        bool finished = false;
        do {
            threadInfo->mutex.lock();
            finished = threadInfo->finished;
            threadInfo->mutex.unlock();
        } while (finished == false);
    }
}

void processConcurrently(long start, long end, std::function<void(long)> callback) {
    processConcurrentlyCommon(&callback, start, end, [](void* userInfo, long index) {
        auto callback = reinterpret_cast<std::function<void(long)>*>(userInfo);
        (*callback)(index);
    });
}
