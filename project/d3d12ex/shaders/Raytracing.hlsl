#include "Colors.hlsl"
#include "Common.hlsl"
#include "RaytracingAlogrithm.hlsl"

cbuffer constants : register(b0)
{
    Camera camera;
    float elapsed;
    int viewportWidth;
    int viewportHeight;
    int shaderMode;
    int visualizationMode;
    float3 lightDirection;
}

RaytracingAccelerationStructure tlas : register(t0);
Buffer<float3> instanceColors : register(t1);
Buffer<uint> instanceGeomtryMapping : register(t2);

Buffer<float3> primitiveNormals[16384] : register(t0, space1);
Buffer<uint> primitiveIndecies[16384] : register(t0, space2);

RWTexture2D<float4> output : register(u0);
RWTexture2D<float> rayDepth : register(u1);

float3 CalculateNormal(RayQuery< RAY_FLAG_NONE> q)
{
uint geometryIdx = instanceGeomtryMapping[NonUniformResourceIndex(q.CommittedInstanceIndex())] + q.CommittedGeometryIndex();
Buffer<uint> indecies = primitiveIndecies[NonUniformResourceIndex(geometryIdx)];
Buffer<float3> geomtryNormals = primitiveNormals[NonUniformResourceIndex(geometryIdx)];

uint vertexIndex = q.CommittedPrimitiveIndex() * 3;
uint aIdx = indecies[NonUniformResourceIndex(vertexIndex)];
uint bIdx = indecies[NonUniformResourceIndex(vertexIndex + 1)];
uint cIdx = indecies[NonUniformResourceIndex(vertexIndex + 2)];

float2 baryCentrics = q.CommittedTriangleBarycentrics();
float3 aNormal = geomtryNormals[NonUniformResourceIndex(aIdx)] * (1 - (baryCentrics.x + baryCentrics.y));
float3 bNormal = geomtryNormals[NonUniformResourceIndex(bIdx)] * baryCentrics.x;
float3 cNormal = geomtryNormals[NonUniformResourceIndex(cIdx)] * baryCentrics.y;
float3 normalObjectNormal = aNormal + bNormal + cNormal;

float3x4 mat = q.CommittedObjectToWorld3x4();
float3 normal = normalize(mul(mat, float4(normalObjectNormal.xyz, 0)));
    return
normal;
}

// Shading Functions:
float4 ShadeBarycentric(RayQuery< RAY_FLAG_NONE> q, RayDesc ray)
{
    float b = q.CommittedRayT() / ray.TMax;
    return float4(q.CommittedTriangleBarycentrics(), b, 1);
}

float4 ShadeShadows(RayQuery< RAY_FLAG_NONE> q, RayDesc firstRay)
{
    float3 hitPoint = firstRay.Origin + (firstRay.Direction * q.CommittedRayT());

    RayDesc lightRay;
    lightRay.Origin = hitPoint;
    lightRay.TMin = 0.00001;
    lightRay.Direction = -lightDirection;
    lightRay.TMax = camera.tMax;

    RayQuery < RAY_FLAG_NONE > sq;
    sq.TraceRayInline(tlas, 0, 1, lightRay);
    sq.Proceed();
    if (sq.CommittedStatus() == COMMITTED_TRIANGLE_HIT)
    {
        return float4(0.5, 0.5, 0.5, 1);
    }
    else
    {
        return float4(1, 1, 1, 1);
    }
}

[numthreads(8, 8, 1)]void main(int2 did
                                : SV_DispatchThreadId)
{
    RayDesc ray;
    ray.Origin = mul(camera.toWorld, float4(0, 0, 0, 1)).xyz;
    ray.TMin = camera.tMin;
    ray.Direction = mul(camera.toWorld, float4(GenerateRayDirection(did, int2(viewportWidth, viewportHeight), camera.fov), 0)).xyz;
    ray.TMax = camera.tMax;

    int instanceMask = 255;
    if (shaderMode < 0)
    {
        instanceMask = instanceMask - 1;
    }
    
    RayQuery < RAY_FLAG_NONE > q;
    q.TraceRayInline(tlas, 0, instanceMask, ray);
    q.Proceed();
    
    float maxRay = ray.TMax;
    if (q.CommittedStatus() == COMMITTED_TRIANGLE_HIT && q.CandidateType() == CANDIDATE_NON_OPAQUE_TRIANGLE)
    {
        bool raymesh = visualizationMode == 1;
        bool hitSpecialMesh = 0 < q.CommittedInstanceID();
        int specialShaderMode = raymesh ? 1 : 2;
        int mode = hitSpecialMesh ? specialShaderMode : shaderMode;
        
        maxRay = q.CommittedRayT();
        switch (mode)
        {
            case -1:{
                    maxRay = camera.tMax;
                    break;
                }
            case 0: // SmoothShadingSW
            {
                    float3 normal = CalculateNormal(q);
                    float lambertian = max(dot(normal, lightDirection), 0.f);
                    lambertian = 0.1f + lambertian * 0.9f;
                    output[did] = lambertian;
                    break;
                }
            case 1: // SmoothShadingInstanceColors
            {
                    const bool raymeshShading = (raymesh && hitSpecialMesh);
                    const float3 normal = CalculateNormal(q);
                    const float3 light = raymeshShading ? (mul(camera.toWorld, float4(0, 1, 0, 0)).xyz) : lightDirection;
                    float lambertian = max(dot(normal, lightDirection), 0.f);
                    const float baseColor = raymeshShading ? 0.5f : 0.2f;
                    lambertian = lerp(baseColor, 1.f, lambertian);
                    output[did] = float4((lambertian * instanceColors[NonUniformResourceIndex(q.CommittedInstanceIndex())]).rgb, 1);
                    break;
                }
            case 2: // InstanceColors
                output[did] = float4(instanceColors[NonUniformResourceIndex(q.CommittedInstanceIndex())], 1);
                break;
            case 3: // Shadows
                output[did] = ShadeShadows(q, ray);
                break;
            case 4: // Normals
            {
                    float3 normal = CalculateNormal(q);
                    float3 normalColor = ((normal) + 1.f) * 0.5f;
                    output[did] = float4(normalColor.rgb, 1);
                    break;
                }
            case 5: // Barycentry 
            {
                    output[did] = ShadeBarycentric(q, ray);
                    break;
                }
        }

    }
    else
    {
        output[did] = 0;
    }
    
    rayDepth[did] = maxRay;
}