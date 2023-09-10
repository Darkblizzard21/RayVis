/* Copyright (c) 2023 Pirmin Pfeifer */
#pragma once
#include <spdlog/fmt/fmt.h>

#include <cassert>
#include <concepts>
#include <limits>
#include <stdexcept>

#ifdef max
#undef max
#endif
#include <source_location>
template <typename T>
concept IntBoolArrayUnderlying =
    std::same_as<T, uint8_t> || std::same_as<T, uint16_t> || std::same_as<T, uint32_t> || std::same_as<T, uint64_t>;

template <IntBoolArrayUnderlying T>
struct intBools {
private:
    T bools = 0;

public:
    void Set(int32_t index, bool value)
    {
        assert(0 <= index && index < Count());
        const T bitMask = (static_cast<T>(1) << index);
        if (value) {
            bools = bools | bitMask;
        } else {
            bools = bools & (std::numeric_limits<T>::max() - bitMask);
        }
    }

    bool Check(int32_t index) const
    {
        assert(0 <= index && index < Count());
        const T bitMask = (static_cast<T>(1) << index);
        return 0 < (bools & bitMask);
    }

    bool operator[](int32_t idx) const
    {
        return Check(idx);
    }

    void SetAll(bool value)
    {
        bools = value ? std::numeric_limits<T>::max() : 0;
    }

    /// number of bool == size in bit
    int32_t Count() const
    {
        return sizeof(T) * 8;
    }

    T RawValue() const
    {
        return bools;
    }
};

typedef intBools<uint8_t>  bool8;
typedef intBools<uint16_t> bool16;
typedef intBools<uint32_t> bool32;
typedef intBools<uint64_t> bool64;

class BufferReader {
private:
    const int64_t  initialSize;
    int64_t        remainingSize_;
    const uint8_t* data_;

public:
    BufferReader(int64_t dataSize, const void* data)
        : initialSize(dataSize), remainingSize_(dataSize), data_(reinterpret_cast<const uint8_t*>(data))
    {
        if (sizeof(uint8_t) != 1) {
            throw std::runtime_error("Expected \"sizeof(uint8_t) == 1\" but it was not true");
        }
    }

    template <typename T>
    void read(T& item, std::source_location sourceLoc = std::source_location::current())
    {
        read(reinterpret_cast<void*>(&item), sizeof(item), sourceLoc);
    }

    void read(void* dest, size_t size, std::source_location sourceLoc = std::source_location::current())
    {
        if (remainingSize_ < size) {
            throw std::runtime_error(
                fmt::format("Remaining size ({}) is smaller than required size ({}) - {} at {}:{} in {}",
                            remainingSize_,
                            size,
                            sourceLoc.file_name(),
                            sourceLoc.line(),
                            sourceLoc.column(),
                            sourceLoc.function_name()));
        }
        std::memcpy(dest, data_, size);
        data_ += size;
        remainingSize_ -= size;
    }

    inline bool empty() const
    {
        return remainingSize_ == 0;
    }

    inline int64_t remainingBytes()
    {
        return remainingSize_;
    }

    /// returns progress between 0.0 and 1.0
    inline double progress()
    {
        assert(remainingSize_ <= initialSize);
        const double readSize  = initialSize - remainingSize_;
        const double totalSize = initialSize;
        return readSize / totalSize;
    }
};

//https://stackoverflow.com/a/51695896
template<typename F>
struct Lazy {
    F f; 

    operator decltype(f())() const
    {
        return f();
    }
};

template <typename F>
Lazy(F f) -> Lazy<F>;