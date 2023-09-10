PAL Trace API chunk specification
=========================

API chunks in PAL trace sources encapsulate client API information. The chunk data is essentially the `TraceChunkApiInfo` struct.

`TraceChunkApiInfo` is based off of `SqttFileChunkApiInfo`. Tools and clients intending to retrieve api info from RDF files, should be aware of the RDF API chunk layout. The complete layout is below.

All offsets are relative to the chunk start. All sizes are in bytes. Structures are assumed to be "C" packed.

```c
struct TraceChunkApiInfo
{
    TraceApiType apiType;
    uint16_t     apiVersionMajor; // Major client API version
    uint16_t     apiVersionMinor; // Minor client API version
};

enum class TraceApiType : uint32
{
    DIRECTX_9  = 0,
    DIRECTX_11 = 1,
    DIRECTX_12 = 2,
    VULKAN     = 3,
    OPENGL     = 4,
    OPENCL     = 5,
    MANTLE     = 6,
    GENERIC    = 7
};
```