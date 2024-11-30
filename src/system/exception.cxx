#include <erebus/system/exception.hxx>

namespace Er
{

namespace ExceptionProps
{

ER_SYSTEM_EXPORT const PropertyInfo Result{ PropertyType::Int32, "exception/result", "Result" };
ER_SYSTEM_EXPORT const PropertyInfo DecodedError{ PropertyType::String, "exception/decoded_error", "Decoded error" };


} // namespace ExceptionProps {}

} // namespace Er {}