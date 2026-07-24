#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

struct Commodity
{
    int id = 0;                         
    std::string name;                   
    std::string symbol;                     
    double currentPrice = 0.0;                   
    std::string unit;                      
    std::vector<double> priceHistory;     
    bool showOnChart = false;            
    double baseTrend = 0.0;                
    double volatility = 0.005;            
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Commodity, 
    id, name, symbol, currentPrice, unit, baseTrend, volatility
)