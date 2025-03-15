#include <erebus/system/exception.hxx>

namespace Er
{

namespace ExceptionProps
{

const PropertyInfo Result{ PropertyType::Int32, "Er.Exception.result", "Result" };
const PropertyInfo DecodedError{ PropertyType::String, "Er.Exception.decoded_error", "Decoded error" };


} // namespace ExceptionProps {}

} // namespace Er {}