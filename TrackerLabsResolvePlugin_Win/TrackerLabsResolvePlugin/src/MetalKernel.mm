#import <Metal/Metal.h>

#include <unordered_map>
#include <mutex>

void CopyBufferFromGPUToCPU(const float* pGPU,float* pCPU,int size)
{
    id<MTLBuffer> pGPUBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(pGPU));
    void* bufferPointer = [pGPUBuf contents];
    memcpy(pCPU, bufferPointer, size);
}
void CopyBufferFromCPUToGPU(const float* pGPU,float* pCPU,int size)
{
    id<MTLBuffer> pGPUBuf = reinterpret_cast<id<MTLBuffer> >(const_cast<float *>(pGPU));
    void* bufferPointer = [pGPUBuf contents];
    memcpy(bufferPointer,pCPU, size);
}