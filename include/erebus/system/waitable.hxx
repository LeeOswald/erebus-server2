#pragma once

#include <erebus/system/erebus.hxx>

#include <condition_variable>
#include <mutex>

#include <boost/noncopyable.hpp>


namespace Er
{


//
// Condition variable paired with a value that can be waited for
//

template <typename ValueT>
class Waitable final
    : public boost::noncopyable
{
public:
    using ValueType = ValueT;

    explicit Waitable(ValueType initial = ValueType{})
        : m_value(initial)
        , m_mutex()
        , m_cv()
    {
    }

    ValueType get() const noexcept
    {
        return m_value.load();
    }

    void notifyOne() noexcept
    {
        m_cv.notify_one();
    }

    void notifyAll() noexcept
    {
        m_cv.notify_all();
    }

    void set(ValueType val) noexcept
    {
        // lock is required to avoid a data race between calls to
        // get() when called in wait() through a predicate.
        std::unique_lock lock(m_mutex);

        m_value.store(val);
    }

    void setAndNotifyOne(ValueType val) noexcept
    {
        set(std::move(val));
        m_cv.notify_one();
    }

    void setAndNotifyAll(ValueType val) noexcept
    {
        set(std::move(val));
        m_cv.notify_all();
    }

    template <typename... Args>
    auto wait(Args&&... args) const
    {
        std::unique_lock lock(m_mutex);

        return m_cv.wait(lock, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto waitValue(ValueType value, Args&&... args) const
    {
        std::unique_lock lock(m_mutex);
        
        return m_cv.wait(
            lock,
            std::forward<Args>(args)...,
            [this, &value]()
            {
                return m_value.load() == value;
            }
        );
    }

    template <typename... Args>
    auto waitFor(Args&&... args) const
    {
        std::unique_lock lock(m_mutex);
        
        return m_cv.wait_for(lock, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto waitValueFor(ValueType value, Args&&... args) const
    {
        std::unique_lock lock(m_mutex);

        return m_cv.wait_for(
            lock,
            std::forward<Args>(args)...,
            [this, &value]()
            {
                return m_value.load() == value;
            }
        );
    }

    template <typename... Args>
    auto waitUntil(Args&&... args) const
    {
        std::unique_lock lock(m_mutex);
        
        return m_cv.wait_until(lock, std::forward<Args>(args)...);
    }

    template <typename... Args>
    auto waitValueUntil(ValueType value, Args&&... args) const
    {
        std::unique_lock lock(m_mutex);

        return m_cv.wait_until(
            lock,
            std::forward<Args>(args)...,
            [this, &value]()
            {
                return m_value.load() == value;
            }
        );
    }

private:
    std::atomic<ValueType> m_value;
    mutable std::mutex m_mutex;
    mutable std::condition_variable m_cv;
};


template <typename ValueType>
Waitable(ValueType) -> Waitable<ValueType>;




} // namespace Er {}