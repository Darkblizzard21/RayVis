/* Copyright (c) 2023 Pirmin Pfeifer */
#include "SimpleRayMeshGenerator.h"

#undef min

namespace {
    std::unique_ptr<Mesh> PlaneCross(ComPtr<ID3D12Device5> device)
    {
        std::array<Vertex, 8> vertexBuffer;
        const auto            ap = 0;
        const auto            bp = 1;
        const auto            cp = 2;
        const auto            dp = 3;
        const auto            an = 4;
        const auto            bn = 5;
        const auto            cn = 6;
        const auto            dn = 7;
        // Front Vertex
        // a-1, 1     b1,1
        // c-1,-1     d1,-1
        vertexBuffer[ap]         = Float3(-1, 1, 1);
        vertexBuffer[bp]         = Float3(1, 1, 1);
        vertexBuffer[cp]         = Float3(-1, -1, 1);
        vertexBuffer[dp]         = Float3(1, -1, 1);
        // Back Vertex
        vertexBuffer[an]         = Float3(-1, 1, -1);
        vertexBuffer[bn]         = Float3(1, 1, -1);
        vertexBuffer[cn]         = Float3(-1, -1, -1);
        vertexBuffer[dn]         = Float3(1, -1, -1);

        std::array<UINT16, 4 * 3> indexBuffer;
        // First Plane
        // ap an dp
        indexBuffer[0]  = ap;
        indexBuffer[1]  = an;
        indexBuffer[2]  = dp;
        // dp an dn
        indexBuffer[3]  = dp;
        indexBuffer[4]  = an;
        indexBuffer[5]  = dn;
        // Second Plane
        // bp bn cp
        indexBuffer[6]  = bp;
        indexBuffer[7]  = bn;
        indexBuffer[8]  = cp;
        // cp bn cn
        indexBuffer[9]  = cp;
        indexBuffer[10] = bn;
        indexBuffer[11] = cn;

        // Apply uniform scale
        for (size_t i = 0; i < vertexBuffer.size(); i++) {
            vertexBuffer[i] *= 0.5f;
        }

        return std::make_unique<Mesh>(device, vertexBuffer, indexBuffer);
    }
}  // namespace

namespace SimpleRayMeshGenerator {
    Scene GenerateLines(ComPtr<ID3D12Device5> device, RayTrace* raytraces)
    {
        LineDescription des;
        des.raytraces = raytraces;
        return GenerateLines(device, des);
    }

    Scene GenerateLines(ComPtr<ID3D12Device5> device, const LineDescription& desc)
    {
        assert(desc.rayStride > 0);
        auto scene = Scene();
        scene.meshes.push_back(std::move(PlaneCross(device)));
        auto rootNode = std::make_unique<Scene::Node>();
        int  nodeId   = 0;
        rootNode->id  = nodeId++;

        const auto& rays = desc.raytraces->rays;
        for (size_t i = 0; i < rays.size(); i += desc.rayStride) {
            const auto& ray = rays[i];
            if (!IncludeRay(ray, desc.filter)) {
                continue;
            }

            auto rayNode          = std::make_shared<Scene::Node>();
            rayNode->id           = nodeId++;
            rayNode->mesh         = scene.meshes[0].get();
            rayNode->meshId       = 0;
            rayNode->instanceMask = InstanceMask::RAY_MESH;
            rayNode->meshColor    = desc.color;

            // Make matrix
            const float  rayT       = std::min(ray.tHitOrTMax(), desc.maxT);
            const Float3 meshOrigin = ray.origin + (ray.direction * rayT * 0.5f);
            const Float3 scale      = {desc.thickness, desc.thickness, rayT * linalg::length(ray.direction)};
            const Float4 roatation  = Float4(rotAtoB(D3FORWARD, Double3(ray.direction)));
            rayNode->matrix         = transform(meshOrigin, roatation, scale);

            rootNode->children.push_back(rayNode);
        }

        scene.rootNodes.push_back(std::move(rootNode));
        scene.RecalculateMinMax();

        const auto  skipedRays    = rays.size() - nodeId;
        const float skipedPercent = (1.f - (static_cast<float>(nodeId) / rays.size())) * 100;
        spdlog::info("Generated RayNodes {} (Rays skipped: {}, {}%)", nodeId, skipedRays, skipedPercent);
        return scene;
    }

}  // namespace SimpleRayMeshGenerator
