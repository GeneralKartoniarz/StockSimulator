#pragma once
#include <string>
#include <vector>

struct Commodity
{
    int id;                         
    std::string name;                   
    std::string symbol;                     
    double currentPrice;                   
    std::string unit;                      
    std::vector<double> priceHistory;     
    bool showOnChart = false;            
    double baseTrend = 0.0;                
    double volatility = 0.005;            
};