/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#undef min
#undef max
#include <linalg/linalg.h>
#include <spdlog/spdlog.h>

#include <glm/ext/vector_float3.hpp>

constexpr float PI = 3.141593;

using Int2      = linalg::vec<int32_t, 2>;
using Int3      = linalg::vec<int32_t, 3>;
using Float2    = linalg::vec<float, 2>;
using Float3    = linalg::vec<float, 3>;
using Float4    = linalg::vec<float, 4>;
using Double3   = linalg::vec<double, 3>;
using Double4   = linalg::vec<double, 4>;
using Vertex    = Float3;
using Matrix3x3 = linalg::mat<float, 3, 3>;
using Matrix4x4 = linalg::mat<float, 4, 4>;

constexpr Int2 I2ZERO   = {0, 0};
constexpr Int2 I2UP     = {0, 1};
constexpr Int2 I2DOWN   = {0, -1};
constexpr Int2 I23RIGHT = {1, 0};
constexpr Int2 I23LEFT  = {-1, 0};

constexpr Float3 F3ONE       = {1, 1, 1};
constexpr Float3 F3ZERO      = {0, 0, 0};
constexpr Float3 F3UP        = {0, 1, 0};
constexpr Float3 F3DOWN      = {0, -1, 0};
constexpr Float3 F3FORWARD   = {0, 0, 1};
constexpr Float3 F3BACKWARDS = {0, 0, -1};
constexpr Float3 F3RIGHT     = {1, 0, 0};
constexpr Float3 F3LEFT      = {-1, 0, 0};

constexpr Double3 D3FORWARD = {0, 0, 1};

namespace math {

    template <class T, int M>
    inline linalg::vec<T, M> Min()
    {
        return linalg::vec<T, M>(std::numeric_limits<T>::min());
    }

    template <class T, int M>
    inline linalg::vec<T, M> Max()
    {
        return linalg::vec<T, M>(std::numeric_limits<T>::max());
    }
}  // namespace math

template <typename T>
concept FloatingPoint = std::same_as<T, float> || std::same_as<T, double>;

template <typename T>
inline linalg::vec<T, 4> rotAtoB(const linalg::vec<T, 3>& a, const linalg::vec<T, 3>& b)
    requires(FloatingPoint<T>)
{
    const auto axis = linalg::cross(a, b);

    const auto           cosA   = linalg::dot(a, b);
    const auto           k      = 1.0f / (1.0f + cosA);
    linalg::mat<T, 3, 3> result = linalg::identity;
    result[0][0]                = (axis.x * axis.x * k) + cosA;
    result[1][0]                = (axis.y * axis.x * k) - axis.z;
    result[2][0]                = (axis.z * axis.x * k) + axis.y;

    result[0][1] = (axis.x * axis.y * k) + axis.z;
    result[1][1] = (axis.y * axis.y * k) + cosA;
    result[2][1] = (axis.z * axis.y * k) - axis.x;

    result[0][2] = (axis.x * axis.z * k) - axis.y;
    result[1][2] = (axis.y * axis.z * k) + axis.x;
    result[2][2] = (axis.z * axis.z * k) + cosA;
    return linalg::rotation_quat(result);
}

inline Matrix4x4 transform(Float3 translation, Float4 roatation, Float3 scale)
{
    Matrix4x4 result = linalg::identity;
    result           = mul(result, linalg::translation_matrix(translation));
    result           = mul(result, linalg::rotation_matrix(roatation));
    result           = mul(result, linalg::scaling_matrix(scale));
    return result;
}

constexpr linalg::vec<float, 3> mul(const linalg::mat<float, 4, 4>& a,
                                    const linalg::vec<float, 3>&    b,
                                    bool                            isDir = false)
{
    const linalg::vec<float, 4> b4  = {b.x, b.y, b.z, isDir ? 0.f : 1.f};
    const auto                  res = mul(a, b4);
    return linalg::vec<float, 3>(res.x, res.y, res.z);
}

template <class T>
constexpr linalg::vec<T, 4> to4(const linalg::vec<T, 3> vec, T w)
{
    return {vec.x, vec.y, vec.z, w};
}


constexpr Float4 to4(const glm::vec3& vec, float w = 0.f)
{
    return {vec.x, vec.y, vec.z, w};
}

constexpr Float4 to4(Float3 vec, float w = 0)
{
    return {vec.x, vec.y, vec.z, w};
}

template <class T>
constexpr linalg::vec<T, 3> to3(const linalg::vec<T, 4>& vec)
{
    return {vec.x, vec.y, vec.z};
}


constexpr Float3 to3(const glm::vec3& vec)
{
    return {vec.x, vec.y, vec.z};
}

constexpr linalg::vec<int32_t, 2> toInt(const Float2& vec)
{
    return {static_cast<int32_t>(vec.x), static_cast<int32_t>(vec.y)};
}

constexpr linalg::vec<int32_t, 3> toInt(const Float3& vec)
{
    return {static_cast<int32_t>(vec.x), static_cast<int32_t>(vec.y), static_cast<int32_t>(vec.z)};
}

constexpr linalg::vec<int32_t, 4> toInt(const Float4& vec)
{
    return {static_cast<int32_t>(vec.x),
            static_cast<int32_t>(vec.y),
            static_cast<int32_t>(vec.z),
            static_cast<int32_t>(vec.w)};
}

constexpr float degToRad(float deg)
{
    return deg * (PI / 180);
}

constexpr float radToDeg(float rad)
{
    return rad * (180 / PI);
}

inline Float3 rotateAround(Float3 axis, Float3 toRotate, float rad)
{
    const auto rotQuat = linalg::rotation_quat(axis, rad);
    const auto rotMat  = linalg::rotation_matrix(rotQuat);
    return mul(rotMat, toRotate);
}

inline bool nearlyEqual(double a, double b, double threshhold = 0.001)
{
    return abs(a - b) < threshhold;
}

inline bool nearlyZero(double a, double threshhold = 0.001)
{
    return abs(a) < threshhold;
}

inline Float3 projectToPlane(const Float3& point, const Float3& planeNormal)
{
    const Float3 projected = point - (linalg::dot(point, planeNormal) * planeNormal);
    assert(linalg::dot(projected, planeNormal) == 0);
    return projected;
}

template <>
struct fmt::formatter<Float3> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    };

    template <typename FormatContext>
    inline auto format(const Float3& f3, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({},{},{})", f3.x, f3.y, f3.z);
    };
};

template <>
struct fmt::formatter<Float4> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext& ctx)
    {
        return ctx.begin();
    };

    template <typename FormatContext>
    inline auto format(const Float4& f4, FormatContext& ctx) const
    {
        return fmt::format_to(ctx.out(), "({},{},{},{})", f4.x, f4.y, f4.z, f4.w);
    };
};
