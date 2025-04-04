#pragma once

#include <erebus/system/erebus.hxx>

#if ER_DEBUG
    #include <atomic>
#endif

namespace Er::Util
{

struct NullMutex
{
    constexpr NullMutex() noexcept = default;

    void lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
    }

    bool try_lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
        return true;
    }

    void unlock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_sub(1, std::memory_order_relaxed) == 1);
#endif
    }

#if ER_DEBUG
private:
    std::atomic<long> m_locks = 0; // check if we're really running single-threaded
#endif
};


struct NullSharedMutex
{
    constexpr NullSharedMutex() noexcept = default;

    void lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
    }

    bool try_lock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
        return true;
    }

    void unlock() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_sub(1, std::memory_order_relaxed) == 1);
#endif
    }

    void lock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
    }

    bool try_lock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_add(1, std::memory_order_relaxed) == 0);
#endif
        return true;
    }

    void unlock_shared() noexcept
    {
#if ER_DEBUG
        ErAssert(m_locks.fetch_sub(1, std::memory_order_relaxed) == 1);
#endif
    }

#if ER_DEBUG
private:
    std::atomic<long> m_locks = 0; // check if we're really running single-threaded
#endif
};

} // namespace Er::Util {}