#pragma once
#include "EnviromentObjectsType.h"

namespace ImGui {
	//ChatGPT function
	// Helper function to convert enum values to strings for ImGui display
	static const char* ShapeTypeToString(ShapeType shape) {
		switch (shape) {
		case ShapeType::EnvironmentLine: return "EnvironmentLine";
		case ShapeType::StaticTarget:    return "StaticTarget";
		case ShapeType::MovingTarget:    return "MovingTarget";
		case ShapeType::Player:          return "Player";
		case ShapeType::None:            return "None";
		default:                         return "Unknown";
		}
	}

	//ChatGPT function
	// Helper function to convert string to enum
	static ShapeType StringToShapeType(const std::string& str) {
		if (str == "EnvironmentLine") return ShapeType::EnvironmentLine;
		if (str == "StaticTarget")    return ShapeType::StaticTarget;
		if (str == "MovingTarget")    return ShapeType::MovingTarget;
		if (str == "Player")          return ShapeType::Player;
		if (str == "None")            return ShapeType::None;
		return ShapeType::None;
	}
}

namespace sf {
	//ChatGPT function
	static std::vector<sf::Vector2f> GetRectangleCorners(const sf::RectangleShape& rect) {
		// Retrieve the local dimensions of the rectangle
		sf::Vector2f size = rect.getSize();
		sf::Transform transform = rect.getTransform();

		// Calculate the actual corners in world space
		std::vector<sf::Vector2f> corners = {
			transform.transformPoint({0.f, 0.f}),                     // Top-left
			transform.transformPoint({size.x, 0.f}),                  // Top-right
			transform.transformPoint({size.x, size.y}),               // Bottom-right
			transform.transformPoint({0.f, size.y})                   // Bottom-left
		};

		return corners;
	}
}

namespace Physics {

	// LINE/LINE
	//https://stackoverflow.com/questions/3746274/line-intersection-with-aabb-rectangle
	static bool Intersects(glm::vec2 a1, glm::vec2 a2, glm::vec2 b1, glm::vec2 b2, glm::vec2& intersection)
	{
		intersection = glm::vec2(0.0f);

		glm::vec2 b = a2 - a1;
		glm::vec2 d = b2 - b1;
		float bDotDPerp = b.x * d.y - b.y * d.x;

		// if b dot d == 0, it means the lines are parallel so have infinite intersection points
		if (bDotDPerp == 0)
			return false;

		glm::vec2 c = b1 - a1;
		float t = (c.x * d.y - c.y * d.x) / bDotDPerp;
		if (t < 0 || t > 1)
			return false;

		float u = (c.x * b.y - c.y * b.x) / bDotDPerp;
		if (u < 0 || u > 1)
			return false;

		intersection = a1 + t * b;

		return true;
	}


	// LINE/RECTANGLE
	static bool LineRect(glm::vec2 start, glm::vec2 end, glm::vec2 A, glm::vec2 B, glm::vec2 C, glm::vec2 D, glm::vec2& Intersection) {
		// Initialize the closest distance to a very large value
		float closestDistance = std::numeric_limits<float>::max();
		bool hasIntersection = false;

		// Temporary variable to store intersection points
		glm::vec2 intersection;

		// Check each side of the rectangle for intersection
		if (Intersects(start, end, A, B, intersection)) { // Left side
			hasIntersection = true;
			float dist = glm::distance2(start, intersection);
			if (dist < closestDistance) {
				closestDistance = dist;
				Intersection = intersection;
			}
		}

		if (Intersects(start, end, B, C, intersection)) { // Right side
			hasIntersection = true;
			float dist = glm::distance2(start, intersection);
			if (dist < closestDistance) {
				closestDistance = dist;
				Intersection = intersection;
			}
		}

		if (Intersects(start, end, C, D, intersection)) { // Top side
			hasIntersection = true;
			float dist = glm::distance2(start, intersection);
			if (dist < closestDistance) {
				closestDistance = dist;
				Intersection = intersection;
			}
		}

		if (Intersects(start, end, D, A, intersection)) { // Bottom side
			hasIntersection = true;
			float dist = glm::distance2(start, intersection);
			if (dist < closestDistance) {
				closestDistance = dist;
				Intersection = intersection;
			}
		}

		// Return whether an intersection was found and the closest point
		return hasIntersection;
	}
}