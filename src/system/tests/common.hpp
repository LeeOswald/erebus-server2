#pragma once

#include <gtest/gtest.h>


#include <erebus/system/erebus.hxx>

#include <iostream>
#include <syncstream>


namespace TestProps
{


} // TestProps {}


struct InstanceCounter 
{
    static int instances;

    InstanceCounter() 
    { 
        ++instances; 
    }
    
    InstanceCounter(const InstanceCounter&) 
    { 
        ++instances; 
    }
    
    InstanceCounter& operator=(const InstanceCounter&) 
    { 
        ++instances; 
        return *this;
    }
    
    ~InstanceCounter() 
    { 
        --instances; 
    }
};


