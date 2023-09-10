/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Scene.h"

#include <rayvis-utils/BreakAssert.h>
#include <rayvis-utils/DataStructures.h>

#include <filesystem>
#include <queue>
#include <set>

namespace {
    void OverrideColor(Scene::Node* node, std::function<Float3(const Scene::Node* const)>& colorFunc)
    {
        if (node->mesh) {
            node->meshColor = colorFunc(node);
        }
        for (const auto& child : node->children) {
            OverrideColor(child.get(), colorFunc);
        }
    }

    struct MeshPrefix {
        size_t primitvieCount;
    };

    struct PrimitivePrefix {
        size_t vertexByteSize;
        size_t vertexCount;
        size_t indexByteSize;
        size_t indexCount;
    };

    struct RootNodePrefix {
        size_t nodeCount;
        size_t rootNodeId;
    };

    struct NodePrefix {
        size_t childCount;

        size_t meshId    = std::numeric_limits<size_t>::max();  // max == no mesh
        Float3 meshColor = Float3(0.f);

        int32_t   id;
        uint8_t   instanceMask;
        Matrix4x4 matrix;
    };
}  // namespace

bool Scene::Node::IsChild() const
{
    return children.empty();
}

size_t Scene::Node::MeshInstanceCount() const
{
    size_t count = 0;
    if (mesh.has_value()) {
        count++;
    }
    for (auto child : children) {
        count += child->MeshInstanceCount();
    }
    return count;
}

Scene::Scene(ComPtr<ID3D12Device5> device)
{
    std::array<Vertex, 3> triangle = {Vertex(0, 1, 0), Vertex(1, 0, 0), Vertex(-1, 0, 0)};
    std::array<UINT16, 3> index    = {0, 1, 2};
    meshes.push_back(std::make_unique<Mesh>(device, triangle, index));
    auto node       = std::make_unique<Scene::Node>();
    node->meshId    = 0;
    node->mesh      = meshes[0].get();
    node->meshColor = Float3(1, 0, 1);

    node->id = 0;
    rootNodes.push_back(std::move(node));

    RecalculateMinMax();
}

Scene Scene::LoadFrom(ComPtr<ID3D12Device5> device, std::string path)
{
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error(fmt::format("path doesn't exist (\"{}\")", path));
    }

    std::filesystem::path filePath(path);
    if (filePath.extension() == ".rayvis") {
        return LoadFromRAYVIS(device, path);
    }
    throw std::runtime_error(fmt::format("File \"{}\" is not supported", filePath.filename().string()));
}

