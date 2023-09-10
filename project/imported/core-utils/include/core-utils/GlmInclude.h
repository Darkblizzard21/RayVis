#pragma once

// include file for glm header with configuration

// defines to setup GLM library
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_XYZW_ONLY
#define GLM_FORCE_INTRINSICS

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/transform.hpp>

#include "core-utils/StringUtils.h"

namespace core {

    inline int GetDominantAxis(const glm::vec2& v)
    {
        if (abs(v.y) > abs(v.x)) {
            return 1;
        }
        return 0;
    }

    inline int GetDominantAxis(const glm::vec3& v)
    {
        if (abs(v.y) > abs(v.x) && abs(v.y) > abs(v.z)) {
            return 1;
        } else if (abs(v.z) > abs(v.x) && abs(v.z) > abs(v.y)) {
            return 2;
        }
        return 0;
    }

    inline int GetDominantAxis(const glm::vec4& v)
    {
        if (abs(v.y) > abs(v.x) && abs(v.y) > abs(v.z) && abs(v.y) > abs(v.w)) {
            return 1;
        } else if (abs(v.z) > abs(v.x) && abs(v.z) > abs(v.y) && abs(v.z) > abs(v.w)) {
            return 2;
        } else if (abs(v.w) > abs(v.x) && abs(v.w) > abs(v.y) && abs(v.w) > abs(v.z)) {
            return 3;
        }

        return 0;
    }

}  // namespace core

template <typename T, glm::qualifier Q>
struct std::formatter<glm::vec<2, T, Q>> : std::formatter<T> {
    auto format(const glm::vec<2, T, Q>& value, std::format_context& context)
    {
        auto it = context.out();
        it++    = 'v';
        it++    = 'e';
        it++    = 'c';
        it++    = '2';
        it++    = '(';
        it      = std::formatter<T>::format(value.x, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.y, context);
        it++    = ')';
        return it;
    }

private:
    std::formatter<T> elementFormatter_;
};

template <typename T, glm::qualifier Q>
struct std::formatter<glm::vec<3, T, Q>> : std::formatter<T> {
    auto format(const glm::vec<3, T, Q>& value, std::format_context& context)
    {
        auto it = context.out();
        it++    = 'v';
        it++    = 'e';
        it++    = 'c';
        it++    = '3';
        it++    = '(';
        it      = std::formatter<T>::format(value.x, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.y, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.z, context);
        it++    = ')';
        return it;
    }
};

template <typename T, glm::qualifier Q>
struct std::formatter<glm::vec<4, T, Q>> : std::formatter<T> {
    auto format(const glm::vec<4, T, Q>& value, std::format_context& context)
    {
        auto it = context.out();
        it++    = 'v';
        it++    = 'e';
        it++    = 'c';
        it++    = '4';
        it++    = '(';
        it      = std::formatter<T>::format(value.x, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.y, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.z, context);
        it++    = ',';
        it++    = ' ';
        it      = std::formatter<T>::format(value.w, context);
        it++    = ')';
        return it;
    }
};
