Memory Trace Snapshot Index chunk specification
=========================

The Snapshot Index chunk provides a list of indices of active snapshots contained within an RDF file.  Each index cooresponds to an index for one of the Snapshot Data chunks in the RDF file.  Since RDF files cannot be easily modified, a new Snapshot Data chunk is appended when a snapshot is added or renamed.  A new Snapshot Index chunk is appended when a snapshot is added, renamed or removed.

To summerize:
When a snapshot is added, a Snapshot Data chunk is appended to the end of the RDF file.  A new update Snapshot Index chunk is appeneded to the end of the RDF file.
When a snapshot is renamed, the renamed Snapshot Data chunk is appended to the end of the RDF file.  A new update Snapshot Index chunk is appended to the end of the RDF file.
When a snapshot is deleted, the Snapshot Index chunk is updated and appended to the end of the RDF file.

When the RDF file is loaded, only the last Snapshot Index chunk should be parsed.  All other Snapshot Index chunks should be ignored.  Once the list of Snapshot Data Chunk indices is known, the snapshots can be loaded from the RDF file.

An RDF file can be compacted by copying the RDF chunks to a new file; skipping over unused Snapshot Data chunks and Snapshot Index chunks.  Then deleting the original RDF file and renaming the compacted copy to the original filename.

The Snapshot Index chunk consists of a header followed by a payload (the list of indices).

```c
constexpr uint16_t kMaxSnapshotIndex = 1024;

struct TraceSnapShotIndexHeader
{
    uint16_t indexCount;  // The number of snapshot indices in the payload following the header.
    uint32_t version;
};

constexpr char kSnapshotChunkId[RDF_IDENTIFIER_SIZE] = "RmvSnapshotIndex";

// Payload (following the TraceSnapShotIndexHeader): a variable list of uint16_t values that represent the chunk index numbers of active snapshots (i.e. those that haven't been renamed or deleted).


```
