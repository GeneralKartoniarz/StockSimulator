#pragma once
#include <nlohmann/json.hpp>

enum class Sector {
    Tech,     
    Automotive,  
    Energy,      
    Luxury,      
    Consumer     
};

NLOHMANN_JSON_SERIALIZE_ENUM(Sector, {
    {Sector::Tech, "Tech"},
    {Sector::Automotive, "Automotive"},
    {Sector::Energy, "Energy"},
    {Sector::Luxury, "Luxury"},
    {Sector::Consumer, "Consumer"}
})