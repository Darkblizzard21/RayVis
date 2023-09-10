
float3 GenerateRayDirection(int2 pixel, int2 viewportDimension, float fov)
{
    pixel = pixel - (viewportDimension / 2);
    const float aspectScale = viewportDimension.y < viewportDimension.x ? viewportDimension.x : viewportDimension.y;
    float tanHalfAngle = tan(fov / 2);
    return normalize(float3((float2(pixel.x, -pixel.y) * tanHalfAngle / aspectScale).xy, -1.0));
}

float2 IntersectAABB(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax)
{
    float3 dirFraction = 1 / rayDir;
    float3 tLower = (boxMin - rayOrigin) * dirFraction;
    float3 tUpper = (boxMax - rayOrigin) * dirFraction;
    float3 t1 = float3(min(tLower.x, tUpper.x), min(tLower.y, tUpper.y), min(tLower.z, tUpper.z));
    float3 t2 = float3(max(tLower.x, tUpper.x), max(tLower.y, tUpper.y), max(tLower.z, tUpper.z));
    float tNear = max(max(t1.x, t1.y), t1.z);
    float tFar = min(min(t2.x, t2.y), t2.z);
    return float2(tNear, tFar);
}