cmake_minimum_required(VERSION 3.28)
project(StockSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# SFML
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

# ImGui
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1    
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui)

# ImPlot
FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG v0.16
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(implot)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

# ImGui-SFML
FetchContent_Declare(imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG master 
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui_sfml)

# nlohmann/json (NOWOŚĆ)
FetchContent_Declare(json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(json)

add_executable(StockSimulator 
    main.cpp
    Game.hpp
    GameState.hpp 
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
    Game.cpp
    MainMenuState.hpp
    MainMenuState.cpp
    BankruptcyState.hpp   
    BankruptcyState.cpp
    PlayingState.hpp
    PlayingState.cpp
    Company.hpp
    Commodity.hpp
    Sector.hpp
    Mail.hpp
    EventSystem.hpp
    EventSystem.cpp
)

if(MSVC)
    target_compile_options(StockSimulator PRIVATE /W4)
endif()

target_include_directories(StockSimulator PRIVATE 
    ${imgui_SOURCE_DIR}
    ${implot_SOURCE_DIR}
)

target_link_libraries(StockSimulator PRIVATE 
    SFML::Graphics 
    SFML::Window 
    SFML::Audio
    SFML::System
    ImGui-SFML::ImGui-SFML
    nlohmann_json::nlohmann_json
)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})