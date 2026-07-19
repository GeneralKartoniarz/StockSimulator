#pragma once
#include <string>
#include <vector>
#include "Sector.hpp"

struct Company {
    int id;                              
    std::string ticker;                  
    std::string name;                    
    std::string description;           
    double currentPrice;                    
    std::vector<double> priceHistory;     
    bool showOnChart = false;              
    Sector sector;                           
    double baseTrend = 0.0;                  
    double volatility = 0.01;            
    double shortTermSentiment = 0.0;  
};