/* Copyright (c) 2023 Pirmin Pfeifer */
#include "Mesh.h"

#include <rayvis-utils/BreakAssert.h>

namespace {

    class VertexAccessor {
    public:
        VertexAccessor() = default;

        VertexAccessor(std::span<Vertex> vertexBuffer)
        {
            bufferView_ = vertexBuffer.data();
            length_     = vertexBuffer.size();
            byteLength_ = vertexBuffer.size_bytes();
        }

        const Vertex& operator[](size_t idx) const
        {
            Assert(idx < length_);
            return *(bufferView_ + idx);
        }

        const Vertex* data() const
        {
            Assert(bufferView_ != nullptr);
            return bufferView_;
        }

        size_t length() const
        {
            Assert(length_ != 0);
            return length_;
        }

        size_t byteLength() const
        {
            Assert(byteLength_ != 0);
            return byteLength_;
        }

        DXGI_FORMAT format() const
        {
            return DXGI_FORMAT_R32G32B32_FLOAT;
        }

    private:
        const Float3* bufferView_ = nullptr;
        size_t        length_     = 0;
        size_t        byteLength_ = 0;
    };

    class IndexAccessor {
    public:
        IndexAccessor() = default;

        IndexAccessor(std::span<UINT> indexBuffer)
        {
            format_     = DXGI_FORMAT_R32_UINT;
            length_     = indexBuffer.size();
            byteLength_ = indexBuffer.size_bytes();

            data_ = reinterpret_cast<const uint16_t*>(indexBuffer.data());
        }

        IndexAccessor(std::span<UINT16> indexBuffer)
        {
            format_     = DXGI_FORMAT_R16_UINT;
            length_     = indexBuffer.size();
            byteLength_ = indexBuffer.size_bytes();

            data_ = indexBuffer.data();
        }

        uint32_t operator[](size_t idx) const
        {
            Assert(idx < length_);
            if (format_ == DXGI_FORMAT_R16_UINT) {
                return *(data_ + idx);
            } else {
                // DXGI_FORMAT_R32_UINT
                return *(reinterpret_cast<const uint32_t*>(data_ + idx * 2));
            }
            return 0;
        }

        const void* data() const
        {
            Assert(data_ != nullptr);
            return data_;
        }

        size_t length() const
        {
            Assert(length_ != 0);
            return length_;
        }

        size_t byteLength() const
        {
            Assert(byteLength_ != 0);
            return byteLength_;
        }

        DXGI_FORMAT format() const
        {
            Assert(format_ != DXGI_FORMAT_UNKNOWN);
            return format_;
        }

    private:
        const uint16_t* data_       = nullptr;
        size_t          length_     = 0;
        size_t          byteLength_ = 0;
        DXGI_FORMAT     format_     = DXGI_FORMAT_UNKNOWN;

        std::vector<uint16_t> generatedIndices_;
    };

    std::vector<Float3> GenerateWeightedNormals(const VertexAccessor& vAccess, const IndexAccessor& iAccess)
    {
        std::vector<Float3> normals(vAccess.length());

        Assert(iAccess.length() % 3 == 0);
        for (size_t i = 0; i < (iAccess.length() / 3); i++) {
            const auto aIdx = iAccess[(i * 3) + 0];
            const auto bIdx = iAccess[(i * 3) + 1];
            const auto cIdx = iAccess[(i * 3) + 2];

            const Vertex& a = vAccess[aIdx];
            const Vertex& b = vAccess[bIdx];
            const Vertex& c = vAccess[cIdx];
            const Vertex  u = b - a;
            const Vertex  v = c - a;

            const Vertex weightedNormal = linalg::cross(u, v) * -0.5f;

            normals[aIdx] += weightedNormal;
            normals[bIdx] += weightedNormal;
            normals[cIdx] += weightedNormal;
        }

        std::transform(
            normals.begin(), normals.end(), normals.begin(), [](const Float3& n) { return linalg::normalize(n); });
        return normals;
    };
}  // namespace

Mesh::Mesh(ComPtr<ID3D12Device5> device, std::span<Vertex> vertexBuffer, IndexBufferVariant indexBuffer)
{
    std::vector<std::pair<std::span<Vertex>, std::variant<std::span<UINT>, std::span<UINT16>>>> packed = {};
    std::pair<std::span<Vertex>, std::variant<std::span<UINT>, std::span<UINT16>>> pack(vertexBuffer, indexBuffer);
    packed.push_back(pack);
    Init(device, packed);
}

Mesh::Mesh(ComPtr<ID3D12Device5> device, std::span<PrimitivePack> primitives)
{
    Init(device, primitives);
}

Mesh::Mesh(ComPtr<ID3D12Device5> device, std::span<PrimitivePackV> primitives)
{
    std::vector<PrimitivePack> packed;
    for (auto& prim : primitives) {
        if (std::holds_alternative<std::vector<UINT>>(prim.second)) {
            packed.push_back(PrimitivePack(prim.first, std::get<std::vector<UINT>>(prim.second)));
        } else {
            packed.push_back(PrimitivePack(prim.first, std::get<std::vector<UINT16>>(prim.second)));
        }
    }
    Init(device, packed);
}

D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS Mesh::GetBlasInput(
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags)
{
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS blasInput;
    blasInput.Type           = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blasInput.DescsLayout    = D3D12_ELEMENTS_LAYOUT_ARRAY;
    blasInput.Flags          = flags;
    blasInput.NumDescs       = primitives_.size();
    blasInput.pGeometryDescs = descriptions_.data();

    return blasInput;
}

