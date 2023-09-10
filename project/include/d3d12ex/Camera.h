/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <configuration/Configuration.h>
#include <rayvis-utils/MathTypes.h>

#include <optional>

class Camera : public core::IConfigurationComponent {
public:
    Matrix4x4 CalcFromWorld();
    Matrix4x4 CalcToWorld();

    void SetCameraLookAt(Float3 center, std::optional<Float3> eye = std::nullopt, Float3 upVector = F3UP);
    void RecalculateUp();

    void MoveUp(float deltaSeconds);
    void MoveRight(float deltaSeconds);
    void MoveForward(float deltaSeconds);

    void LookRight(float degrees);
    void LookUp(float degrees);

    void  SetSpeed(const float speed);
    float GetSpeed() const;

    void  SetFoV(const float fov);
    float GetFoV() const;
    float GetFoVRad() const;

    void  SetTMin(const float tMin);
    float GetTMin() const;

    void  SetTMax(const float tMax);
    float GetTMax() const;

    void   SetPosition(const Float3& pos);
    Float3 GetPosition() const;

    Float3 GetForwardG() const;
    Float3 GetUpG() const;
    Float3 GetRightG() const;

    Float3 GetForwardL() const;
    Float3 GetUpL() const;
    Float3 GetRightL() const;

    Matrix4x4 GetRoationMatrix() const;

    core::ConfigurationEntry::FloatParameters GetConfigParameters(
        const std::string_view     key,
        const std::source_location sourceLocation = std::source_location::current());

private:
    void SetUpL(const Float3& up);
    void SetRightL(const Float3& right);

    void                  SetConfigurationImpl(std::unique_ptr<core::IConfiguration>&& configuration) override;
    core::IConfiguration& GetConfigurationImpl() override;

    float                                 yaw_   = 0;
    float                                 pitch_ = 0;
    std::unique_ptr<core::IConfiguration> config_;
};

inline void Camera::SetSpeed(const float speed)
{
    config_->Set<float>("speed", speed);
}

inline float Camera::GetSpeed() const
{
    return config_->Get<float>("speed");
}

inline void Camera::SetFoV(const float fov)
{
    config_->Set<float>("fov", fov);
}

inline float Camera::GetFoV() const
{
    return config_->Get<float>("fov");
}

inline float Camera::GetFoVRad() const
{
    return config_->Get<float>("fov") * PI / 180;
}


inline void Camera::SetTMin(const float minT)
{
    config_->Set<float>("minT", minT);
}

inline float Camera::GetTMin() const
{
    return config_->Get<float>("minT");
}

inline void Camera::SetTMax(const float maxT)
{
    config_->Set<float>("maxT", maxT);
}

inline float Camera::GetTMax() const
{
    return config_->Get<float>("maxT");
}

inline void Camera::SetPosition(const Float3& pos)
{
    config_->Set<glm::vec3>("pos", glm::vec3(pos.x, pos.y, pos.z));
}

inline Float3 Camera::GetPosition() const
{
    const auto v = config_->Get<glm::vec3>("pos");
    return to3(v);
};

inline Float3 Camera::GetForwardL() const
{
    const auto u = GetUpL();
    const auto r = GetRightL();
    assert(nearlyZero(linalg::dot(u, r)));
    return linalg::normalize(linalg::cross(u, r));
};

inline void Camera::SetUpL(const Float3& up)
{
    config_->Set<glm::vec3>("up", glm::vec3(up.x, up.y, up.z));
}

inline Float3 Camera::GetUpL() const
{
    const auto v = config_->Get<glm::vec3>("up");
    return to3(v);
};

inline void Camera::SetRightL(const Float3& right)
{
    config_->Set<glm::vec3>("right", glm::vec3(right.x, right.y, right.z));
}

inline Float3 Camera::GetRightL() const
{
    const auto v = config_->Get<glm::vec3>("right");
    return to3(v);
};