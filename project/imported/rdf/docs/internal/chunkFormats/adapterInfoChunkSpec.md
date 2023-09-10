Memory Trace Adapter chunk specification
=========================

Adapter chunks in memory trace sources encapsulate device information. The adapter chunk does not have a chunk header and only consists of chunk data, which is essentially the `TraceAdapterInfo` struct.

`TraceAdapterInfo` contains basic info about the actual device.


```c
constexpr uint32_t kGpuNameMaxLen = 128; // GPU name max length including null terminator

/// An enumeration of the memory types.
typedef enum
{
    DD_MEMORY_TYPE_UNKNOWN = 0,
    DD_MEMORY_TYPE_DDR2,
    DD_MEMORY_TYPE_DDR3,
    DD_MEMORY_TYPE_DDR4,
    DD_MEMORY_TYPE_GDDR5,
    DD_MEMORY_TYPE_GDDR6,
    DD_MEMORY_TYPE_HBM,
    DD_MEMORY_TYPE_HBM2,
    DD_MEMORY_TYPE_HBM3,
    DD_MEMORY_TYPE_LPDDR4,
    DD_MEMORY_TYPE_LPDDR5,
    DD_MEMORY_TYPE_DDR5,
    DD_MEMORY_TYPE_COUNT
} DD_MEMORY_TYPE;

struct TraceAdapterInfo
{
    /// Name of the gpu
    char name[kGpuNameMaxLen];

    /// PCI Family
    uint32_t familyId;
    /// PCI Revision
    uint32_t revisionId;
    /// PCI Device
    uint32_t deviceId;
    /// Minumum engine clock in Mhz
    uint32_t minEngineClock;
    /// Maximum engine clock in Mhz
    uint32_t maxEngineClock;
    /// Type of memory (see DD_MEMORY_TYPE)
    uint32_t memoryType;
    /// Number of memory operations per clock
    uint32_t memoryOpsPerClock;
    /// Bus width of memory interface in bits
    uint32_t memoryBusWidth;
    /// Bandwidth of memory in MB/s
    uint32_t memoryBandwidth;
    /// Minumum memory clock in Mhz
    uint32_t minMemoryClock;
    /// Minumum memory clock in Mhz
    uint32_t maxMemoryClock;
};

constexpr char kAdapterChunkId[RDF_IDENTIFIER_SIZE]  = "AdapterInfo";

```


