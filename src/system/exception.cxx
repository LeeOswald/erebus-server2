#include <erebus/system/exception.hxx>

namespace Er
{

namespace ExceptionProps
{

const PropertyInfo Result{ PropertyType::Int32, "exception/result", "Result" };
const PropertyInfo DecodedError{ PropertyType::String, "exception/decoded_error", "Decoded error" };


} // namespace ExceptionProps {}

} // namespace Er {}