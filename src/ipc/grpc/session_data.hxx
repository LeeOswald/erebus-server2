#pragma once

#include <erebus/system/erebus.hxx>

#include <atomic>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Erp
{


template <typename KeyT, typename DataT>
    requires std::is_default_constructible_v<DataT>
class SessionData final
    : public boost::noncopyable
{
private:
    struct DataWrapper;

public:
    using KeyType = KeyT;
    using DataType = DataT;

    explicit SessionData(std::chrono::seconds inactivityThreshold)
        : InactivityThreshold(inactivityThreshold)
    {}
    
    struct Ref
    {
        ~Ref()
        {
            if (w)
            {
                w->touched = std::chrono::steady_clock::now();
                --w->refs;
            }
        }

        constexpr Ref(DataWrapper* w = nullptr) noexcept
            : w(w)
        {
        }

        Ref(const Ref&) = delete;
        Ref& operator=(const Ref&) = delete;

        constexpr void swap(Ref& other) noexcept
        {
            using std::swap;
            swap(w, other.w);
        }

        constexpr Ref(Ref&& other) noexcept
            : Ref()
        {
            swap(other);
        }

        constexpr Ref& operator=(Ref&& other) noexcept
        {
            Ref tmp(std::move(other));
            swap(tmp);
            return *this;
        }

        constexpr operator bool() const noexcept
        {
            return !!w;
        }

        constexpr DataType& get() noexcept
        {
            ErAssert(w);
            return w->cookie;
        }

        constexpr void detach() noexcept
        {
            w = nullptr;
        }

    private:
        DataWrapper* w;
    };

    Ref get(const KeyType& key)
    {
        auto lock = [](DataWrapper* w, bool touch) -> Ref
        {
            long expected = 0;
            if (w->refs.compare_exchange_strong(expected, 1, std::memory_order_acq_rel))
            {
                if (touch)
                    w->touched = std::chrono::steady_clock::now();
                
                return Ref(w);
            }

            // someone has already locked this cookie
            return Ref();
        };

        // fast path
        {
            std::shared_lock l(m_mutex);
            auto it = m_cookies.find(key);
            if (it != m_cookies.end())
            {
                auto w = it->second.get();
                return lock(w, true);
            }
        }

        // slow path
        auto now = std::chrono::steady_clock::now();
        
        {
            std::unique_lock l(m_mutex);

            // drop stale cookies
            if (m_prevStaleCheck + InactivityThreshold < now)
            {
                for (auto it = m_cookies.begin(); it != m_cookies.end(); )
                {
                    if (it->first == key) [[unlikely]]
                    {
                        ++it;
                        continue;
                    }

                    auto w = it->second.get();
                    auto locked = lock(w, false);
                    if (locked && (w->touched + InactivityThreshold < now))
                    {
                        // drop it
                        auto next = std::next(it);
                        m_cookies.erase(it);
                        it = next;
                    }
                    else
                    {
                        ++it;
                    }
                }

                m_prevStaleCheck = now;
            }

            // maybe the key have been inserted since the above check
            auto it = m_cookies.find(key);
            if (it != m_cookies.end())
            {
                auto w = it->second.get();
                return lock(w, true);
            }

            // insert a new cookie
            auto inserted = m_cookies.insert({ key, std::make_unique<DataWrapper>() });
            auto w = inserted.first->second.get();
            return lock(w, inserted.second); // only update touch if reusing the existing item
        }
    }

private:
    struct DataWrapper
        : public boost::noncopyable
    {
        DataWrapper() = default;

        DataType cookie;
        std::atomic<long> refs = 0;
        std::chrono::steady_clock::time_point touched = std::chrono::steady_clock::now();
    };

    const std::chrono::seconds InactivityThreshold;
        
    std::shared_mutex m_mutex;
    std::chrono::steady_clock::time_point m_prevStaleCheck = std::chrono::steady_clock::now();
    std::unordered_map<KeyType, std::unique_ptr<DataWrapper>> m_cookies;
};


} // namespace Erp {}
