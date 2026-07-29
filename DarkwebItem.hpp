#pragma once
#include <string>
#include <nlohmann/json.hpp>

struct DarkwebItem
{
    int id = 0;
    std::string name;
    std::string description;
    double price = 0.0;
    bool isPurchased = false;
    std::string category = "General";
    std::string effectType = "NONE";
    double effectValue = 0.0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    DarkwebItem,
    id, name, description, price, isPurchased, category, effectType, effectValue
)