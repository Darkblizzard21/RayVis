PAL Trace ASIC chunk specification
=========================

ASIC chunks in PAL trace sources encapsulate device information. Typically, chunks are composed of a chunk header and a chunk data block. However, ASIC chunk does not have a chunk header and only consists of chunk data, which is essentially the `TraceChunkAsicInfo` struct.

`TraceChunkAsicInfo` is based off of `SqttFileChunkAsicInfo` and contains the actual device information. Each device(ie. device info) in the platform is considerd a separate chunk, which will be translated to the RDF-based `TraceChunkInfo` in TraceSession. The translated `TraceChunkInfo`(for each device) is written into the RDF file with a unique chunk index. Tools and clients intending to retrieve device info from RDF files, should be aware of the RDF ASIC chunk layout.

All offsets are relative to the chunk start. All sizes are in bytes. Structures are assumed to be "C" packed.

```c
struct TraceChunkAsicInfo
{
    uint64_t        shaderCoreClockFrequency;                  // Gpu core clock frequency in Hz
    uint64_t        memoryClockFrequency;                      // Memory clock frequency in Hz
    uint64_t        gpuTimestampFrequency;                     // Frequency of the gpu timestamp clock in Hz
    uint64_t        maxShaderCoreClock;                        // Maximum shader core clock frequency in Hz
    uint64_t        maxMemoryClock;                            // Maximum memory clock frequency in Hz
    int32_t         deviceId;
    int32_t         deviceRevisionId;
    int32_t         vgprsPerSimd;                              // Number of VGPRs per SIMD
    int32_t         sgprsPerSimd;                              // Number of SGPRs per SIMD
    int32_t         shaderEngines;                             // Number of shader engines
    int32_t         computeUnitPerShaderEngine;                // Number of compute units per shader engine
    int32_t         simdPerComputeUnit;                        // Number of SIMDs per compute unit
    int32_t         wavefrontsPerSimd;                         // Number of wavefronts per SIMD
    int32_t         minimumVgprAlloc;                          // Minimum number of VGPRs per wavefront
    int32_t         vgprAllocGranularity;                      // Allocation granularity of VGPRs
    int32_t         minimumSgprAlloc;                          // Minimum number of SGPRs per wavefront
    int32_t         sgprAllocGranularity;                      // Allocation granularity of SGPRs
    int32_t         hardwareContexts;                          // Number of hardware contexts
    TraceGpuType    gpuType;
    TraceGfxIpLevel gfxIpLevel;
    int32_t         gpuIndex;
    int32_t         ceRamSize;                                 // Max size in bytes of CE RAM space available
    int32_t         ceRamSizeGraphics;                         // Max CE RAM size available to graphics engine in bytes
    int32_t         ceRamSizeCompute;                          // Max CE RAM size available to Compute engine in bytes
    int32_t         maxNumberOfDedicatedCus;                   // Number of CUs dedicated to real time audio queue
    int64_t         vramSize;                                  // Total number of bytes to VRAM
    int32_t         vramBusWidth;                              // Width of the bus to VRAM
    int32_t         l2CacheSize;                               // Total number of bytes in L2 Cache
    int32_t         l1CacheSize;                               // Total number of L1 cache bytes per CU
    int32_t         ldsSize;                                   // Total number of LDS bytes per CU
    char            gpuName[TRACE_GPU_NAME_MAX_SIZE];          // Name of the GPU, padded to 256 bytes
    float           aluPerClock;                               // Number of ALUs per clock
    float           texturePerClock;                           // Number of texture per clock
    float           primsPerClock;                             // Number of primitives per clock
    float           pixelsPerClock;                            // Number of pixels per clock
    uint32_t        memoryOpsPerClock;                         // Number of memory operations per memory clock cycle
    TraceMemoryType memoryChipType;
    uint32_t        ldsGranularity;                            // LDS allocation granularity expressed in bytes
    uint16_t        cuMask[TRACE_MAX_NUM_SE][TRACE_SA_PER_SE]; // Mask of present, non-harvested CUs (physical layout)
};
```

* `deviceRevisionId` is a HW-specific value differentiating between different SKUs or revisions.  Corresponds to one of the PRID_* revision IDs.
* `gpuType` determines the the type of GPU (Eg. Discrete, Integrated etc.). `TraceGpuType` is similar to the `SqttGpuType` and `Pal::GpuType` enums.

    ```c
    enum class TraceGpuType : uint32
    {
        Unknown,
        Integrated,
        Discrete,
        Virtual
    };
    ```
* `gfxIpLevel` specifies the graphics IP level. `TraceGfxIpLevel` struct defines the major, minor and stepping versions (Eg. Hainan is 6.0.2).

    ```c
    struct TraceGfxIpLevel
    {
        Pal::uint16 major;
        Pal::uint16 minor;
        Pal::uint16 stepping;
    };
    ```
* `hardwareContexts` is the number of distinct state contexts available for graphics workloads. It is mostly irrelevant to clients, but might be useful to tools.
* `gpuName` is the (null-terminated) name of the GPU. `TRACE_GPU_NAME_MAX_SIZE` is set to 256.
* `memoryChipType` is the type of memory chip used by the ASIC. `TraceMemoryType` is similar to the `SqttMemoryType` and `Pal::LocalMemoryType` enums.

    ```c
    enum class TraceMemoryType : uint32
    {
        Unknown,
        Ddr,
        Ddr2,
        Ddr3,
        Ddr4,
        Ddr5,
        Gddr3,
        Gddr4,
        Gddr5,
        Gddr6,
        Hbm,
        Hbm2,
        Hbm3,
        Lpddr4,
        Lpddr5
    };
* `cuMask` represents the mask of present, non-harvested CUs (ie. physical layout). `TRACE_MAX_NUM_SE` and `TRACE_SA_PER_SE` are set to 32 and 2 respectively.