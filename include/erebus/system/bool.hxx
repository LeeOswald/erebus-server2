#pragma once

#if !EREBUS_HXX_INCLUDED
    #include <erebus/system/erebus.hxx>
#endif


namespace Er
{

/**
    * @brief Fixed-size bool
    *
    * Sometimes we need a bool that has the same size on all platforms and across the wire.
    */

using Bool = std::uint8_t;

constexpr Bool False = 0;
constexpr Bool True = 1;



} // namespace Er {}