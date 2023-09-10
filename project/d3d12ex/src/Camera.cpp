/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Camera.h"

#include <spdlog/spdlog.h>

#include <cassert>

Matrix4x4 Camera::CalcFromWorld()
{
    const auto eye        = GetPosition();
    const auto center     = eye + GetForwardG();
    const auto view_y_dir = GetUpG();
    const auto lookAt     = linalg::lookat_matrix(eye, center, view_y_dir);
    return lookAt;
}

Matrix4x4 Camera::CalcToWorld()
{
    return linalg::inverse(CalcFromWorld());
}

void Camera::SetCameraLookAt(Float3 center, std::optional<Float3> eye, Float3 upVector)
{
    upVector = normalize(upVector);
    if (!eye.has_value()) {
        eye = GetPosition();
    }
    const auto forward     = normalize(center - eye.value());
    const auto rightVector = normalize(linalg::cross(forward, upVector));
    assert(rightVector != F3ZERO);

    // Correct up Vector
    if (linalg::dot(forward, upVector) != 0) {
        upVector = normalize(linalg::cross(rightVector, forward));
    }

    assert(nearlyZero(linalg::dot(upVector, rightVector)));
    SetPosition(*eye);
    SetUpL(normalize(upVector));
    SetRightL(normalize(rightVector));
    yaw_   = 0;
    pitch_ = 0;
}

#define sign(x) ((x == 0) ? 0 : ((0 < x) ? 1 : -1))

void Camera::RecalculateUp()
{
    const auto rotationValueRight = std::roundf(yaw_ / (PI / 2)) * (PI / 2);
    auto       up                 = GetUpL();
    auto       right              = GetRightL();
    right                         = linalg::normalize(rotateAround(up, right, rotationValueRight));
    assert(std::abs(right.x) < 0.01 || 0.99 < std::abs(right.x));
    assert(std::abs(right.y) < 0.01 || 0.99 < std::abs(right.y));
    assert(std::abs(right.z) < 0.01 || 0.99 < std::abs(right.z));

    right = Float3(0.99 < std::abs(right.x) ? sign(right.x) : 0,
                   0.99 < std::abs(right.y) ? sign(right.y) : 0,
                   0.99 < std::abs(right.z) ? sign(right.z) : 0);

    SetRightL(right);
    yaw_ = 0;

    const auto rotationValueUp = std::roundf(pitch_ / (PI / 2)) * (PI / 2);

    up = linalg::normalize(rotateAround(right, up, rotationValueUp));
    assert(std::abs(up.x) < 0.01 || 0.99 < std::abs(up.x));
    assert(std::abs(up.y) < 0.01 || 0.99 < std::abs(up.y));
    assert(std::abs(up.z) < 0.01 || 0.99 < std::abs(up.z));

    up = Float3(0.99 < std::abs(up.x) ? sign(up.x) : 0,
                0.99 < std::abs(up.y) ? sign(up.y) : 0,
                0.99 < std::abs(up.z) ? sign(up.z) : 0);

    SetUpL(up);
    pitch_ = 0;
}

void Camera::MoveUp(float deltaSeconds)
{
    SetPosition(GetPosition() + GetUpG() * (deltaSeconds * GetSpeed()));
}

void Camera::MoveRight(float deltaSeconds)
{
    SetPosition(GetPosition() + GetRightG() * (deltaSeconds * GetSpeed()));
}

void Camera::MoveForward(float deltaSeconds)
{
    SetPosition(GetPosition() + GetForwardG() * (deltaSeconds * GetSpeed()));
}

void Camera::LookRight(float degrees)
{
    const float sign = config_->Get<bool>("invert") ? -1 : 1;
    yaw_ -= sign * degToRad(degrees) * 0.66;
}

void Camera::LookUp(float degrees)
{
    const float sign = config_->Get<bool>("invert") ? -1 : 1;
    pitch_ -= sign * degToRad(degrees) * 0.66;
    pitch_ = std::min(std::max(pitch_, -PI / 2), PI / 2);
}

Float3 Camera::GetForwardG() const
{
    const auto forward = mul(GetRoationMatrix(), GetForwardL());
    return linalg::normalize(forward);
}

Float3 Camera::GetUpG() const
{
    return mul(GetRoationMatrix(), GetUpL());
}

Float3 Camera::GetRightG() const
{
    return mul(GetRoationMatrix(), GetRightL());
}

Matrix4x4 Camera::GetRoationMatrix() const
{
    const auto yawMat   = linalg::rotation_matrix(linalg::rotation_quat(GetUpL(), yaw_));
    const auto pitchMat = linalg::rotation_matrix(linalg::rotation_quat(GetRightL(), pitch_));
    return mul(yawMat, pitchMat);
}

core::ConfigurationEntry::FloatParameters Camera::GetConfigParameters(const std::string_view     key,
                                                                      const std::source_location sourceLocation)
{
    return std::get<core::ConfigurationEntry::FloatParameters>(config_->GetEntry(key, sourceLocation).GetParameters());
}

void Camera::SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration)
{
    core::ConfigurationEntry::FloatParameters floatParams = {};
    floatParams.min                                       = 60;
    floatParams.max                                       = 180;
    configuration->Register("fov", 90, "Camera FoV", "Camera field of view", floatParams);

    floatParams.min = 0.01f;
    floatParams.max = 1000000;
    configuration->Register("speed", 5000, "Camera FoV", "Camera field of view", floatParams);

    floatParams.min = 0.f;
    floatParams.max = pow(2.f, 31.5f);
    configuration->Register("minT", 0.1f, "Camera minT", "Camera minT", floatParams);
    floatParams.min = 1.f;
    floatParams.max = pow(2.f, 32.0f);
    configuration->Register("maxT", 150000, "Camera maxT", "Camera maxT", floatParams);

    floatParams.min = std::numeric_limits<float>::lowest();
    floatParams.max = std::numeric_limits<float>::max();
    configuration->Register("pos", glm::vec3(125, -5000, 2000), "Camera position", "position", floatParams);

    floatParams.min = -1.f;
    floatParams.max = 1.f;
    configuration->Register("up", glm::vec3(0, 0, 1), "Camera up", "Camera up vector", floatParams);

    floatParams.min = -1.f;
    floatParams.max = 1.f;
    configuration->Register("right", glm::vec3(1, 0, 0), "Camera right", "Camera right vector", floatParams);

    configuration->Register("invert", true, "Camera Invert", "");

    config_ = std::move(configuration);
}

core::IConfiguration& Camera::GetConfigurationImpl()
{
    return *config_.get();
}
