#pragma once

#include <span>

namespace core {

    template <typename ValueType, typename IndexType = std::size_t>
        requires(std::convertible_to<std::size_t, IndexType>)
    struct IndexedIterator {
        struct IndexedValuePair {
            IndexType  index;
            ValueType& value;
        };

        struct Iterator {
        public:
            using iterator_category = std::forward_iterator_tag;
            using difference_type   = std::ptrdiff_t;
            using value_type        = IndexedValuePair;

            Iterator(std::span<ValueType>::iterator begin, std::span<ValueType>::iterator position)
                : begin_(begin), iterator_(position)
            {
            }

            Iterator() {}

            Iterator& operator++()
            {
                ++iterator_;
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator temp = *this;
                ++iterator_;
                return temp;
            }

            IndexedValuePair operator*() const
            {
                return {static_cast<IndexType>(std::distance(begin_, iterator_)), *iterator_};
            }

            auto operator==(const Iterator& other) const
            {
                return iterator_ == other.iterator_;
            }

        private:
            std::span<ValueType>::iterator begin_;
            std::span<ValueType>::iterator iterator_;
        };

        IndexedIterator(const std::span<ValueType> data) : data_(data) {}

        Iterator begin() const
        {
            return Iterator(data_.begin(), data_.begin());
        }

        Iterator end() const
        {
            return Iterator(data_.begin(), data_.end());
        }

    private:
        std::span<ValueType> data_;
    };

}  // namespace core