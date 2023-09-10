DevDriver Event Data Chunk Specification
=========================

The chunk header, `ddEventRdfChunkHeader` contains the provider ID field which can be used to determine the types of events
which will be expected to be found in the chunk.

Individual events are structured as a shared header (`ddEventHeader`) followed by an event specific payload.

The details of the DevDriver events are provider specific. Each event provider details the events it can output in the DevDriver
registry here: https://github.amd.com/DevDriver/dd-registry/tree/main/events


```c
/// Headers for DDEvent and DDEventProvider are defined in dd_event.h
#include <events/utils/dd_event.h>

/// RDF ChunkIds for the Crash Events:
constexpr char kDevDriverEventChunkId[RDF_IDENTIFIER_SIZE] = "DDEvent";

```