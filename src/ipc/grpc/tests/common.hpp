#pragma once

#include <gtest/gtest.h>


#include <erebus/system/logger2.hxx>
#include <erebus/system/result.hxx>
#include <erebus/system/waitable.hxx>

#include <chrono>
#include <iostream>
#include <mutex>


extern std::uint16_t g_serverPort;
extern std::chrono::milliseconds g_operationTimeout;



template <class Interface>
struct CompletionBase
    : public Interface
{
    CompletionBase() = default;

    bool wait(std::chrono::milliseconds timeout = g_operationTimeout)
    {
        return m_complete.waitValueFor(true, timeout);
    }

    const std::optional<Er::ResultCode>& error() const
    {
        return m_error;
    }

    const std::string& errorMessage() const
    {
        return m_errorMessage;
    }

    bool serverPropertyMappingExpired() const
    {
        return m_serverPropertyMappingExpired;
    }

    bool clientPropertyMappingExpired() const
    {
        return m_clientPropertyMappingExpired;
    }

    bool success() const
    {
        return m_success;
    }

    void handleServerPropertyMappingExpired() override
    {
        m_serverPropertyMappingExpired = true;
        m_complete.setAndNotifyAll(true);
    }

    void handleClientPropertyMappingExpired() override
    {
        m_clientPropertyMappingExpired = true;
        m_complete.setAndNotifyAll(true);
    }

    void handleTransportError(Er::ResultCode result, std::string&& message) override
    {
        m_error = result;
        m_errorMessage = std::move(message);
        m_complete.setAndNotifyAll(true);
    }

    void handleSuccess() override
    {
        m_success = true;
        m_complete.setAndNotifyAll(true);
    }

protected:
    Er::Waitable<bool> m_complete;
    bool m_success = false;
    bool m_serverPropertyMappingExpired = false;
    bool m_clientPropertyMappingExpired = false;
    std::optional<Er::ResultCode> m_error;
    std::string m_errorMessage;
};