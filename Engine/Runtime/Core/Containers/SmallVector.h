/// @file
/// @brief Inline-storage vector. Holds up to N elements in an aligned in-class buffer and
///        falls back to a heap allocation through std::allocator when more capacity is
///        needed. Drop-in replacement for std::vector<T> on hot paths that almost always
///        stay within N (command list barrier translation, draw filter chains, viewport
///        / scissor binds), eliminating a heap allocation per call.

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

namespace Hylux
{

/// @brief Vector with N inline element slots; promotes to a heap allocation on overflow.
///        Single-threaded by design.
template<typename T, std::size_t N>
class SmallVector
{
    static_assert(N >= 1, "SmallVector<T, N>: N must be >= 1");

public:
    using value_type      = T;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = T&;
    using const_reference = const T&;
    using pointer         = T*;
    using const_pointer   = const T*;
    using iterator        = T*;
    using const_iterator  = const T*;

    SmallVector() noexcept : data_(InlineData()), capacity_(N) {}

    explicit SmallVector(size_type count) : SmallVector()
    {
        Resize(count);
    }

    SmallVector(size_type count, const T& value) : SmallVector()
    {
        Reserve(count);
        for (size_type i = 0; i < count; ++i)
        {
            ::new (static_cast<void*>(data_ + i)) T(value);
        }
        size_ = count;
    }

    template<typename InputIt,
             typename = std::enable_if_t<std::is_convertible_v<
                 typename std::iterator_traits<InputIt>::iterator_category, std::input_iterator_tag>>>
    SmallVector(InputIt first, InputIt last) : SmallVector()
    {
        for (; first != last; ++first)
        {
            EmplaceBack(*first);
        }
    }

    SmallVector(std::initializer_list<T> values) : SmallVector(values.begin(), values.end()) {}

    SmallVector(const SmallVector& other) : SmallVector()
    {
        Reserve(other.size_);
        for (size_type i = 0; i < other.size_; ++i)
        {
            ::new (static_cast<void*>(data_ + i)) T(other.data_[i]);
        }
        size_ = other.size_;
    }

