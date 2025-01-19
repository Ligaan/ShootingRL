#include "LevelData.h"
#include "glm/gtx/vector_angle.hpp"
#include "algorithm"
#include "fstream"

// Helper function to convert enum values to strings for ImGui display
const char* ShapeTypeToString(ShapeType shape) {
	switch (shape) {
	case ShapeType::EnvironmentLine: return "EnvironmentLine";
	case ShapeType::StaticTarget:    return "StaticTarget";
	case ShapeType::MovingTarget:    return "MovingTarget";
	case ShapeType::Player:          return "Player";
	case ShapeType::None:            return "None";
	default:                         return "Unknown";
	}
}

// Helper function to convert string to enum
ShapeType StringToShapeType(const std::string& str) {
	if (str == "EnvironmentLine") return ShapeType::EnvironmentLine;
	if (str == "StaticTarget")    return ShapeType::StaticTarget;
	if (str == "MovingTarget")    return ShapeType::MovingTarget;
	if (str == "Player")          return ShapeType::Player;
	if (str == "None")            return ShapeType::None;
	return ShapeType::None;
}

LevelData::LevelData()
{
	ResetPreviewLine();
	mousePrevEvent.type = sf::Event::MouseButtonReleased;
}
void LevelData::SaveData(const std::string& filename)
{
	{
		std::ofstream os(std::string("../assets/levels/") + filename + std::string(".json"));
		cereal::JSONOutputArchive archive(os);
		archive(lines);
	}
}
void LevelData::LoadData(const std::string& filename)
{
	std::ifstream is(std::string("../assets/levels/") + filename +std::string(".json"));
	if (!is.is_open())
	{
		throw std::runtime_error("Failed to open file for loading data.");
	}

	cereal::JSONInputArchive archive(is);
	archive(lines);  // Load the `lines` member variable
}
void LevelData::Draw(sf::RenderWindow& window)
{
	////

	//Buffer access
	//sf::Image screenshot = window.capture();

	// Save the image (optional)
	//screenshot.saveToFile("screenshot.png");

	sf::Vector2 mousePos = sf::Mouse::getPosition(window);

	PreviewMod(window);

	//window.draw(shape);
	window.draw(previewLine);
	for (auto line : lines) {
		window.draw(line.first);
	}
}

void LevelData::PreviewMod(sf::RenderWindow& window)
{
	if (RightMouseButtonClicked)
		ResetPreviewLine();

	switch (currentMode) {
	case ShapeType::Player: {
		if (LeftMouseButtonClicked) {
			int playerIndex = FindPlayerIndex();
			if (playerIndex == -1) {
				AddPreviewLine();
			}
			else {
				lines[playerIndex].first = previewLine;
			}
		}

		sf::Vector2 mousePos = sf::Mouse::getPosition(window);
		previewLine.setPosition(sf::Vector2f(mousePos) - sf::Vector2(2.5f, 2.5f));
		previewLine.setSize(sf::Vector2(5.0f, 5.0f));
		previewLine.setRotation(0.0f);
		previewLine.setFillColor(sf::Color::Green);
		break;
	}
	case ShapeType::StaticTarget: {
		if (LeftMouseButtonClicked) {
			AddPreviewLine();
		}

		sf::Vector2 mousePos = sf::Mouse::getPosition(window);
		previewLine.setPosition(sf::Vector2f(mousePos) - sf::Vector2(2.5f, 2.5f));
		previewLine.setSize(sf::Vector2(5.0f, 5.0f));
		previewLine.setRotation(0.0f);
		previewLine.setFillColor(sf::Color::Red);
		break;
	}
	case ShapeType::MovingTarget: {
		if (LeftMouseButtonClicked) {
			AddPreviewLine();
		}

		sf::Vector2 mousePos = sf::Mouse::getPosition(window);
		previewLine.setPosition(sf::Vector2f(mousePos) - sf::Vector2(2.5f, 2.5f));
		previewLine.setSize(sf::Vector2(5.0f, 5.0f));
		previewLine.setRotation(0.0f);
		previewLine.setFillColor(sf::Color::Blue);
		break;
	}
	case ShapeType::EnvironmentLine: {
		if (LeftMouseButtonClicked) {
			if (!previewLineEnabled) {
				sf::Vector2 mousePos = sf::Mouse::getPosition(window);
				SetPreviewLineStart(glm::vec2(mousePos.x, mousePos.y));
			}
			else {
				AddPreviewLine();
			}
		}

		if (previewLineEnabled) {
			glm::vec2 start = glm::vec2(previewLine.getPosition().x, previewLine.getPosition().y);
			sf::Vector2 mousePos = sf::Mouse::getPosition(window);
			glm::vec2 end = glm::vec2(mousePos.x, mousePos.y);

			glm::vec2 v1Normalized(1.0f, 0.0f);
			glm::vec2 v2Normalized = glm::normalize(end - start);

			// Calculate the angle in radians

			float angle = glm::orientedAngle(v1Normalized, v2Normalized);

			// Convert radians to degrees
			float angleDegrees = glm::degrees(angle);

			previewLine.setSize(sf::Vector2(glm::length(end - start), 5.0f));
			previewLine.setRotation(angleDegrees);
			previewLine.setFillColor(sf::Color::White);
		}
		break;
	}
	default: {

	}
	}
}

