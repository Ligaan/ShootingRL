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

        //temporary way to access the screen buffer
        {
            sf::Texture texture;

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
                // Capture the window contents to the texture
                texture.create(window.getSize().x, window.getSize().y);
                texture.update(window);

                // Create an image from the texture
                sf::Image screenshot = texture.copyToImage();

                // Save the image to a file (optional)
                screenshot.saveToFile("../assets/screenshot.png");
            }
        }

        if (levelData.IsSimulationRunning())
            levelData.Update(timer.asSeconds());

    }

    ImGui::SFML::Shutdown();
}
