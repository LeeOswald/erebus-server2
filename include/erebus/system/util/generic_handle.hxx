#pragma once

#include <erebus/system/erebus.hxx>

#if ER_WINDOWS
    #include <erebus/system/system/unwindows.h>
#elif ER_POSIX
    #include <unistd.h>
#endif


namespace Er::Util
{

template <typename HandleT, typename BaseT, BaseT InvalidV, typename DeleterT>
class GenericHandle final
{
    static_assert(sizeof(BaseT) == sizeof(HandleT));

public:
    using Deleter = DeleterT;

    static constexpr const HandleT InvalidHandle = HandleT(InvalidV);

    ~GenericHandle()
    {
        reset();
    }

    constexpr GenericHandle(HandleT h = InvalidHandle) noexcept
        : m_h(h)
    {
    }

    GenericHandle(const GenericHandle&) = delete;
    GenericHandle& operator=(const GenericHandle&) = delete;

    GenericHandle(GenericHandle&& o) noexcept
        : GenericHandle()
    {
        using std::swap;
        swap(m_h, o.m_h);
    }

    GenericHandle& operator=(GenericHandle&& o) noexcept
    {
        GenericHandle tmp(std::move(o));
        using std::swap;
        swap(m_h, tmp.m_h);
        return *this;
    }

    [[nodiscard]] constexpr operator HandleT() const noexcept
    {
        return m_h;
    }

    [[nodiscard]] constexpr HandleT get() const noexcept
    {
        return m_h;
    }

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return (m_h != InvalidHandle);
    }

    [[nodiscard]] constexpr HandleT* writeable() noexcept
    {
        ErAssert(!valid());
        return &m_h;
    }

    void reset(HandleT h = InvalidHandle) noexcept
    {
        if (valid())
        {
            Deleter d;
            d(m_h);
        }

        m_h = h;
    }

private:
    HandleT m_h = InvalidHandle;
};


#if ER_POSIX

namespace __
{

struct FileCloser
{
    void operator()(int fd) noexcept
    {
        ::close(fd);
    }
};

} // namespace __ {}

using FileHandle = GenericHandle<int, int, -1, __::FileCloser>;

#elif ER_WINDOWS

namespace __
{

struct FileCloser
{
    void operator()(HANDLE fd) noexcept
    {
        ::CloseHandle(fd);
    }
};

} // namespace __ {}

using FileHandle = GenericHandle<HANDLE, intptr_t, -1, __::FileCloser>;

#endif


} // namespace Er::Util {}