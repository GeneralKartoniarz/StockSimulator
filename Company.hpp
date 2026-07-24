#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "Sector.hpp"

struct Company {
    int id = 0;                              
    std::string ticker;                  
    std::string name;                    
    std::string description;           
    double currentPrice = 0.0;                    
    std::vector<double> priceHistory;     
    bool showOnChart = false;              
    Sector sector = Sector::Tech;                           
    double baseTrend = 0.0;                  
    double volatility = 0.01;            
    double shortTermSentiment = 0.0;  
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(
    Company, 
    id, ticker, name, description, currentPrice, sector, baseTrend, volatility
)