#pragma once

#include <erebus/system/property.hxx>

#include <stdexcept>
#include <vector>


//
// exception class with (almost) arbitrary properties
// that can be marshaled through RPC
//

namespace Er
{


class ER_SYSTEM_EXPORT Exception
    : public std::exception
{
public:
    template <typename Message>
    explicit Exception(std::source_location location, Message&& message) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<Message>(message)))
    {
    }

    template <typename Message, typename Prop, typename... Props>
    explicit Exception(std::source_location location, Message&& message, Prop&& prop, Props&&... props) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<Message>(message)), std::forward<Prop>(prop), std::forward<Props>(props)...)
    {
    }

    const char* what() const noexcept override
    {
        return m_context->message.c_str();
    }

    const std::string& message() const
    {
        return m_context->message;
    }

    std::source_location location() const noexcept
    {
        return m_context->location;
    }

    const std::vector<Property>& properties() const noexcept
    {
        return m_context->properties;
    }

    template <typename Prop>
        requires std::is_same_v<std::remove_cvref_t<Prop>, Property>
    Exception& add(Prop&& prop)
    {
        m_context->addProp(std::forward<Prop>(prop));
        return *this;
    }

    const Property* find(const PropertyInfo& type) const noexcept
    {
        for (auto& prop : m_context->properties)
        {
            if (type.name == prop.info().name)
                return &prop;
        }

        return nullptr;
    }

protected:
    struct Context final
    {
        std::source_location location;
        std::string message;
        std::vector<Property> properties;
        
        template <typename MessageT>
        Context(std::source_location location, MessageT&& message)
            : location(location)
            , message(std::forward<MessageT>(message))
        {
        }

        template <typename MessageT, typename Prop, typename... Props>
        Context(std::source_location location, MessageT&& message, Prop&& prop, Props&&... props)
            : Context(location, std::forward<MessageT>(message), std::forward<Props>(props)...)
        {
            addProp(std::forward<Prop>(prop), false);
        }

        template <typename Prop>
            requires std::is_same_v<std::remove_cvref_t<Prop>, Property>
        void addProp(Prop&& prop, bool back)
        {
            if (back)
                properties.push_back(std::forward<Prop>(prop));
            else
                properties.insert(properties.begin(), std::forward<Prop>(prop));
        }
    };

    std::shared_ptr<Context> m_context;
};


template <typename ExceptionVisitor>
auto dispatchException(const std::exception_ptr& ep, ExceptionVisitor& visitor)
{
    try
    {
        std::rethrow_exception(ep);
    }
    catch (const Exception& e)
    {
        return visitor(e);
    }
    catch (const std::bad_alloc& e)
    {
        return visitor(e);
    }
    catch (const std::bad_cast& e)
    {
        return visitor(e);
    }
    catch (const std::length_error& e)
    {
        return visitor(e);
    }
    catch (const std::out_of_range& e)
    {
        return visitor(e);
    }
    catch (const std::invalid_argument& e)
    {
        return visitor(e);
    }
    catch (const std::exception& e)
    {
        return visitor(e);
    }
    catch (...)
    {
        return visitor(ep);
    }
}


namespace ExceptionProps
{

extern ER_SYSTEM_EXPORT const PropertyInfo Result;

} // namespace ExceptionProps {}


} // namespace Er {}


#define ErThrowPosixError(msg, err, ...) \
    throw ::Er::Exception(std::source_location::current(), msg, ::Er::ExceptionProps::PosixErrorCode(int32_t(err)), ::Er::ExceptionProps::DecodedError(::Er::Util::posixErrorToString(err)), ##__VA_ARGS__)

#if ER_WINDOWS

#define ErThrowWin32Error(msg, err, ...) \
    throw ::Er::Exception(std::source_location::current(), msg, ::Er::ExceptionProps::Win32ErrorCode(int32_t(err)), ::Er::ExceptionProps::DecodedError(::Er::Util::win32ErrorToString(err)), ##__VA_ARGS__)

#endif

#define ErThrow(msg, ...) \
    throw ::Er::Exception(std::source_location::current(), msg, ##__VA_ARGS__)
