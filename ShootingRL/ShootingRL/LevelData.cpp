#include "LevelData.h"
#include "glm/gtx/vector_angle.hpp"
#include "algorithm"
#include "fstream"

#include "Utilities.h"

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

	int playerIndex = FindPlayerIndex();
	if (playerIndex != -1) {
		lines[playerIndex].first.setOrigin(lines[playerIndex].first.getSize() / 2.0f);
	}
}
void LevelData::Update(float dt)
{
	playerIndex = FindPlayerIndex();
	if (playerIndex != -1) {
		if (shoot) {
			PlayerRaycast();
			CheckTarget();
		}
		PlayerMovement(dt);
		PlayerDirection();
		CheckForWinLose(dt);
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

	window.draw(previewLine);
	for (auto line : lines) {
		window.draw(line.first);
	}
	if (runSimulation) {
		window.draw(playerDirection);
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
		previewLine.setSize(sf::Vector2(10.0f, 10.0f));
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
		previewLine.setSize(sf::Vector2(10.0f, 10.0f));
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
		previewLine.setSize(sf::Vector2(10.0f, 10.0f));
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
	if (isImGuiHovered) {
		return;
	}
	if (event.type == sf::Event::MouseButtonReleased) {
		if (event.mouseButton.button == sf::Mouse::Left) {
			leftMouseButtonClicked = true;
		}
		if (event.mouseButton.button == sf::Mouse::Right) {
			rightMouseButtonClicked = true;
		}
	}
	if (event.type == sf::Event::KeyReleased) {
		if (event.key.code == sf::Keyboard::Space) {
			shoot = true;
		}
	}
	if (event.type == sf::Event::KeyPressed) {
		if (event.key.code == sf::Keyboard::W) {
			up = true;
		}
		if (event.key.code == sf::Keyboard::S) {
			down = true;
		}
		if (event.key.code == sf::Keyboard::A) {
			left = true;
		}
		if (event.key.code == sf::Keyboard::D) {
			right = true;
		}
		if (event.key.code == sf::Keyboard::Left) {
			rotateLeft = true;
		}
		if (event.key.code == sf::Keyboard::Right) {
			rotateRight = true;
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

int LevelData::FindFirstEnemyIntex()
{
	auto it1 = std::find_if(lines.begin(), lines.end(), [](const auto& pair) {
		return pair.second == ShapeType::StaticTarget;
		});

	if (it1 != lines.end()) {
		return static_cast<int>(std::distance(lines.begin(), it1)); // Get the index of the found element
	}

	auto it2 = std::find_if(lines.begin(), lines.end(), [](const auto& pair) {
		return pair.second == ShapeType::MovingTarget;
		});

	if (it2 != lines.end()) {
		return static_cast<int>(std::distance(lines.begin(), it2)); // Get the index of the found element
	}

	return -1;
}

void LevelData::SelectModWindow()
{
	isImGuiHovered = ImGui::IsWindowHovered() || ImGui::IsAnyItemHovered();
	// Create a combo box to select ShapeType
	const char* shapeNames[] = { "EnvironmentLine", "StaticTarget", "MovingTarget", "Player", "None" };
	const char* currentShapeName = ImGui::ShapeTypeToString(currentMode);

	if (ImGui::BeginCombo("ShapeType", currentShapeName)) {
		for (int i = 0; i < 5; ++i) {
			bool isSelected = (currentMode == ImGui::StringToShapeType(shapeNames[i]));
			if (ImGui::Selectable(shapeNames[i], isSelected)) {
				currentMode = ImGui::StringToShapeType(shapeNames[i]);
				ResetPreviewLine();
			}
		}
		ImGui::EndCombo();
	}

	// Display the current selected shape type
	ImGui::Text("Selected Shape: %s", ImGui::ShapeTypeToString(currentMode));
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
		timer = 0.0f;
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
	shoot = false;
	up = down = left = right = false;
	isImGuiHovered = runSimulation ? false : isImGuiHovered;
	rotateLeft = rotateRight = false;
	lastTargetIndex = -1;
}

void LevelData::PlayerRaycast()
{
	previewLine.setPosition(lines[playerIndex].first.getPosition());
	if (debugLine) {
		previewLine.setFillColor(sf::Color::Yellow);
	}
	else {
		previewLine.setOutlineColor(sf::Color::Transparent);
		previewLine.setFillColor(sf::Color::Transparent);
	}

	float angleInDegrees = lines[playerIndex].first.getRotation();

	// Convert the angle from degrees to radians (GLM uses radians)
	float angleInRadians = glm::radians(-angleInDegrees);

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


	std::vector<sf::Vector2f> corners, prevLine = GetRectangleCorners(previewLine);
	int index = 0;
	for (auto line : lines) {
		if (line.second != ShapeType::Player) {
			corners = GetRectangleCorners(line.first);
			if (Physics::LineRect(start,
				end,
				glm::vec2(corners[0].x, corners[0].y),
				glm::vec2(corners[1].x, corners[1].y),
				glm::vec2(corners[2].x, corners[2].y),
				glm::vec2(corners[3].x, corners[3].y), end)) {

				glm::vec2 v1Normalized(1.0f, 0.0f);
				glm::vec2 v2Normalized = glm::normalize(end - start);

				lastTargetIndex = index;

				// Calculate the angle in radians

				float angle = glm::orientedAngle(v1Normalized, v2Normalized);

				// Convert radians to degrees
				float angleDegrees = glm::degrees(angle);
				previewLine.setSize(sf::Vector2(glm::length(end - start), 1.0f));
				previewLine.setRotation(angleDegrees);
			}
		}
		index++;
	}
}

float LevelData::PlayerMovement(float dt)
{
	float score = 0.0f;
	float originalOrientation = lines[playerIndex].first.getRotation();
	sf::Vector2f originalPosition = lines[playerIndex].first.getPosition();
	if (up) {
		lines[playerIndex].first.setPosition(lines[playerIndex].first.getPosition() + sf::Vector2f(0.0, -1.0f) * movementValue * dt);
		score += moveReward;
	}
	if (down) {
		lines[playerIndex].first.setPosition(lines[playerIndex].first.getPosition() + sf::Vector2f(0.0, 1.0f) * movementValue * dt);
		score += moveReward;
	}
	if (left) {
		lines[playerIndex].first.setPosition(lines[playerIndex].first.getPosition() + sf::Vector2f(-1.0, 0.0f) * movementValue * dt);
		score += moveReward;
	}
	if (right) {
		lines[playerIndex].first.setPosition(lines[playerIndex].first.getPosition() + sf::Vector2f(1.0, 0.0f) * movementValue * dt);
		score += moveReward;
	}
	if (rotateLeft) {
		lines[playerIndex].first.setRotation(lines[playerIndex].first.getRotation() + 1.0f * rotationForce * dt);
		score += rotateReward;
	}
	if (rotateRight) {
		lines[playerIndex].first.setRotation(lines[playerIndex].first.getRotation() - 1.0f * rotationForce * dt);
		score += rotateReward;
	}

	for (auto line : lines) {
		if (line.second != ShapeType::Player) {
			if (Physics::RectanglesIntersect(lines[playerIndex].first, line.first)) {
				score += collideReward;
				lines[playerIndex].first.setPosition(originalPosition);
				lines[playerIndex].first.setRotation(originalOrientation);
				break;
			}
		}
	}
	return score;
}

void LevelData::PlayerDirection()
{
	playerDirection.setPosition(lines[playerIndex].first.getPosition());
	playerDirection.setFillColor(sf::Color::Magenta);

	float angleInDegrees = lines[playerIndex].first.getRotation();

	// Convert the angle from degrees to radians (GLM uses radians)
	float angleInRadians = glm::radians(-angleInDegrees);

	// Create the rotation matrix using GLM
	glm::mat2 rotationMatrix = glm::mat2(
		glm::cos(angleInRadians), -glm::sin(angleInRadians),
		glm::sin(angleInRadians), glm::cos(angleInRadians)
	);

	// Rotate the vector using the matrix
	glm::vec2 rotatedVec = rotationMatrix * glm::vec2(0.0f, 25.0f);

	glm::vec2 start = glm::vec2(lines[playerIndex].first.getPosition().x, lines[playerIndex].first.getPosition().y);
	glm::vec2 end = start + rotatedVec;

	glm::vec2 v1Normalized(1.0f, 0.0f);
	glm::vec2 v2Normalized = glm::normalize(end - start);

	// Calculate the angle in radians

	float angle = glm::orientedAngle(v1Normalized, v2Normalized);

	// Convert radians to degrees
	float angleDegrees = glm::degrees(angle);

	playerDirection.setSize(sf::Vector2(glm::length(end - start), 2.0f));
	playerDirection.setRotation(angleDegrees);
}

float LevelData::CheckForWinLose(float dt)
{
	if (FindFirstEnemyIntex() == -1) {
		runSimulation = false;
		return winReward;
	}
	timer += dt;
	if (timer == simulationDeadline) {
		runSimulation = false;
		return loseReward;
	}
	return -timer*timeMultiplier;
}

float LevelData::CheckTarget()
{
	if (lastTargetIndex != -1) {
		switch (lines[lastTargetIndex].second) {
		case ShapeType::EnvironmentLine: {
			return missTargetReward;
			break;
		}
		case ShapeType::StaticTarget: {
			lines.erase(lines.begin() + lastTargetIndex);
			playerIndex = FindPlayerIndex();
			return hitStaticTargetReward;
			break;
		}
		case ShapeType::MovingTarget: {
			lines.erase(lines.begin() + lastTargetIndex);
			playerIndex = FindPlayerIndex();
			return hitMovingTargetReward;
			break;
		}
		default: {
			return missTargetReward;
			break;
		}
		}
	}
}
