#pragma once

#include <gtest/gtest.h>


#include <erebus/system/logger2.hxx>

#include <iostream>
#include <syncstream>


struct InstanceCounter
{
    static int instances;

    InstanceCounter()
    {
        ++instances;
    }

    InstanceCounter(const InstanceCounter&)
    {
        ++instances;
    }

    InstanceCounter& operator=(const InstanceCounter&)
    {
        ++instances;
        return *this;
    }

    ~InstanceCounter()
    {
        --instances;
    }
};

class CapturedStderr
    : public Er::Log2::ISink
{
public:
    CapturedStderr() = default;
    ~CapturedStderr() = default;

    void write(Er::Log2::Record::Ptr r) override
    {
        m_out << r->message() << "\n";
    }

    void flush() override
    {
    }

    std::string content() const
    {
        auto s = m_out.str();
        return s;
    }

private:
    std::stringstream m_out;
};