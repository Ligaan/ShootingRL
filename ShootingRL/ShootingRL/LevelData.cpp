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

bool areRectanglesOverlapping(const sf::RectangleShape& rect1, const sf::RectangleShape& rect2) {
	return rect1.getGlobalBounds().intersects(rect2.getGlobalBounds());
}

// Function to get the angle of a point relative to the center
float angleFromCenter(const sf::Vector2f& point, const sf::Vector2f& center) {
	return std::atan2(point.y - center.y, point.x - center.x);
}

std::vector<sf::Vector2f> getRectangleCorners(const sf::RectangleShape& rect) {
	sf::FloatRect bounds = rect.getGlobalBounds();
	sf::Transform transform = rect.getTransform();

	// Get the corners of the rectangle in world space after transformation
	std::vector<sf::Vector2f> corners = {
		transform.transformPoint(0.f, 0.f),                                      // Top-left
		transform.transformPoint(bounds.width, 0.f),                             // Top-right
		transform.transformPoint(bounds.width, bounds.height),                   // Bottom-right
		transform.transformPoint(0.f, bounds.height)                             // Bottom-left
	};

	return corners;
}


// LINE/LINE
std::pair<bool, glm::vec2> lineLine(glm::vec2 A, glm::vec2 B, glm::vec2 C, glm::vec2 D) {

	// calculate the direction of the lines
	float uA = ((D.x - C.x) * (A.y - C.y) - (D.y - C.y) * (A.x - C.x)) / ((D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y));
	float uB = ((B.x - A.x) * (A.y - C.y) - (B.y - A.x) * (A.y - C.x)) / ((D.y - C.y) * (B.x - A.x) - (D.x - C.x) * (B.y - A.y));

	// if uA and uB are between 0-1, lines are colliding
	if (uA >= 0 && uA <= 1 && uB >= 0 && uB <= 1) {

		// optionally, draw a circle where the lines meet
		float intersectionX = A.x + (uA * (B.x - A.x));
		float intersectionY = A.y + (uA * (B.y - A.y));

		return std::make_pair(true, glm::vec2(intersectionX, intersectionY));
	}
	return std::make_pair(false, glm::vec2(0.0f));
}
// LINE/RECTANGLE
glm::vec2 lineRect(glm::vec2 start, glm::vec2 end, glm::vec2 A, glm::vec2 B, glm::vec2 C, glm::vec2 D) {

	// check if the line has hit any of the rectangle's sides
	// uses the Line/Line function below
	std::pair<bool, glm::vec2> left = lineLine(start, end, A, B);
	std::pair<bool, glm::vec2> right = lineLine(start, end, B, C);
	std::pair<bool, glm::vec2> top = lineLine(start, end, C, D);
	std::pair<bool, glm::vec2> bottom = lineLine(start, end, D, A);

	// Initially set the closest distance to a very large value
	float closestDistance = std::numeric_limits<float>::max();
	glm::vec2 closestPoint;

	// Check the left intersection
	if (left.first) {
		float dist = glm::distance2(start, left.second);
		if (dist < closestDistance) {
			closestDistance = dist;
			closestPoint = left.second;
		}
	}

	// Check the right intersection
	if (right.first) {
		float dist = glm::distance2(start, right.second);
		if (dist < closestDistance) {
			closestDistance = dist;
			closestPoint = right.second;
		}
	}

	// Check the top intersection
	if (top.first) {
		float dist = glm::distance2(start, top.second);
		if (dist < closestDistance) {
			closestDistance = dist;
			closestPoint = top.second;
		}
	}

	// Check the bottom intersection
	if (bottom.first) {
		float dist = glm::distance2(start, bottom.second);
		if (dist < closestDistance) {
			closestDistance = dist;
			closestPoint = bottom.second;
		}
	}

	// If we found any intersection, return the result
	return  closestPoint;
}





