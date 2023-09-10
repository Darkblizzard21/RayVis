#include "Common.hlsl"
#include "Colors.hlsl"
#include "RaytracingAlogrithm.hlsl"

struct VolumeData
{
    float3 min;
    float chunkCount;
    float3 max;
    float cellSize;
    float chunkSize;
};

cbuffer constants : register(b0)
{
    Camera camera;
    float baseTransparency;
    int2 viewportDimensions;
    int samplesPerCell;
    bool excludeExceeding;
    VolumeData volumeData;
    float elapsed;
    float minAccumulateValue;
    float maxAccumulateValue;
    bool enableChunkDebugging;
    int maxDebugChunkCount;
}

Texture2D<float> rayDepth : register(t0);
Buffer<float3> chunkMinMaxData : register(t1);

Texture3D<float> volumes[512] : register(t0, space1);

SamplerState vSampler : register(s0);


RWTexture2D<float4> output : register(u0);


float2 TraceVolume(float3 origin, float3 direction, float maxT, float3 minBounds, float3 maxBounds, Texture3D<float> volume)
{
    float2 minMaxT = IntersectAABB(origin, direction, minBounds, maxBounds);
    minMaxT = float2(max(0, min(minMaxT.x, maxT)), min(minMaxT.y, maxT));
    
    if ((minMaxT.y < minMaxT.x) || (maxT <= minMaxT.x) || (minMaxT.y < 0))
    {
        return float2(0.0, 0.0);
    }

    
    int maxSteps = (volumeData.chunkSize * SQR3) * samplesPerCell;
    
    float stepSize = volumeData.cellSize / samplesPerCell;
    float stepT = stepSize / length(direction);
    
    float startT = ceil(minMaxT.x / stepT) * stepT;
    int steps = min(ceil((minMaxT.y - startT) / stepT) + samplesPerCell, maxSteps);
    
    float weight = 1.f / samplesPerCell;
    
    float3 stepVector = direction * stepT;
    float3 samplePoint = (origin + direction * startT) - minBounds;
    float3 extends = maxBounds - minBounds;
    
    float volumeValue = 0;
    for (int i = 0; i < steps; i++)
    {
        volumeValue += volume.SampleLevel(vSampler, NonUniformResourceIndex((samplePoint / extends)), 0) * weight;
        samplePoint = samplePoint + stepVector;
    }
    return float2(volumeValue, 1);
}

[numthreads(8, 8, 1)]void main(int2 did
                                : SV_DispatchThreadId)
{
    
    float preivousT = rayDepth[NonUniformResourceIndex(did)];
    output[did].a = 1.f;
    
    float3 origin = mul(camera.toWorld, float4(0, 0, 0, 1)).xyz;
    float3 direction = mul(camera.toWorld, float4(GenerateRayDirection(did, viewportDimensions, camera.fov), 0)).xyz;
    
    // Check if hit at all
    float2 minMaxT = IntersectAABB(origin, direction, volumeData.min, volumeData.max);
    minMaxT = float2(max(0, min(minMaxT.x, preivousT)), min(minMaxT.y, preivousT));
    
    if ((minMaxT.y < minMaxT.x) || (preivousT <= minMaxT.x) || (minMaxT.y < 0))
    {
        return;
    }
       
    float hitvolumes = 0;
    float volumeValue = 0;
    uint chunkCount = volumeData.chunkCount;
    for (uint i = 0; i < chunkCount; i++)
    {
        float3 min = chunkMinMaxData[i * 2];
        float3 max = chunkMinMaxData[i * 2 + 1];
        Texture3D<float> volume = volumes[i];
        float2 res = TraceVolume(origin, direction, preivousT, min, max, volume);
        volumeValue += res.x;
        hitvolumes += res.y;

    }

    if ((minAccumulateValue < volumeValue) && (!excludeExceeding || (volumeValue <= maxAccumulateValue)))
    {
        float value = clamp((volumeValue - minAccumulateValue) / (maxAccumulateValue - minAccumulateValue), 0, 1);
        float blendValue = min((1 - baseTransparency) * value + baseTransparency, 1.f);
        output[did] = lerp(output[did], float4(Plasma(value), 1), blendValue);
    }
    else if (enableChunkDebugging)
    {
        if (0 == hitvolumes)
        {
            output[did] = lerp(output[did], float4(0.5, 0.5, 0.5, 1), baseTransparency);
        }
        else
        {
            float4 debugColor = float4(Oranges(hitvolumes / maxDebugChunkCount), 1);
            output[did] = lerp(output[did], debugColor, baseTransparency);

        }
    }
}