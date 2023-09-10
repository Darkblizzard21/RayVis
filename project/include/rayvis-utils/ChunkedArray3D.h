/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <rayvis-utils/MathTypes.h>

#include <map>

template <typename T>
class ChunkedArray3D final {
public:
    ChunkedArray3D(const size_t chunkSize) : chunkSize(chunkSize){};

     T& At(const Int3& pos)
    {
        assert(0 <= pos.x);
        assert(0 <= pos.y);
        assert(0 <= pos.z);

        const Int3 chunkId(pos.x / chunkSize, pos.y / chunkSize, pos.z / chunkSize);

        if (chunkId != lastChunkId_) {
            auto it = data_.find(chunkId);
            if (it == data_.end()) {
                std::vector<T> newChunk(chunkSize * chunkSize * chunkSize);
                data_[chunkId] = newChunk;
                it             = data_.find(chunkId);
            }

            lastChunkId_ = chunkId;
            lastChunk_   = &it->second;
        }

        const Int3 chunkInternalId(pos.x % chunkSize, pos.y % chunkSize, pos.z % chunkSize);
        return (
            *lastChunk_)[chunkInternalId.x * chunkSize * chunkSize + chunkInternalId.y * chunkSize + chunkInternalId.z];
    }

    inline auto& At(int32_t x, int32_t y, int32_t z)
    {
        return At(Int3(x, y, z));
    }

    inline auto& operator[](const Int3& pos)
    {
        return At(pos);
    }

    const T& At(const Int3& pos) const
    {
        assert(0 <= pos.x);
        assert(0 <= pos.y);
        assert(0 <= pos.z);
        const Int3 chunkId(pos.x / chunkSize, pos.y / chunkSize, pos.z / chunkSize);

        auto it = data_.find(chunkId);
        if (it == data_.end()) {
            return T();
        }
        const Int3 chunkInternalId(pos.x % chunkSize, pos.y % chunkSize, pos.z % chunkSize);
        return it->second[chunkInternalId.x * chunkSize * chunkSize + pos.y * chunkSize + pos.z];
    }

    inline const T& At(int32_t x, int32_t y, int32_t z) const
    {
        return At(Int3(x, y, z));
    }

    inline const T& operator[](const Int3& pos) const
    {
        return At(pos);
    }

    inline std::vector<T> GetChunkFor(const Int3& pos)
    {
        assert(0 <= pos.x);
        assert(0 <= pos.y);
        assert(0 <= pos.z);
        T&         _ = (*this)[pos];
        const Int3 chunkId(pos.x / chunkSize, pos.y / chunkSize, pos.z / chunkSize);
        return data_[chunkId];
    }
     

    const std::map<Int3, std::vector<T>>& GetData() const
    {
        return data_;
    }

    void Apply(std::function<void(T&)> func)
    {
        for (auto& chunk : data_) {
            std::for_each(chunk.second.begin(), chunk.second.end(), func);
        }
    }

    inline size_t ChunkCount() const
    {
        return data_.size();
    }

    inline size_t ChunkSize() const
    {
        return chunkSize;
    }

private:
    const size_t                   chunkSize;
    std::map<Int3, std::vector<T>> data_;

    Int3            lastChunkId_ = Int3(-1, -1, -1);
    std::vector<T>* lastChunk_   = nullptr;
};

inline ChunkedArray3D<uint8_t> USHORT_ARRAY_RAY_VIS()
{
    return ChunkedArray3D<uint8_t>(64);
}