///////////////////////


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
	lastLoadedFile = filename;
	std::ifstream is(std::string("../assets/levels/") + filename + std::string(".json"));
	if (!is.is_open())
	{
		throw std::runtime_error("Failed to open file for loading data.");
	}

	cereal::JSONInputArchive archive(is);
	archive(lines);  // Load the `lines` member variable
}
void LevelData::Update(float dt)
{
	int playerIndex = FindPlayerIndex();
	if (playerIndex != -1) {

		lines[playerIndex].first.setRotation(lines[playerIndex].first.getRotation() + 10.0f * dt);

		previewLine.setPosition(lines[playerIndex].first.getPosition());
		previewLine.setFillColor(sf::Color::Yellow);
		//previewLine.setSize(sf::Vector2(5.0f,500.0f));

		float angleInDegrees = lines[playerIndex].first.getRotation();

		// Convert the angle from degrees to radians (GLM uses radians)
		float angleInRadians = glm::radians(angleInDegrees);

		// Create the rotation matrix using GLM
		glm::mat2 rotationMatrix = glm::mat2(
			glm::cos(angleInRadians), -glm::sin(angleInRadians),
			glm::sin(angleInRadians), glm::cos(angleInRadians)
		);

		// Rotate the vector using the matrix
		glm::vec2 rotatedVec = rotationMatrix * glm::vec2(0.0f, 500.0f);

		glm::vec2 start = glm::vec2(lines[playerIndex].first.getPosition().x, lines[playerIndex].first.getPosition().y);
		glm::vec2 end = start + rotatedVec;

		glm::vec2 v1Normalized(1.0f, 0.0f);
		glm::vec2 v2Normalized = glm::normalize(end - start);

		// Calculate the angle in radians

		float angle = glm::orientedAngle(v1Normalized, v2Normalized);

		// Convert radians to degrees
		float angleDegrees = glm::degrees(angle);

		previewLine.setSize(sf::Vector2(glm::length(end - start), 2.0f));
		previewLine.setRotation(angleDegrees);


		circle.clear();
		circle.push_back(sf::CircleShape());
		circle[circle.size() - 1].setPosition(sf::Vector2f(start.x, start.y));
		circle[circle.size() - 1].setRadius(10.0f);
		circle[circle.size() - 1].setOrigin(sf::Vector2f(5.0f, 5.0f));
		circle.push_back(sf::CircleShape());
		circle[circle.size() - 1].setPosition(sf::Vector2f(end.x, end.y));
		circle[circle.size() - 1].setRadius(10.0f);
		circle[circle.size() - 1].setOrigin(sf::Vector2f(5.0f, 5.0f));
		//circle.setRadius(20.0f);
		DrawCircle = false;
		glm::vec2 newEnd;


		std::vector<sf::Vector2f> corners,prevLine = getRectangleCorners(previewLine);
		for (auto line : lines) {
			if (line.second != ShapeType::Player) {
				//adjustRectangleSize(previewLine, line.first, 2.0f);
				if (areRectanglesOverlapping(previewLine, line.first)) {
					DrawCircle = true;
					corners = getRectangleCorners(line.first);
					end = lineRect(start,
						end,
						glm::vec2(corners[0].x, corners[0].y),
						glm::vec2(corners[1].x, corners[1].y),
						glm::vec2(corners[2].x, corners[2].y),
						glm::vec2(corners[3].x, corners[3].y));
					circle.push_back(sf::CircleShape());
					circle[circle.size() - 1].setPosition(sf::Vector2f(end.x, end.y));
					circle[circle.size() - 1].setRadius(10.0f);
					circle[circle.size() - 1].setOrigin(sf::Vector2f(5.0f, 5.0f));
					// 
					// 
					//glm::vec2 v1Normalized(1.0f, 0.0f);
					//glm::vec2 v2Normalized = glm::normalize(end - start);

					//// Calculate the angle in radians

					//float angle = glm::orientedAngle(v1Normalized, v2Normalized);

					//// Convert radians to degrees
					//float angleDegrees = glm::degrees(angle);

					//previewLine.setSize(sf::Vector2(glm::length(end - start), 1.0f));
					//previewLine.setRotation(angleDegrees);
				}
			}
		}
	}
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
	if (DrawCircle)
		for (auto cir : circle) {
			window.draw(cir);
		}
}

bool LevelData::IsSimulationRunning()
{
	return runSimulation;
}

void LevelData::PreviewMod(sf::RenderWindow& window)
{
	if (rightMouseButtonClicked)
		ResetPreviewLine();

	switch (currentMode) {
	case ShapeType::Player: {
		if (leftMouseButtonClicked) {
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
		if (leftMouseButtonClicked) {
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
		if (leftMouseButtonClicked) {
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
		if (leftMouseButtonClicked) {
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
			leftMouseButtonClicked = true;
		}
		if (event.mouseButton.button == sf::Mouse::Right) {
			rightMouseButtonClicked = true;
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

void LevelData::RunSimulation()
{
	if (ImGui::Button("Run")) {
		// Use the file name from the input field to save
		runSimulation = true;
		currentMode = ShapeType::None;
	}
	if (ImGui::Button("Stop")) {
		// Use the file name from the input field to save
		runSimulation = false;
	}
	if (ImGui::Button("Reload")) {
		// Use the file name from the input field to save
		LoadData(std::string(lastLoadedFile));  // Pass the file name to the Save function
	}
}

void LevelData::ResetInput()
{
	//Mouse
	leftMouseButtonClicked = false;
	rightMouseButtonClicked = false;
}
