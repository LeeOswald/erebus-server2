#pragma once

#include <erebus/system/erebus.hxx>


namespace Er::Lua
{

class State;

//
// Lua 64-bit integers
//

template <typename T, typename = void>
struct IntegerWrapper;

template <typename T>
struct IntegerWrapper<T, std::enable_if_t<std::is_integral<T>::value && (sizeof(T) == 8), void>>
{
    using HalfType = std::conditional_t<std::is_signed_v<T>, int32_t, uint32_t>;
    T value;

    constexpr IntegerWrapper(T v = T()) : value(v) {}
    constexpr IntegerWrapper(HalfType hi, uint32_t lo) : value((T(hi) << 32) | lo) {}

    static constexpr IntegerWrapper __add(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value + b.value); }
    static constexpr IntegerWrapper __sub(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value - b.value); }
    static constexpr IntegerWrapper __mul(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value * b.value); }
    static constexpr IntegerWrapper __div(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value / b.value); }
    static constexpr IntegerWrapper __mod(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value % b.value); }
    static constexpr IntegerWrapper __unm(const IntegerWrapper& a) { return IntegerWrapper(-a.value); }
    static constexpr IntegerWrapper __band(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value & b.value); }
    static constexpr IntegerWrapper __bor(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value | b.value); }
    static constexpr IntegerWrapper __bxor(const IntegerWrapper& a, const IntegerWrapper& b) { return IntegerWrapper(a.value ^ b.value); }
    static constexpr IntegerWrapper __bnot(const IntegerWrapper& a) { return IntegerWrapper(~a.value); }
    static constexpr IntegerWrapper __shl(const IntegerWrapper& a, unsigned int n) { return IntegerWrapper(a.value << n); }
    static constexpr IntegerWrapper __shr(const IntegerWrapper& a, unsigned int n) { return IntegerWrapper(a.value >> n); }
    static constexpr bool __eq(const IntegerWrapper& a, const IntegerWrapper& b) { return (a.value == b.value); }
    static constexpr bool __lt(const IntegerWrapper& a, const IntegerWrapper& b) { return (a.value < b.value); }
    static constexpr bool __gt(const IntegerWrapper& a, const IntegerWrapper& b) { return (a.value > b.value); }
    static constexpr bool __le(const IntegerWrapper& a, const IntegerWrapper& b) { return (a.value <= b.value); }
    static std::string __tostring(const IntegerWrapper& a) { return std::to_string(a.value); }
};

using Int64Wrapper = IntegerWrapper<int64_t>;
using UInt64Wrapper = IntegerWrapper<uint64_t>;

ER_SYSTEM_EXPORT void registerInt64(State& state);
ER_SYSTEM_EXPORT void registerUInt64(State& state);

} // namespace Er::Lua {}