D3D12_RAYTRACING_GEOMETRY_DESC Mesh::GetDes(size_t primitiveId, D3D12_GPU_VIRTUAL_ADDRESS transform)
{
    Assert(primitiveId < primitives_.size());
    D3D12_RAYTRACING_GEOMETRY_DESC geometryDesc       = {};
    geometryDesc.Type                                 = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    geometryDesc.Triangles.IndexBuffer                = primitives_[primitiveId].indexBuffer_->GetGPUVirtualAddress();
    geometryDesc.Triangles.IndexCount                 = static_cast<UINT>(IndexCount(primitiveId));
    geometryDesc.Triangles.IndexFormat                = primitives_[primitiveId].indexFormat_;
    geometryDesc.Triangles.Transform3x4               = transform;
    geometryDesc.Triangles.VertexFormat               = primitives_[primitiveId].vertexFormat_;
    geometryDesc.Triangles.VertexCount                = static_cast<UINT>(VertexCount(primitiveId));
    geometryDesc.Triangles.VertexBuffer.StartAddress  = primitives_[primitiveId].vertexBuffer_->GetGPUVirtualAddress();
    geometryDesc.Triangles.VertexBuffer.StrideInBytes = VertexByteSize();
    geometryDesc.Flags                                = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;

    return geometryDesc;
}

size_t Mesh::PrimitiveCount() const
{
    return primitives_.size();
}

size_t Mesh::VertexByteSize(size_t primitiveId) const
{
    Assert(primitiveId < primitives_.size());
    size_t result = 0;
    switch (primitives_[primitiveId].vertexFormat_) {
    case DXGI_FORMAT_R32G32B32_FLOAT:
        result = sizeof(float) * 3;
        break;
    case DXGI_FORMAT_R32G32B32A32_FLOAT:
        result = sizeof(float) * 4;
        break;
    default:
        Assert(false);
    }
    return result;
}

size_t Mesh::IndexByteSize(size_t primitiveId) const
{
    Assert(primitiveId < primitives_.size());
    size_t result = 0;
    switch (primitives_[primitiveId].indexFormat_) {
    case DXGI_FORMAT_R32_UINT:
        result = sizeof(UINT);
        break;
    case DXGI_FORMAT_R16_UINT:
        result = sizeof(UINT16);
        break;
    default:
        Assert(false);
    }
    return result;
}

size_t Mesh::VertexCount(size_t primitiveId)
{
    const auto width    = primitives_[primitiveId].vertexBuffer_->Width();
    const auto byteSize = VertexByteSize(primitiveId);

    Assert((width % byteSize) == 0);
    return width / byteSize;
}

size_t Mesh::IndexCount(size_t primitiveId)
{
    const auto width    = primitives_[primitiveId].indexBuffer_->Width();
    const auto byteSize = IndexByteSize(primitiveId);

    Assert((width % byteSize) == 0);
    return width / byteSize;
}

size_t Mesh::TriangleCount(size_t primitive)
{
    return VertexCount(primitive) / 3;
}

Vertex Mesh::Min() const
{
    return minExtends_;
}

Vertex Mesh::Max() const
{
    return maxExtends_;
}

std::vector<ID3D12Resource*> Mesh::GetPrimtivieNormalBuffers()
{
    std::vector<ID3D12Resource*> result;
    for (const auto& prim : primitives_) {
        result.push_back(prim.normalBuffer_->Get());
    }
    return result;
}

std::vector<std::pair<DXGI_FORMAT, ID3D12Resource*>> Mesh::GetPrimtivieIndexBuffers()
{
    std::vector<std::pair<DXGI_FORMAT, ID3D12Resource*>> result;
    for (const auto& prim : primitives_) {
        result.push_back(std::pair(prim.indexFormat_, prim.indexBuffer_->Get()));
    }
    return result;
}

void Mesh::Init(ComPtr<ID3D12Device5>                                                                     device,
                std::span<std::pair<std::span<Vertex>, std::variant<std::span<UINT>, std::span<UINT16>>>> primitives)
{
    assert(primitives_.empty());
    for (const auto& prim : primitives) {
        const auto& vertexBuffer    = prim.first;
        const auto& indexBuffer     = prim.second;
        const bool  isFullIndexSize = std::holds_alternative<std::span<UINT>>(indexBuffer);

        Primitive primitive;
        primitive.Init(device);

        primitive.vertexFormat_ = DXGI_FORMAT_R32G32B32_FLOAT;
        primitive.vertexBuffer_->Map<Vertex>(vertexBuffer);

        primitive.indexFormat_ = isFullIndexSize ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
        if (isFullIndexSize) {
            primitive.indexBuffer_->Map<UINT>(std::get<std::span<UINT>>(indexBuffer));
        } else {
            primitive.indexBuffer_->Map<UINT16>(std::get<std::span<UINT16>>(indexBuffer));
        }

        IndexAccessor       indexAccessor = isFullIndexSize ? IndexAccessor(std::get<std::span<UINT>>(indexBuffer))
                                                            : IndexAccessor(std::get<std::span<UINT16>>(indexBuffer));
        std::vector<Vertex> normals       = GenerateWeightedNormals(VertexAccessor(vertexBuffer), indexAccessor);
        primitive.normalBuffer_->Map<Vertex>(normals);

        primitives_.push_back(primitive);
        descriptions_.push_back(GetDes(primitives_.size() - 1, 0));

        VertexAccessor vAccess(vertexBuffer);
        for (int i = 0; i < vAccess.length(); ++i) {
            minExtends_ = linalg::min(minExtends_, vAccess[i]);
            maxExtends_ = linalg::max(maxExtends_, vAccess[i]);
        }
    }
}

void Mesh::Primitive::Init(ComPtr<ID3D12Device5> device)
{
    indexBuffer_  = std::make_shared<UploadBuffer>(device);
    vertexBuffer_ = std::make_shared<UploadBuffer>(device);
    normalBuffer_ = std::make_shared<UploadBuffer>(device);
}