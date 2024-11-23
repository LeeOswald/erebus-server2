#pragma once

#include <erebus/system/erebus.hxx>

namespace Er::Util
{

///
/// sometimes we need an unique_ptr<> that can be used as an out parameter of a function
/// this is required by a huge number of C-style APIs
///

template <typename T, typename DeleterT>
struct AutoPtr final
{
    using Deleter = DeleterT;
    using Value = T;
    using Pointer = Value*;
    using ConstPointer = const Value*;

    ~AutoPtr()
    {
        reset();
    }

    constexpr explicit AutoPtr(Pointer ptr = nullptr) noexcept
        : m_ptr(ptr)
    {
    }

    AutoPtr(const AutoPtr&) = delete;
    AutoPtr& operator=(const AutoPtr&) = delete;

    AutoPtr(AutoPtr&& o) noexcept
        : AutoPtr()
    {
        std::swap(m_ptr, o.m_ptr);
    }

    AutoPtr& operator=(AutoPtr&& o) noexcept
    {
        AutoPtr tmp(std::move(o));
        std::swap(m_ptr, tmp.m_ptr);
        return *this;
    }

    [[nodiscard]] constexpr operator bool() const noexcept
    {
        return !!m_ptr;
    }

    [[nodiscard]] constexpr operator ConstPointer() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr operator Pointer() noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr ConstPointer get() const noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr Pointer get() noexcept
    {
        return m_ptr;
    }

    [[nodiscard]] constexpr ConstPointer operator->() const noexcept
    {
        ErAssert(m_ptr);
        return m_ptr;
    }

    [[nodiscard]] constexpr Pointer operator->() noexcept
    {
        ErAssert(m_ptr);
        return m_ptr;
    }

    [[nodiscard]] Pointer* writeable() noexcept
    {
        ErAssert(!m_ptr);
        return &m_ptr;
    }

    void reset(Pointer ptr = nullptr)
    {
        if (m_ptr)
        {
            Deleter d;
            d(m_ptr);
        }

        m_ptr = ptr;
    }

private:
    Pointer m_ptr = nullptr;
};


} // namespace Er::Util {}
