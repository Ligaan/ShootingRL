#pragma once
#include "SFML/Graphics.hpp"
#include "glm/glm.hpp"
#include "vector"
#include "utility"
#include "imgui-SFML.h"
#include "imgui.h"
#define GLM_ENABLE_EXPERIMENTAL

enum class ShapeType {
	EnvironmentLine,
	StaticTarget,
	MovingTarget,
	Player,
	None
};

class LevelData
{
public:
	LevelData();
	// Serialization
	void SaveDate();
	void LoadData();
	// Core Functions
	void Update(float dt);
	void Draw(sf::RenderWindow& window);
	// Level Editing Functions
	void PreviewMod(sf::RenderWindow& window);
	void SetPreviewLineStart(glm::vec2 start);
	void AddPreviewLine();
	void CheckWindowEvent(sf::Event& event, sf::RenderWindow& window);
	void ResetPreviewLine();
	int FindPlayerIndex();
	// ImGui Functions
	void SelectModWindow();
	// Input
	void ResetInput();
private:
	std::vector<std::pair<sf::RectangleShape, ShapeType>> lines;
	sf::RectangleShape previewLine;
	bool previewLineEnabled = false;
	sf::Event mousePrevEvent;
	bool ImGuiWindowHovered = false;
	ShapeType currentMode = ShapeType::None;
	//
	bool LeftMouseButtonClicked = false;
	bool RightMouseButtonClicked = false;
	//
	bool isImGuiHovered = false;
};