    SmallVector(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : SmallVector()
    {
        MoveFrom(std::move(other));
    }

    SmallVector& operator=(const SmallVector& other)
    {
        if (this != &other)
        {
            Clear();
            Reserve(other.size_);
            for (size_type i = 0; i < other.size_; ++i)
            {
                ::new (static_cast<void*>(data_ + i)) T(other.data_[i]);
            }
            size_ = other.size_;
        }
        return *this;
    }

    SmallVector& operator=(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>
                                                        && std::is_nothrow_destructible_v<T>)
    {
        if (this != &other)
        {
            Clear();
            ReleaseHeapIfAny();
            data_     = InlineData();
            capacity_ = N;
            MoveFrom(std::move(other));
        }
        return *this;
    }

    ~SmallVector()
    {
        Clear();
        ReleaseHeapIfAny();
    }

    [[nodiscard]] iterator       begin() noexcept       { return data_; }
    [[nodiscard]] const_iterator begin() const noexcept { return data_; }
    [[nodiscard]] iterator       end() noexcept         { return data_ + size_; }
    [[nodiscard]] const_iterator end() const noexcept   { return data_ + size_; }
    [[nodiscard]] const_iterator cbegin() const noexcept { return data_; }
    [[nodiscard]] const_iterator cend() const noexcept   { return data_ + size_; }

    [[nodiscard]] size_type size() const noexcept     { return size_; }
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool      empty() const noexcept    { return size_ == 0; }
    [[nodiscard]] bool      IsInline() const noexcept { return data_ == InlineData(); }
    [[nodiscard]] static constexpr size_type inline_capacity() noexcept { return N; }

    [[nodiscard]] pointer       data() noexcept       { return data_; }
    [[nodiscard]] const_pointer data() const noexcept { return data_; }

    [[nodiscard]] reference operator[](size_type index) noexcept
    {
        assert(index < size_ && "SmallVector::operator[]: index out of range");
        return data_[index];
    }
    [[nodiscard]] const_reference operator[](size_type index) const noexcept
    {
        assert(index < size_ && "SmallVector::operator[]: index out of range");
        return data_[index];
    }

    [[nodiscard]] reference       front() noexcept       { assert(size_ > 0); return data_[0]; }
    [[nodiscard]] const_reference front() const noexcept { assert(size_ > 0); return data_[0]; }
    [[nodiscard]] reference       back() noexcept        { assert(size_ > 0); return data_[size_ - 1]; }
    [[nodiscard]] const_reference back() const noexcept  { assert(size_ > 0); return data_[size_ - 1]; }

    void Reserve(size_type newCapacity)
    {
        if (newCapacity <= capacity_)
        {
            return;
        }
        Reallocate(newCapacity);
    }

    void Resize(size_type newSize)
    {
        if (newSize > size_)
        {
            Reserve(newSize);
            for (size_type i = size_; i < newSize; ++i)
            {
                ::new (static_cast<void*>(data_ + i)) T();
            }
        }
        else
        {
            for (size_type i = newSize; i < size_; ++i)
            {
                data_[i].~T();
            }
        }
        size_ = newSize;
    }

    void Resize(size_type newSize, const T& value)
    {
        if (newSize > size_)
        {
            Reserve(newSize);
            for (size_type i = size_; i < newSize; ++i)
            {
                ::new (static_cast<void*>(data_ + i)) T(value);
            }
        }
        else
        {
            for (size_type i = newSize; i < size_; ++i)
            {
                data_[i].~T();
            }
        }
        size_ = newSize;
    }

    void Clear() noexcept
    {
        for (size_type i = 0; i < size_; ++i)
        {
            data_[i].~T();
        }
        size_ = 0;
    }

    template<typename U>
    reference PushBack(U&& value)
    {
        return EmplaceBack(std::forward<U>(value));
    }

    template<typename... Args>
    reference EmplaceBack(Args&&... args)
    {
        if (size_ == capacity_)
        {
            Reallocate(capacity_ * 2);
        }
        T* slot = ::new (static_cast<void*>(data_ + size_)) T(std::forward<Args>(args)...);
        ++size_;
        return *slot;
    }

    void PopBack() noexcept
    {
        assert(size_ > 0 && "SmallVector::PopBack: empty");
        --size_;
        data_[size_].~T();
    }

    iterator Erase(const_iterator position)
    {
        return Erase(position, position + 1);
    }

    iterator Erase(const_iterator first, const_iterator last)
    {
        assert(first >= begin() && last <= end() && first <= last);
        const size_type startIndex = static_cast<size_type>(first - begin());
        const size_type endIndex   = static_cast<size_type>(last - begin());
        const size_type tail       = size_ - endIndex;
        for (size_type i = 0; i < tail; ++i)
        {
            data_[startIndex + i] = std::move(data_[endIndex + i]);
        }
        for (size_type i = startIndex + tail; i < size_; ++i)
        {
            data_[i].~T();
        }
        size_ -= (endIndex - startIndex);
        return data_ + startIndex;
    }

    /// @brief Drops the heap allocation when the vector currently fits inline. Elements
    ///        remain valid. When capacity already lives inline this is a no-op.
    void ShrinkToFit()
    {
        if (IsInline() || size_ > N)
        {
            return;
        }
        T* heap = data_;
        data_   = InlineData();
        for (size_type i = 0; i < size_; ++i)
        {
            ::new (static_cast<void*>(data_ + i)) T(std::move(heap[i]));
            heap[i].~T();
        }
        std::allocator<T>().deallocate(heap, capacity_);
        capacity_ = N;
    }

    // STL-style lowercase aliases for range-based loops and generic code.
    [[nodiscard]] iterator       Begin() noexcept       { return begin(); }
    [[nodiscard]] const_iterator Begin() const noexcept { return begin(); }
    [[nodiscard]] iterator       End() noexcept         { return end(); }
    [[nodiscard]] const_iterator End() const noexcept   { return end(); }
    [[nodiscard]] size_type      Size() const noexcept  { return size_; }
    [[nodiscard]] bool           Empty() const noexcept { return size_ == 0; }
    [[nodiscard]] pointer        Data() noexcept        { return data_; }
    [[nodiscard]] const_pointer  Data() const noexcept  { return data_; }

private:
    [[nodiscard]] pointer InlineData() noexcept
    {
        return reinterpret_cast<pointer>(&inlineBuffer_);
    }
    [[nodiscard]] const_pointer InlineData() const noexcept
    {
        return reinterpret_cast<const_pointer>(&inlineBuffer_);
    }

    void Reallocate(size_type requested)
    {
        const size_type newCapacity = requested < N ? N : requested;
        if (newCapacity == capacity_)
        {
            return;
        }
        T* fresh = std::allocator<T>().allocate(newCapacity);
        for (size_type i = 0; i < size_; ++i)
        {
            ::new (static_cast<void*>(fresh + i)) T(std::move_if_noexcept(data_[i]));
            data_[i].~T();
        }
        ReleaseHeapIfAny();
        data_     = fresh;
        capacity_ = newCapacity;
    }

    void ReleaseHeapIfAny() noexcept
    {
        if (data_ != InlineData() && capacity_ > 0 && data_ != nullptr)
        {
            std::allocator<T>().deallocate(data_, capacity_);
        }
    }

    void MoveFrom(SmallVector&& other) noexcept(std::is_nothrow_move_constructible_v<T>)
    {
        if (!other.IsInline())
        {
            data_           = other.data_;
            size_           = other.size_;
            capacity_       = other.capacity_;
            other.data_     = other.InlineData();
            other.size_     = 0;
            other.capacity_ = N;
        }
        else
        {
            for (size_type i = 0; i < other.size_; ++i)
            {
                ::new (static_cast<void*>(data_ + i)) T(std::move(other.data_[i]));
            }
            size_ = other.size_;
            other.Clear();
        }
    }

    alignas(T) std::byte inlineBuffer_[sizeof(T) * N];
    pointer              data_;
    size_type            size_{0};
    size_type            capacity_;
};

} // namespace Hylux
