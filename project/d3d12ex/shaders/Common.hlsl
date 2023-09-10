/// DATASTRUCTURES

struct Camera
{
    float4x4 toWorld;
    float tMin;
    float tMax;
    float fov; // in degree
};

/// MATH

#define PI 3.14159265f
#define SQR3 1.73205080757f

float2 rotate(float2 vec, float rad)
{
    float x = cos(rad) * vec.x - sin(rad) * vec.y;
    float y = sin(rad) * vec.x + cos(rad) * vec.y;
    return float2(x, y);
}

float sin1(float value)
{
    return (sin(value) + 1) / 2;
}