#include "LevelData.h"

int main()
{
    auto window = sf::RenderWindow({ /*1920u, 1080u*/ 800u,800u }, "CMake SFML Project");
    window.setFramerateLimit(144);
    if (!ImGui::SFML::Init(window))
        return -1;

    LevelData levelData;

    sf::Clock clock;
    while (window.isOpen())
    {
        ImGui::SFML::Update(window, clock.restart());
        levelData.ResetInput();
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
        levelData.SelectModWindow();
        levelData.SaveLoadWindow();
        ImGui::End();

        window.clear();
        levelData.Draw(window);
        ImGui::SFML::Render(window);
        window.display();
    }

    ImGui::SFML::Shutdown();
}
