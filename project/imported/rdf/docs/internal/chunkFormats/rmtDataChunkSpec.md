Memory Trace RMT Data chunk specification
=========================

The RMT specification is complicated and contains the details of the data for the packet: https://amdcloud-my.sharepoint.com/:w:/g/personal/stovey_amd_com/ETYtfxn6ZSZNpJs3P3DUA0wBi7r3RKJE0HPgyhp9aP_T0g
The header for the RMT stream is provided below.


```c
typedef uint32_t ProcessId;

struct TraceRmtHeader
{
    DevDriver::ProcessId processId;
    uint32_t             threadId;
    size_t               totalDataSize;
    uint32_t             streamIndex;
    uint16_t             rmtMajorVersion;
    uint16_t             rmtMinorVersion;
};

Rmt streams are broken into multiple chunks when the stream exceeds 4MB.
The streamIndex field in the header can be used to link the separate chunks that comprise a single data stream together.

constexpr char kStreamChunkId[RDF_IDENTIFIER_SIZE]   = "RmtData";

```