Scene Scene::LoadFromRAYVIS(ComPtr<ID3D12Device5> device, std::string filename, size_t chunkIdx)
{
    auto path = std::filesystem::path(filename);
    Assert(path.extension().string() == ".rayvis");
    if (!std::filesystem::exists(path)) {
        spdlog::error("Scene Loading: file dose not exist");
        Assert(false);
        throw std::runtime_error("file dose not exist");
    }

    auto chunkfile = rdf::ChunkFile(filename.c_str());

    if (chunkfile.GetChunkCount(SCENE_CHUNK_ID) <= chunkIdx) {
        throw std::runtime_error(fmt::format("Scene Loading: missing chunk with SCENE_CHUNK_ID at index {}", chunkIdx));
    }

    const auto headerSize   = chunkfile.GetChunkHeaderSize(SCENE_CHUNK_ID, chunkIdx);
    const auto chunkVersion = chunkfile.GetChunkVersion(SCENE_CHUNK_ID, chunkIdx);

    if (headerSize != sizeof(SceneChunkHeader)) {
        throw std::runtime_error("Scene Loading: Wrong header size");
    }
    if (chunkVersion != SCENE_CHUNK_VERSION) {
        throw std::runtime_error("Scene Loading: Wrong SCENE_CHUNK_VERSION");
    }

    Scene            scene = Scene();
    SceneChunkHeader header;
    chunkfile.ReadChunkHeaderToBuffer(SCENE_CHUNK_ID, chunkIdx, &header);
    chunkfile.ReadChunkData(SCENE_CHUNK_ID, chunkIdx, [&](int64_t dataSize, const void* data) {
        BufferReader reader(dataSize, data);
        // Load meshes
        for (size_t meshId = 0; meshId < header.meshCount; meshId++) {
            MeshPrefix meshPrefix = {};
            reader.read(meshPrefix);
            std::vector<Mesh::PrimitivePackV> primitives;
            for (size_t primId = 0; primId < meshPrefix.primitvieCount; primId++) {
                PrimitivePrefix primPrefix = {};
                reader.read(primPrefix);

                Mesh::PrimitivePackV pack = {};
                // Read Vertex
                Assert(primPrefix.vertexByteSize == sizeof(Vertex));
                std::vector<Vertex> vertices = {};
                vertices.resize(primPrefix.vertexCount);
                reader.read(vertices.data(), primPrefix.vertexByteSize * primPrefix.vertexCount);
                pack.first = std::move(vertices);

                // Read Indicies
                if (primPrefix.indexByteSize == sizeof(UINT)) {
                    std::vector<UINT> indicies;
                    indicies.resize(primPrefix.indexCount);
                    reader.read(indicies.data(), primPrefix.indexByteSize * primPrefix.indexCount);
                    pack.second = std::move(indicies);
                } else if (primPrefix.indexByteSize == sizeof(UINT16)) {
                    std::vector<UINT16> indicies;
                    indicies.resize(primPrefix.indexCount);
                    reader.read(indicies.data(), primPrefix.indexByteSize * primPrefix.indexCount);
                    pack.second = std::move(indicies);
                } else {
                    throw std::runtime_error(
                        fmt::format("Scene Loading: Unknown index format with size {}", primPrefix.indexByteSize));
                }
                primitives.push_back(pack);
            }
            scene.meshes.push_back(std::make_unique<Mesh>(device, primitives));
        }

        // load root nodes
        const auto readNextNode = [&reader]() {
            NodePrefix nodePrefix = {};
            reader.read(nodePrefix);
            std::vector<uint32_t> nodeChildIds = {};
            nodeChildIds.resize(nodePrefix.childCount);
            reader.read(nodeChildIds.data(), sizeof(uint32_t) * nodePrefix.childCount);
            return std::pair(std::move(nodePrefix), std::move(nodeChildIds));
        };

        const auto setUpNode = [&meshes = scene.meshes](Node* node, const NodePrefix& prefix) {
            node->meshId =
                prefix.meshId < std::numeric_limits<size_t>::max() ? std::optional(prefix.meshId) : std::nullopt;
            if (node->meshId.has_value()) {
                node->mesh      = meshes[*node->meshId].get();
                node->meshColor = prefix.meshColor;
            }

            node->id           = prefix.id;
            node->instanceMask = prefix.instanceMask;
            node->matrix       = prefix.matrix;
        };

        for (size_t rootNodeId = 0; rootNodeId < header.rootNodeCount; rootNodeId++) {
            RootNodePrefix rnPrefix = {};
            reader.read(rnPrefix);
            Assert(rnPrefix.rootNodeId == 0);

            std::unique_ptr<Node> rootNode         = std::make_unique<Node>();
            std::vector<uint32_t> rootNodeChildIds = {};
            {
                auto rootNodeData = readNextNode();
                setUpNode(rootNode.get(), rootNodeData.first);
                rootNodeChildIds = std::move(rootNodeData.second);
            }

            std::vector<std::pair<std::shared_ptr<Node>, std::vector<uint32_t>>> nodes;
            // nodes[0] = root // should never be read
            nodes.push_back(std::pair<std::shared_ptr<Node>, std::vector<uint32_t>>(nullptr, {}));
            for (size_t nodeId = 1; nodeId < rnPrefix.nodeCount; nodeId++) {
                auto nodeData = readNextNode();
                auto node     = std::make_shared<Node>();
                setUpNode(node.get(), nodeData.first);
                nodes.push_back(std::pair(std::move(node), std::move(nodeData.second)));
            }

            // Fill in node children
            for (size_t nodeId = 0; nodeId < rnPrefix.nodeCount; nodeId++) {
                Node*                  currentNode = rootNode.get();
                std::vector<uint32_t>& children    = rootNodeChildIds;
                if (nodeId != 0) {
                    currentNode = nodes[nodeId].first.get();
                    children    = nodes[nodeId].second;
                }

                for (const auto& childId : children) {
                    Assert(0 < childId);
                    currentNode->children.push_back(nodes[childId].first);
                }
            }

            scene.rootNodes.push_back(std::move(rootNode));
        }

        if (!reader.empty()) {
            throw std::runtime_error(
                fmt::format("Scene Loading: Finished reading but chunk still contains data ({} bytes | read {:0.1f})",
                            reader.remainingBytes(),
                            reader.progress() * 100.0));
        }
    });

    return scene;
}

