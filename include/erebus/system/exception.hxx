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
    explicit Exception(std::source_location location, auto&& message) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<decltype(message)>(message)))
    {
    }

    explicit Exception(std::source_location location, auto&& message, auto&& prop, auto&&... props) noexcept
        : m_context(std::make_shared<Context>(location, std::forward<decltype(message)>(message)), std::forward<decltype(prop)>(prop), std::forward<decltype(props)>(props)...)
    {
    }

    const char* what() const noexcept override
    {
        return m_context->message.c_str();
    }

    const auto& message() const
    {
        return m_context->message;
    }

    auto location() const noexcept
    {
        return m_context->location;
    }

    const auto& properties() const noexcept
    {
        return m_context->properties;
    }

    template <typename Prop>
        requires std::is_same_v<std::remove_cvref_t<Prop>, Property>
    Exception& add(Prop&& prop)
    {
        m_context->addProp(std::forward<Prop>(prop), true);
        return *this;
    }

    const Property* find(const PropertyInfo& type) const noexcept
    {
        for (auto& prop : m_context->properties)
        {
            if (type.name == prop.info()->name)
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
        
        Context(std::source_location location, auto&& message)
            : location(location)
            , message(std::forward<decltype(message)>(message))
        {
        }

        Context(std::source_location location, auto&& message, auto&& prop, auto&&... props)
            : Context(location, std::forward<decltype(message)>(message), std::forward<decltype(props)>(props)...)
        {
            addProp(std::forward<decltype(prop)>(prop), false);
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


class exceptionStackIterator
{
public:
    using value_type = std::exception_ptr;

    exceptionStackIterator() noexcept = default;

    exceptionStackIterator(std::exception_ptr exception) noexcept 
        : m_exception(std::move(exception)) 
    {}

    exceptionStackIterator& operator++() noexcept
    {
        try
        {
            std::rethrow_exception(m_exception);
        }
        catch (const std::nested_exception& e)
        {
            m_exception = e.nested_ptr();
        }
        catch (...)
        {
            m_exception = {};
        }
        return *this;
    }

    std::exception_ptr operator*() const noexcept
    {
        return m_exception;
    }

private:
    friend bool operator==(const exceptionStackIterator& a, const exceptionStackIterator& b) noexcept
    {
        return *a == *b;
    }

    friend bool operator!=(const exceptionStackIterator& a, const exceptionStackIterator& b) noexcept
    {
        return !(a == b);
    }


private:
    std::exception_ptr m_exception;
};


class exceptionStackRange
{
public:
    exceptionStackRange() noexcept = default;

    exceptionStackRange(exceptionStackIterator begin, exceptionStackIterator end) noexcept 
        : m_begin(std::move(begin))
        , m_end(std::move(end)) 
    {}

    exceptionStackIterator begin() const noexcept
    {
        return m_begin;
    }

    exceptionStackIterator end() const noexcept
    {
        return m_end;
    }

private:
    exceptionStackIterator m_begin;
    exceptionStackIterator m_end;
};


inline exceptionStackRange makeExceptionStackRange(std::exception_ptr exception) noexcept
{
    return exceptionStackRange(exceptionStackIterator(std::move(exception)), exceptionStackIterator());
}

inline exceptionStackRange currentExceptionStack() noexcept
{
    return makeExceptionStackRange(std::current_exception());
}


namespace ExceptionProps
{

extern ER_SYSTEM_EXPORT const PropertyInfo Result;
extern ER_SYSTEM_EXPORT const PropertyInfo DecodedError;

} // namespace ExceptionProps {}


} // namespace Er {}

