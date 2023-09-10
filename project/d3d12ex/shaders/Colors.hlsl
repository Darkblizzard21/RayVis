static const float3 PlasmaColors[10] =
{
    float3(0.05, 0.03, 0.53),
    float3(0.27, 0.01, 0.62),
    float3(0.45, 0, 0.66),
    float3(0.61, 0.09, 0.62),
    float3(0.74, 0.21, 0.52),
    float3(0.84, 0.34, 0.42),
    float3(0.93, 0.47, 0.33),
    float3(0.98, 0.62, 0.23),
    float3(0.99, 0.78, 0.15),
    float3(0.94, 0.97, 0.13)
};

static const float3 OrangesColors[6] =
{
    float3(0.85, 0.28, 0),
    float3(0.95, 0.41, 0.07),
    float3(0.99, 0.55, 0.24),
    float3(0.99, 0.68, 0.42),
    float3(0.99, 0.82, 0.64),
    float3(1, 0.93, 0.87)
};

float3 Plasma(float value)
{
    value = max(0, min(1, value));
    float scale = value * 9;
    float lower = floor(scale);
    float upper = ceil(scale);
    float lerpV = scale - lower;
    return lerp(PlasmaColors[lower], PlasmaColors[upper], lerpV);

}

float3 Oranges(float value)
{
    value = max(0, min(1, value));
    float scale = value * 5;
    float lower = floor(scale);
    float upper = ceil(scale);
    float lerpV = scale - lower;
    return lerp(OrangesColors[lower], OrangesColors[upper], lerpV);

}
