#include <erebus/system/result.hxx>

namespace Er
{

namespace
{

struct ResultMapping
{
    ResultCode code;
    std::string message;
};

const ResultMapping g_resultMapping[] =
{
    { Result::Ok, "Success" },
    { Result::Failure, "Failure" },
    { Result::OutOfMemory, "Out of memory" },
    { Result::Internal, "Internal error" },
    { Result::AccessDenied, "Access denied" },
    { Result::Timeout, "Timed out" },
    { Result::Canceled, "Operation canceled" },
    { Result::AlreadyExists, "Already exists" },
    { Result::InvalidArgument, "Invalid argument" },
    { Result::Unsupported, "Operation not supported" },
    { Result::NotFound, "Not found" },
    { Result::InsufficientResources, "Insufficient resources" },
    { Result::SharingViolation, "Sharing violation" },
};


} // namespace {}


ER_SYSTEM_EXPORT const std::string& resultToString(ResultCode code)
{
    for (auto& m : g_resultMapping)
    {
        if (m.code == code)
            return m.message;
    }

    static std::string empty;
    return empty;
}

} // namespace Er {}