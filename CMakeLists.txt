cmake_minimum_required(VERSION 3.28)
project(StockSimulator LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.1    
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(implot
    GIT_REPOSITORY https://github.com/epezent/implot.git
    GIT_TAG v0.16
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(implot)
set(IMGUI_SFML_FIND_SFML OFF CACHE BOOL "" FORCE)
set(IMGUI_DIR ${imgui_SOURCE_DIR} CACHE PATH "" FORCE)

FetchContent_Declare(imgui_sfml
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
    GIT_TAG master 
    GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui_sfml)

add_executable(StockSimulator 
    main.cpp 
    ${implot_SOURCE_DIR}/implot.cpp
    ${implot_SOURCE_DIR}/implot_items.cpp
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
    SFML::System
    ImGui-SFML::ImGui-SFML
)