#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <mutex>

// Resolve often hands plugins MTLBuffers with MTLStorageModePrivate. [buffer contents] is nil;
// memcpy then crashes. Use a shared staging buffer + blit when CPU access is unavailable.

namespace {

std::mutex gStagingMutex;
id<MTLBuffer> gStaging;
NSUInteger gStagingCapacity = 0;
void* gStagingDeviceId = nullptr;

// Precondition: gStagingMutex is held. Reuse or grow one staging buffer per GPU device.
static id<MTLBuffer> stagingBufferLocked(id<MTLDevice> device, NSUInteger byteCount)
{
    void* devId = (__bridge void*)device;
    if (!gStaging || gStagingDeviceId != devId || gStagingCapacity < byteCount)
    {
        id<MTLBuffer> nb = [device newBufferWithLength:byteCount options:MTLResourceStorageModeShared];
        if (!nb || !nb.contents)
            return nil;
        gStaging = nb;
        gStagingCapacity = byteCount;
        gStagingDeviceId = devId;
    }
    return gStaging;
}

// Precondition: gStagingMutex is held for the whole blit + wait.
static void copyViaStagingLocked(id<MTLCommandQueue> queue, id<MTLBuffer> deviceBuf, void* cpuData, NSUInteger byteCount, BOOL readFromGPU)
{
    id<MTLDevice> device = deviceBuf.device;
    if (!device)
        return;

    id<MTLBuffer> staging = stagingBufferLocked(device, byteCount);
    if (!staging)
        return;

    @autoreleasepool
    {
        id<MTLCommandBuffer> cmdBuf = [queue commandBuffer];
        if (!cmdBuf)
            return;

        id<MTLBlitCommandEncoder> blit = [cmdBuf blitCommandEncoder];
        if (!blit)
            return;

        if (readFromGPU)
        {
            [blit copyFromBuffer:deviceBuf sourceOffset:0 toBuffer:staging destinationOffset:0 size:byteCount];
        }
        else
        {
            memcpy(staging.contents, cpuData, byteCount);
            [blit copyFromBuffer:staging sourceOffset:0 toBuffer:deviceBuf destinationOffset:0 size:byteCount];
        }

        [blit endEncoding];
        [cmdBuf commit];
        [cmdBuf waitUntilCompleted];

        if (readFromGPU)
            memcpy(cpuData, staging.contents, byteCount);
    }
}

} // namespace

void CopyBufferFromGPUToCPU(void* cmdQueue, void* mtlBuffer, float* dstCPU, size_t byteCount)
{
    if (!mtlBuffer || !dstCPU || byteCount == 0)
        return;

    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)mtlBuffer;
    if (!buf)
        return;

    NSUInteger len = (NSUInteger)byteCount;
    if ([buf length] < len)
        return;

    MTLStorageMode mode = buf.storageMode;

    if (mode == MTLStorageModeShared || mode == MTLStorageModeManaged)
    {
        if (mode == MTLStorageModeManaged)
            [buf synchronizeResource];

        void* ptr = buf.contents;
        if (ptr)
        {
            memcpy(dstCPU, ptr, byteCount);
            return;
        }
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)cmdQueue;
    if (!queue)
        return;

    std::lock_guard<std::mutex> lock(gStagingMutex);
    copyViaStagingLocked(queue, buf, dstCPU, len, YES);
}

void CopyBufferFromCPUToGPU(void* cmdQueue, void* mtlBuffer, const float* srcCPU, size_t byteCount)
{
    if (!mtlBuffer || !srcCPU || byteCount == 0)
        return;

    id<MTLBuffer> buf = (__bridge id<MTLBuffer>)mtlBuffer;
    if (!buf)
        return;

    NSUInteger len = (NSUInteger)byteCount;
    if ([buf length] < len)
        return;

    MTLStorageMode mode = buf.storageMode;

    if (mode == MTLStorageModeShared || mode == MTLStorageModeManaged)
    {
        void* ptr = buf.contents;
        if (ptr)
        {
            memcpy(ptr, srcCPU, byteCount);
            [buf didModifyRange:NSMakeRange(0, len)];
            return;
        }
    }

    id<MTLCommandQueue> queue = (__bridge id<MTLCommandQueue>)cmdQueue;
    if (!queue)
        return;

    std::lock_guard<std::mutex> lock(gStagingMutex);
    copyViaStagingLocked(queue, buf, (void*)srcCPU, len, NO);
}
