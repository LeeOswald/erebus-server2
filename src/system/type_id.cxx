
#include <erebus/system/type_id.hxx>

#include <mutex>
#include <shared_mutex>
#include <unordered_map>

#include <boost/noncopyable.hpp>

namespace Er
{

namespace
{

class UserTypeRegistry final
    : public boost::noncopyable
{
public:
    UserTypeRegistry(Er::Log2::ILogger* log)
        : m_log(log)
    {
    }

    RegisteredType* findOrAdd(std::string_view key)
    {
        // maybe already there
        {
            std::shared_lock l(m_mutex);
            auto it = m_entries.find(key);
            if (it != m_entries.end())
                return &it->second;
        }

        // add
        {
            std::unique_lock l(m_mutex);
            auto id = m_next++;
            auto r = m_entries.insert({ key, RegisteredType(key, id) });
            auto entry = &r.first->second;
            if (r.second)
            {
                // really added
                ErLogDebug2(m_log, "Registered type [{}] -> {}", key, id);
            }

            return entry;
        }
    }

private:
    Er::Log2::ILogger* m_log;
    std::shared_mutex m_mutex;
    std::unordered_map<std::string_view, RegisteredType> m_entries;
    TypeIndex m_next = 0;
};

UserTypeRegistry* s_utr = nullptr;

} // namespace {}


ER_SYSTEM_EXPORT void initializeTypeRegistry(Er::Log2::ILogger* log)
{
    ErAssert(!s_utr);
    s_utr = new UserTypeRegistry(log);
}

ER_SYSTEM_EXPORT void finalizeTypeRegistry() noexcept
{
    delete s_utr;
    s_utr = nullptr;
}

ER_SYSTEM_EXPORT RegisteredType* lookupType(std::string_view name)
{
    ErAssert(s_utr);
    return s_utr->findOrAdd(name);
}


} // namespace Er {}