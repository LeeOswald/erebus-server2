#pragma once

#if !EREBUS_HXX_INCLUDED
    #include <erebus/system/erebus.hxx>
#endif


/**
 * @brief Fixed-size bool
 *  
 * Sometimes we need a bool that has the same size on all platforms and across the wire.
 */
 
enum class Bool: std::uint8_t 
{   
    False = 0,
    True = 1
};

constexpr Bool False = Bool::False;
constexpr Bool True = Bool::True;