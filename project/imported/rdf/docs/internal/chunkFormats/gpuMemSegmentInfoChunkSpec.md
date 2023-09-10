Memory Trace Gpu Mem Segment chunk specification
=========================

Segment chunks encapsulate heap information for the system.
The Segment chunk does not have a chunk header and only consists of chunk data, which is an array of `TraceHeapInfo` structs. The types and number of structs is specified by the enum DDHeapType.


```c

enum DDHeapType
{
    DD_HEAP_TYPE_LOCAL     = 0,
    DD_HEAP_TYPE_INVISIBLE = 1,
    DD_HEAP_TYPE_SYSTEM    = 2,
    DD_HEAP_TYPE_COUNT     = 3,
};

struct TraceHeapInfo
{
    DDHeapType type;
    uint64_t   physicalBaseAddress;
    uint64_t   size;
};

constexpr char kHeapChunkId[RDF_IDENTIFIER_SIZE] = "GpuMemSegment";

```
