#pragma once

#include <erebus/system/erebus.hxx>

namespace Er
{
    

using ResultCode = std::int32_t;

namespace Result
{

enum : std::int32_t
{
    Ok = 0,
    Failure = -1,
    OutOfMemory = -2,
    Internal = -3,
    AccessDenied = -4,
    Timeout = -5,
    Canceled = -6,
    AlreadyExists = -8,
    InvalidArgument = -9,
    Unsupported = -10,
    NotFound = -11,
    InsufficientResources = -12,
    SharingViolation = -13,
    DeadlineExceeded = -14,
    Unauthenticated = -15,
    ResourceExhausted = -16,
    FailedPrecondition = -17,
    Aborted = -18,
    OutOfRange = -19,
    Unimplemented = -20,
    Unavailable = -21,
    DataLoss = -22
};


} // namespace Result {}


ER_SYSTEM_EXPORT [[nodiscard]] const std::string& resultToString(ResultCode code);

} // namespace Er {}