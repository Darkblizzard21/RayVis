/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once

#include <amdrdf.h>

#include <nlohmann/json.hpp>

#include "d3d12ex/Mesh.h"

enum InstanceMask : uint8_t {
    NEVER_INCLUDE            = 0,
    DEFAULT                  = 1 << 0,
    RAY_MESH                 = 1 << 1,
    DIRIECTIONAL_POINT_CLOUD = 1 << 2,
    ALLWAYS_INCLUDE          = 255
};

constexpr std::uint32_t SCENE_CHUNK_VERSION = 1;
constexpr const char*   SCENE_CHUNK_ID      = "RAYVIS_SCENE";

struct SceneChunkHeader final {
    size_t meshCount;
    size_t rootNodeCount;
};

class Scene {
public:
    struct Node {
        std::vector<std::shared_ptr<Node>> children  = {};
        std::optional<int>                 meshId    = std::nullopt;
        std::optional<Mesh*>               mesh      = std::nullopt;
        std::optional<Float3>              meshColor = std::nullopt;

        int       id           = -1;
        uint8_t   instanceMask = InstanceMask::DEFAULT;
        Matrix4x4 matrix       = linalg::identity;

        bool   IsChild() const;
        size_t MeshInstanceCount() const;
    };

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Node>> rootNodes;

    Scene() = default;
    Scene(ComPtr<ID3D12Device5> device);

    static Scene LoadFrom(ComPtr<ID3D12Device5> device, std::string path);
    static Scene LoadFromRAYVIS(ComPtr<ID3D12Device5> device, std::string path, size_t chunkIdx = 0);

    void SaveTo(rdf::ChunkFileWriter& writer) const;

    size_t InstanceCount() const;

    Vertex Min() const;
    Vertex Max() const;
    Vertex MinTransformed() const;
    Vertex MaxTransformed() const;

    void RecalculateMinMax();

    void OverrideMeshColors(Float3 meshColor);
    void OverrideMeshColors(std::function<Float3(const Node* const)> colorFunc);

private:
    void CalcualtedTransformedExtends(Scene::Node* node, Matrix4x4 matrixStack = linalg::identity);

    Vertex minExtends_            = math::Max<float, 3>();
    Vertex maxExtends_            = math::Min<float, 3>();
    Vertex minExtendsTransformed_ = math::Max<float, 3>();
    Vertex maxExtendsTransformed_ = math::Min<float, 3>();
};