void Scene::SaveTo(rdf::ChunkFileWriter& writer) const
{
    if (!config::EnableFileSave) {
        throw "SAVE WAS DISABLED";
    }
    auto header          = SceneChunkHeader();
    header.meshCount     = meshes.size();
    header.rootNodeCount = rootNodes.size();

    writer.BeginChunk(SCENE_CHUNK_ID, sizeof(SceneChunkHeader), &header, rdfCompressionZstd, SCENE_CHUNK_VERSION);

    // Write Meshes
    for (const auto& mesh : meshes) {
        MeshPrefix meshPrefix     = {};
        meshPrefix.primitvieCount = mesh->PrimitiveCount();
        writer.AppendToChunk(meshPrefix);
        for (size_t i = 0; i < mesh->PrimitiveCount(); i++) {
            PrimitivePrefix primPrefix = {};
            primPrefix.vertexByteSize  = mesh->VertexByteSize(i);
            primPrefix.vertexCount     = mesh->VertexCount(i);
            primPrefix.indexByteSize   = mesh->IndexByteSize(i);
            primPrefix.indexCount      = mesh->IndexCount(i);
            writer.AppendToChunk(primPrefix);

            const auto& prim = mesh->primitives_[i];
            // Add vertex
            writer.AppendToChunk(primPrefix.vertexByteSize * primPrefix.vertexCount, prim.vertices_.data());

            // Add Indecies
            std::vector<UINT16> index16;
            const void*         data;
            if (primPrefix.indexByteSize == sizeof(UINT)) {
                data = reinterpret_cast<const void*>(prim.indecies.data());
            } else if (primPrefix.indexByteSize == sizeof(UINT16)) {
                for (const auto& index : prim.indecies) {
                    index16.push_back(static_cast<UINT16>(index));
                }
                data = reinterpret_cast<const void*>(index16.data());
            } else {
                throw "Saving failed invalid index format";
            }
            writer.AppendToChunk(primPrefix.indexByteSize * primPrefix.indexCount, data);
        }
    }

    for (const auto& rootNode : rootNodes) {
        std::vector<Node*>      nodes;
        std::map<Node*, size_t> nodeIds;
        {
            std::set<Node*>   nodeSet;
            std::queue<Node*> frontier;
            frontier.push(rootNode.get());
            // ToDo make nodes
            while (!frontier.empty()) {
                const auto& currentNode = frontier.front();
                frontier.pop();
                for (const auto& child : currentNode->children) {
                    if (!nodeSet.contains(child.get())) {
                        frontier.push(child.get());
                        nodeSet.insert(child.get());
                    }
                }
            }
            nodes.clear();
            nodes.reserve(nodeSet.size() + 1);
            nodes.push_back(rootNode.get());
            nodes.insert(nodes.end(), nodeSet.begin(), nodeSet.end());
            for (size_t i = 0; i < nodes.size(); i++) {
                nodeIds[nodes[i]] = i;
            }
        }

        RootNodePrefix rnPrefix = {};
        rnPrefix.nodeCount      = nodes.size();
        rnPrefix.rootNodeId     = nodeIds[rootNode.get()];
        Assert(rnPrefix.rootNodeId == 0);

        writer.AppendToChunk(rnPrefix);
        for (const auto& node : nodes) {
            NodePrefix nodePrefix = {};

            nodePrefix.childCount = node->children.size();
            nodePrefix.meshId     = [&]() {
                if (node->mesh.has_value()) {
                    Mesh* ptr = *node->mesh;
                    for (size_t i = 0; i < meshes.size(); i++) {
                        if (ptr == meshes[i].get()) {
                            return i;
                        }
                    }
                    spdlog::error(fmt::format("Mesh ({}) not found in meshes!", reinterpret_cast<void*>(ptr)));
                }
                return std::numeric_limits<size_t>::max();  // default meshID;
            }();
            nodePrefix.meshColor = node->meshColor.value_or(Float3(0.f));

            nodePrefix.id           = node->id;
            nodePrefix.instanceMask = node->instanceMask;
            nodePrefix.matrix       = node->matrix;

            writer.AppendToChunk(nodePrefix);
            std::vector<uint32_t> childIds;
            for (const auto& child : node->children) {
                childIds.push_back(nodeIds[child.get()]);
            }
            writer.AppendToChunk(sizeof(uint32_t) * childIds.size(), childIds.data());
        }
    }
    writer.EndChunk();
}

size_t Scene::InstanceCount() const
{
    size_t count = 0;
    for (const auto& rootNodes : rootNodes) {
        count += rootNodes->MeshInstanceCount();
    }
    return count;
}

Vertex Scene::Min() const
{
    return minExtends_;
}

Vertex Scene::Max() const
{
    return maxExtends_;
}

Vertex Scene::MinTransformed() const
{
    return minExtendsTransformed_;
}

Vertex Scene::MaxTransformed() const
{
    return maxExtendsTransformed_;
}

void Scene::RecalculateMinMax()
{
    minExtends_            = math::Max<float, 3>();
    maxExtends_            = math::Min<float, 3>();
    minExtendsTransformed_ = math::Max<float, 3>();
    maxExtendsTransformed_ = math::Min<float, 3>();

    for (const auto& mesh : meshes) {
        minExtends_ = min(minExtends_, mesh->Min());
        maxExtends_ = max(maxExtends_, mesh->Max());
    }
    for (const auto& root : rootNodes) {
        CalcualtedTransformedExtends(root.get());
    }
}

void Scene::OverrideMeshColors(Float3 meshColor)
{
    OverrideMeshColors([c = meshColor](const Node* const _) { return c; });
}

void Scene::OverrideMeshColors(std::function<Float3(const Node* const)> colorFunc)
{
    for (const auto& node : rootNodes) {
        OverrideColor(node.get(), colorFunc);
    }
}

void Scene::CalcualtedTransformedExtends(Scene::Node* node, Matrix4x4 matrixStack)
{
    // may be a bit broken investigate later if needed
    const auto transform = mul(matrixStack, node->matrix);
    if (node->mesh.has_value()) {
        Mesh* mesh             = *node->mesh;
        minExtendsTransformed_ = min(minExtendsTransformed_, mul(transform, mesh->Min()));
        maxExtendsTransformed_ = min(maxExtendsTransformed_, mul(transform, mesh->Max()));
    }
    for (auto child : node->children) {
        CalcualtedTransformedExtends(child.get(), transform);
    }
}