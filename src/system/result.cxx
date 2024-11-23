#include <erebus/system/result.hxx>

namespace Er
{

std::string ResultProvider::resultToString(Result r)
{
    switch (r)
    {
    case Er::Result::Ok: return "Success";
    case Er::Result::Failure: return "Failure";
    case Er::Result::OutOfMemory: return "Out of memory";
    case Er::Result::Internal: return "Internal error";
    }

    return "Unexpected error";
}


} // namespace Er {}