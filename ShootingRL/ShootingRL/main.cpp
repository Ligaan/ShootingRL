#include "LevelData.h"

int main()
{
    auto window = sf::RenderWindow({ /*1920u, 1080u*/ 800u,800u }, "CMake SFML Project");
    window.setFramerateLimit(144);
    if (!ImGui::SFML::Init(window))
        return -1;

    LevelData levelData;

    sf::Clock clock;
    sf::Time timer;
    while (window.isOpen())
    {

        ImGui::SFML::Update(window,timer = clock.restart());
        levelData.ResetInput();
        if(levelData.IsSimulationRunning())
        levelData.Update(timer.asSeconds());
        ImGui::Begin("Hello, world!");
        for (auto event = sf::Event(); window.pollEvent(event);)
        {
            ImGui::SFML::ProcessEvent(window, event);

            if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape))
            {
                window.close();
            }
            levelData.CheckWindowEvent(event,window);
        }
        if(!levelData.IsSimulationRunning())
        levelData.SelectModWindow();
        levelData.SaveLoadWindow();
        levelData.RunSimulation();
        ImGui::End();

        window.clear();
        levelData.Draw(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