void LevelData::SetPreviewLineStart(glm::vec2 start)
{
	previewLine.setPosition(sf::Vector2(start.x, start.y));
	previewLineEnabled = true;
}

void LevelData::AddPreviewLine()
{
	lines.push_back(std::make_pair(previewLine, currentMode));
	ResetPreviewLine();

}

void LevelData::CheckWindowEvent(sf::Event& event, sf::RenderWindow& window)
{
	if (event.type == sf::Event::MouseButtonReleased) {
		if (isImGuiHovered) {
			return;
		}
		if (event.mouseButton.button == sf::Mouse::Left) {
			LeftMouseButtonClicked = true;
		}
		if (event.mouseButton.button == sf::Mouse::Right) {
			RightMouseButtonClicked = true;
		}
	}
}

void LevelData::ResetPreviewLine()
{
	previewLine.setPosition(sf::Vector2(0.0f, 0.0f));
	previewLine.setSize(sf::Vector2(0.0f, 0.0f));
	previewLine.setRotation(0.0f);
	previewLineEnabled = false;
}

int LevelData::FindPlayerIndex()
{
	auto it = std::find_if(lines.begin(), lines.end(), [](const auto& pair) {
		return pair.second == ShapeType::Player;
		});

	if (it != lines.end()) {
		return static_cast<int>(std::distance(lines.begin(), it)); // Get the index of the found element
	}
	return -1; // Return -1 if no Player is found
}

void LevelData::SelectModWindow()
{
	isImGuiHovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered();
	// Create a combo box to select ShapeType
	const char* shapeNames[] = { "EnvironmentLine", "StaticTarget", "MovingTarget", "Player", "None" };
	const char* currentShapeName = ShapeTypeToString(currentMode);

	if (ImGui::BeginCombo("ShapeType", currentShapeName)) {
		for (int i = 0; i < 5; ++i) {
			bool isSelected = (currentMode == StringToShapeType(shapeNames[i]));
			if (ImGui::Selectable(shapeNames[i], isSelected)) {
				currentMode = StringToShapeType(shapeNames[i]);
				ResetPreviewLine();
			}
		}
		ImGui::EndCombo();
	}

	// Display the current selected shape type
	ImGui::Text("Selected Shape: %s", ShapeTypeToString(currentMode));
}

void LevelData::SaveLoadWindow()
{
	static char fileName[256] = "";  // Buffer to store the file name (you can adjust the size as needed)

	// Input text for the file name
	ImGui::InputText("File Name", fileName, IM_ARRAYSIZE(fileName));

	if (ImGui::Button("Save")) {
		// Use the file name from the input field to save
		SaveData(std::string(fileName));  // Pass the file name to the Save function
	}

	if (ImGui::Button("Load")) {
		// Use the file name from the input field to load
		LoadData(std::string(fileName));  // Pass the file name to the Load function
	}
}

void LevelData::ResetInput()
{
	//Mouse
	LeftMouseButtonClicked = false;
	RightMouseButtonClicked = false;
}
