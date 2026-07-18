#include <SFML/Graphics.hpp>
#include <optional>
#include "imgui.h"
#include "imgui-SFML.h"
#include "implot.h"

int main()
{
    sf::RenderWindow window(sf::VideoMode({800, 600}), "Symulator Gieldy - Test API");
    window.setFramerateLimit(60);

    ImGui::SFML::Init(window);
    ImPlot::CreateContext();

    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);
    shape.setPosition({300.f, 200.f}); 

    sf::Clock deltaClock;

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            ImGui::SFML::ProcessEvent(window, *event);

            if (event->is<sf::Event::Closed>())
                window.close();
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Panel Gieldowy"); 
        
        ImGui::Text("Jeśli widzisz ten panel i wykres, kompilacja przebiegła pomyślnie!");
        
        if (ImPlot::BeginPlot("Cena Akcji (Test)")) {
            double czas[5] = { 1, 2, 3, 4, 5 };
            double cena[5] = { 10.5, 22.0, 15.0, 35.5, 30.0 };
            
            ImPlot::PlotLine("Spółka XYZ", czas, cena, 5);
            ImPlot::EndPlot();
        }
        
        ImGui::End();
        window.clear(sf::Color(30, 30, 30)); // Ciemnoszare tło
        
        window.draw(shape); 
        
        ImGui::SFML::Render(window); 
        
        window.display();
    }

    ImPlot::DestroyContext();
    ImGui::SFML::Shutdown();

    return 0;
}