#pragma once

#include <core-utils/Exceptions.h>
#include <core-utils/StringUtils.h>

#include <cassert>
#include <functional>
#include <span>
#include <string_view>
#include <vector>

namespace core {

    template <typename S, typename T>
    void PrefixMatchInsert(
        std::span<S>                              source,
        std::vector<T>&                           target,
        std::function<void(T&, S&)>               callback,
        std::function<T(const S&)>                constructor       = [](const S& s) { return T(s); },
        std::function<std::string_view(const S&)> sourceKeyCallback = [](const S& s) { return std::string_view(s); },
        std::function<std::string_view(const T&)> targetKeyCallback = [](const T& t) { return std::string_view(t); })
    {
        auto targetIterator = target.begin();

        for (auto sourceIterator = source.begin(); sourceIterator != source.end(); ++sourceIterator) {
            const auto sourceKey = sourceKeyCallback(*sourceIterator);

            // target list is at end
            if (targetIterator == target.end()) {
                // insert new element at end
                targetIterator = target.insert(targetIterator, constructor(*sourceIterator));
                // process inserted element
                callback(*targetIterator, *sourceIterator);
                // advance target iterator to end again
                ++targetIterator;
                continue;
            }

            // elements are equal
            if (sourceKey == targetKeyCallback(*targetIterator)) {
                // process element
                callback(*targetIterator, *sourceIterator);
                // advance target
                ++targetIterator;
                continue;
            }

            {
                auto matchingSourceIterator = sourceIterator;
                auto matchingTargetIterator = targetIterator;

                for (; matchingSourceIterator != source.end(); ++matchingSourceIterator) {
                    for (matchingTargetIterator = targetIterator; matchingTargetIterator != target.end();
                         ++matchingTargetIterator)
                    {
                        if (sourceKeyCallback(*matchingSourceIterator) == targetKeyCallback(*matchingTargetIterator)) {
                            break;
                        }
                    }

                    if (matchingTargetIterator != target.end()) {
                        break;
                    }
                }

                // matching entry found
                if (matchingSourceIterator == sourceIterator) {
                    assert(matchingTargetIterator != target.end());

                    // set target to found iterator
                    targetIterator = matchingTargetIterator;

                    // process element
                    callback(*targetIterator, *sourceIterator);

                    // advance target
                    ++targetIterator;

                    continue;
                }

                // no entries between current and next matching
                if (matchingTargetIterator == targetIterator) {
                    // insert before current iterator
                    targetIterator = target.insert(targetIterator, constructor(*sourceIterator));
                    // process inserted element
                    callback(*targetIterator, *sourceIterator);
                    // advance target iterator to end again
                    ++targetIterator;
                    continue;
                } else {
                    std::size_t longestPrefixLength = 0;
                    // find longest prefix
                    for (auto longestPrefixIterator = targetIterator; longestPrefixIterator != matchingTargetIterator;
                         ++longestPrefixIterator)
                    {
                        const auto prefix = GetPrefixLength(sourceKey, targetKeyCallback(*longestPrefixIterator));

                        if (prefix > longestPrefixLength) {
                            // set target iterator to insert after longest prefix
                            targetIterator = std::next(longestPrefixIterator);
                        }
                    }

                    // insert after longest prefix
                    targetIterator = target.insert(targetIterator, constructor(*sourceIterator));
                    // process inserted element
                    callback(*targetIterator, *sourceIterator);
                    // advance target iterator to end again
                    ++targetIterator;
                    continue;
                }
            }
        }
    }

    template <typename D, typename S>
    std::span<D> SpanCast(const std::span<S> source)
        requires((((sizeof(D) > sizeof(S)) || ((sizeof(S) % sizeof(D)) == 0))      // D <= S implies (S % D) == 0
                  && ((sizeof(D) <= sizeof(S)) || (sizeof(D) % sizeof(S)) == 0)))  // D > S implies (D % S) == 0
    {
        if constexpr (std::is_const_v<S>) {
            const auto bytes = std::as_bytes(source);

            if ((bytes.size() % sizeof(D)) != 0) {
                throw InvalidArgumentException("source size in bytes must be multiple of destination type size.");
            }

            return std::span<D>(reinterpret_cast<D*>(bytes.data()), bytes.size() / sizeof(D));
        } else {
            const auto bytes = std::as_writable_bytes(source);

            if ((bytes.size() % sizeof(D)) != 0) {
                throw InvalidArgumentException("source size in bytes must be multiple of destination type size.");
            }

            return std::span<D>(reinterpret_cast<D*>(bytes.data()), bytes.size() / sizeof(D));
        }
    }

    struct ByteHash {
        std::size_t operator()(const std::span<const std::byte>& data) const;
    };

    struct ByteEqual {
        std::size_t operator()(const std::span<const std::byte>& a, const std::span<const std::byte>& b) const;
    };

    template <typename T>
    void CopyAsBytes(const T& src, std::span<std::byte> dst)
    {
        if (dst.size() < sizeof(src)) {
            throw InvalidArgumentException("not enough space in destination to copy value.");
        }

        const auto srcBytes = std::as_bytes(std::span<const T>(&src, 1));

        std::copy(srcBytes.begin(), srcBytes.end(), dst.begin());
    }

}  // namespace core