#pragma once
#include "SFML/Graphics.hpp"
#include "glm/glm.hpp"
#include "vector"
#include "utility"
#include "string"
#include "ImGui/imgui-SFML.h"
#include "ImGui/imgui.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <cereal/cereal.hpp>
#include "cereal/archives/json.hpp"
#include "cereal/types/vector.hpp"
#include "cereal/types/utility.hpp"

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
	void SaveData(const std::string& filename);
	void LoadData(const std::string& filename);
	// Core Functions
	void Update(float dt);
	void Draw(sf::RenderWindow& window);
	bool IsSimulationRunning();
	// Level Editing Functions
	void PreviewMod(sf::RenderWindow& window);
	void SetPreviewLineStart(glm::vec2 start);
	void AddPreviewLine();
	void CheckWindowEvent(sf::Event& event, sf::RenderWindow& window);
	void ResetPreviewLine();
	int FindPlayerIndex();
	// ImGui Functions
	void SelectModWindow();
	void SaveLoadWindow();
	void RunSimulation();
	// Input
	void ResetInput();
private:
	std::vector<std::pair<sf::RectangleShape, ShapeType>> lines;
	sf::RectangleShape previewLine;
	bool previewLineEnabled = false;
	sf::Event mousePrevEvent;
	ShapeType currentMode = ShapeType::None;
	//
	bool leftMouseButtonClicked = false;
	bool rightMouseButtonClicked = false;
	//
	bool isImGuiHovered = false;
	bool runSimulation = false;
	//
	std::string lastLoadedFile = "";








	std::vector<sf::CircleShape> circle;
	bool DrawCircle = false;
};

namespace sf {

	template<class Archive>
	void serialize(Archive& archive, sf::Color& c)
	{
		archive(
			CEREAL_NVP(c.r),
			CEREAL_NVP(c.g),
			CEREAL_NVP(c.b),
			CEREAL_NVP(c.a)
		);
	}
	template<class Archive>
	void serialize(Archive& archive, sf::Vector2f& c)
	{
		archive(
			CEREAL_NVP(c.x),
			CEREAL_NVP(c.y)
		);
	}
	/*template<class Archive>
	void serialize(Archive& archive, sf::RectangleShape const& c)
	{
		archive(
			CEREAL_NVP(c.getPosition()),
			CEREAL_NVP(c.getSize()),
			CEREAL_NVP(c.getRotation()),
			CEREAL_NVP(c.getFillColor())
		);
	}*/

	template<class Archive>
	void save(Archive& archive,
		sf::RectangleShape const& c)
	{
		archive(
			cereal::make_nvp("position",c.getPosition()),
			cereal::make_nvp("size",c.getSize()),
			cereal::make_nvp("rotation", c.getRotation()),
			cereal::make_nvp("color", c.getFillColor())
		);
	}

	template<class Archive>
	void load(Archive& archive,
		sf::RectangleShape& c)
	{
		sf::Vector2f position;
		sf::Vector2f size;
		float rotation;
		sf::Color color;

		// Load (deserialize)
		archive(
			cereal::make_nvp("position", position),   // Position (sf::Vector2f)
			cereal::make_nvp("size", size),       // Size (sf::Vector2f)
			cereal::make_nvp("rotation", rotation),   // Rotation (float)
			cereal::make_nvp("color", color)       // Fill color (sf::Color)
		);

		// Set the deserialized values back into the sf::RectangleShape object
		c.setPosition(position);
		c.setSize(size);
		c.setRotation(rotation);
		c.setFillColor(color);
	}
}

namespace cereal
{
	template<class Archive, class F, class S>
	void save(Archive& ar, const std::pair<F, S>& pair)
	{
		ar(pair.first, pair.second);
	}

	template<class Archive, class F, class S>
	void load(Archive& ar, std::pair<F, S>& pair)
	{
		ar(pair.first, pair.second);
	}

	template <class Archive, class F, class S>
	struct specialize<Archive, std::pair<F, S>, cereal::specialization::non_member_load_save> {};
}