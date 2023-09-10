/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <rayvis-utils/MathTypes.h>
#include <rayvis-utils/MathUtils.h>
#include <amdrdf.h>

static constexpr std::uint32_t RAY_TRACE_VERSION   = 1;
static constexpr const char*   RAY_TRACE_CHUNK_ID  = "RAYVIS_RAYTRACE";
static constexpr const char*   RAY_TRACE_EXTENSION = ".trace";

struct RayTraceHeader final {
    std::uint32_t traceId;
};

enum class RayFilter : uint8_t {
    None            = 0,
    IncludeHitRays  = 1 << 0,
    IncludeMissRays = 1 << 1,
    IncludeAllRays  = IncludeHitRays | IncludeMissRays
};

struct Ray final {
    static float missTolerance;

    std::uint32_t rayId;

    Float3 origin;     /// Ray origin
    float  tMin;       /// Ray time minimum bounds
    Float3 direction;  /// Ray direction
    float  tMax;       /// Ray time maximum bounds

    float tHit = -1;

    struct {
        std::uint32_t instanceIndex;
        std::uint32_t primitiveIndex;
        std::uint32_t geometryIndex;
    } hitInfo;

    inline bool HasHit() const
    {
        return (0 <= tHit) && (tHit < (tMax - missTolerance));
    }

    inline float tHitOrTMax() const
    {
        return HasHit() ? tHit : tMax;
    }
};

class RayTrace final {
public:
    std::string      sourcePath;
    std::uint32_t    traceId;
    std::vector<Ray> rays;

    Float2 MinMaxTHit();

    static bool LoadFrom(const char* filename, RayTrace& target, const size_t chunkIdx = 0);
    static bool LoadFrom(rdf::ChunkFile& file, RayTrace& target, const size_t chunkIdx = 0);

    bool        Save(const char* filename, bool overrideFile = true) const;
    bool        Save(rdf::ChunkFileWriter& writer) const;


    void DumpStartEndPointsToCSV(std::string path) const;

    void ScaleBy(const float& scale);
};

inline bool IncludeRay(const Ray& ray, const RayFilter filter)
{
    return ray.HasHit() ? is_flag_set<RayFilter>(RayFilter::IncludeHitRays, filter)
                        : is_flag_set(RayFilter::IncludeMissRays, filter);
}

template <>
inline std::string display_name<RayFilter>(RayFilter filter)
{
    switch (filter) {
    case RayFilter::IncludeHitRays:
        return "Only Hitting Rays";
    case RayFilter::IncludeMissRays:
        return "Only Missing Rays";
    case RayFilter::IncludeAllRays:
        return "All Rays";
    default:
        return "BAD OPTION";
    }
}