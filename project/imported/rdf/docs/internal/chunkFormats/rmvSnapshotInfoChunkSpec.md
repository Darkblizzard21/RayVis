Memory Trace Snapshot chunk specification
=========================

Snapshot chunks represent the user-defined snapshots for a trace.
The Snapshot chunk does not have a chunk header and only consists of chunk data, which is a `TraceSnapShot` struct.


```c
constexpr uint32_t kMaxSnapshotNameLen = 128; // Snapshot name length including null terminator

struct TraceSnapShot
{
    char     name[kMaxSnapshotNameLen];
    /// 64bit timestamp of the snapshot.
    uint64_t snapshotPoint;
    /// Size in bytes of the snapshot name.
    uint32_t nameLength;
    uint32_t version;
};

constexpr char kSnapshotChunkId[RDF_IDENTIFIER_SIZE] = "RmvSnapshotData";

```
