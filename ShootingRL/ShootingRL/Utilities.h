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

	glm::vec2 RotateGlmVector(glm::vec2 vec, float rotation) {
		float angleInRadians = glm::radians(rotation);
		glm::mat2 rotationMatrix = glm::mat2(
			glm::cos(angleInRadians), -glm::sin(angleInRadians),
			glm::sin(angleInRadians), glm::cos(angleInRadians)
		);

		// Rotate the vector using the matrix
		return rotationMatrix * vec;
	}

	//Separating Axis Theorem base on https://stackoverflow.com/questions/62028169/how-to-detect-when-rotated-rectangles-are-colliding-each-other
	// with some chantGPT modifications
	
	static float Project(const sf::Vector2f& point, const sf::Vector2f& axis) {
		return point.x * axis.x + point.y * axis.y;  // Dot product
	}

	// Function to check if projections on the axis overlap
	static bool Overlap(float minA, float maxA, float minB, float maxB) {
		return !(maxA < minB || maxB < minA);  // No overlap if one is completely on one side
	}

	// Function to check if two rectangles (rect1 and rect2) intersect using SAT
	static bool RectanglesIntersect(const sf::RectangleShape& rect1, const sf::RectangleShape& rect2) {
		std::vector<sf::Vector2f> corners1 = GetRectangleCorners(rect1);
		std::vector<sf::Vector2f> corners2 = GetRectangleCorners(rect2);

		// Calculate the axes for rect1 (edges' normals)
		std::vector<sf::Vector2f> axes = {
			sf::Vector2f(rect1.getSize().y, 0.f),  // Normal to the top/bottom edges (Y axis)
			sf::Vector2f(0.f, rect1.getSize().x)   // Normal to the left/right edges (X axis)
		};

		// Loop through all 4 edges of rect2 and calculate its normals
		for (size_t i = 0; i < 4; ++i) {
			sf::Vector2f edge = corners2[(i + 1) % 4] - corners2[i];
			sf::Vector2f axis = sf::Vector2f(-edge.y, edge.x);  // Perpendicular to the edge (normal)
			axes.push_back(axis);
		}

		// For each axis, project both rectangles' corners and check for overlap
		for (const auto& axis : axes) {
			// Project all corners of rect1 and rect2 onto the axis
			float min1 = Project(corners1[0], axis);
			float max1 = min1;
			for (const auto& corner : corners1) {
				float projection = Project(corner, axis);
				min1 = std::min(min1, projection);
				max1 = std::max(max1, projection);
			}

			float min2 = Project(corners2[0], axis);
			float max2 = min2;
			for (const auto& corner : corners2) {
				float projection = Project(corner, axis);
				min2 = std::min(min2, projection);
				max2 = std::max(max2, projection);
			}

			// If the projections do not overlap on any axis, the rectangles do not intersect
			if (!Overlap(min1, max1, min2, max2)) {
				return false;
			}
		}

		// If all projections overlap, the rectangles intersect
		return true;
	}




	/*{
		glm::vec2 size,origin = (A+B+C+D)/4.0f;
		float Rotation;
		glm::vec2 axX = RotateGlmVector(glm::vec2(1.0f, 0.0f), Rotation);
		glm::vec2 axY = RotateGlmVector(glm::vec2(0.0f, 1.0f), Rotation);
		axX = axX * (size.x / 2.0f);
		axY = axY * (size.y / 2.0f);
		return {
				origin+axX+axY,
				origin + axX - axY,
				origin - axY - axX,
				origin + axY - axX,
		}
	}*/

	//glm::vec2 Project(const glm::vec2& point, const glm::vec2& lineOrigin, const glm::vec2& lineDirection) {
	//	// Ensure the line direction is normalized
	//	glm::vec2 dir = glm::normalize(lineDirection);

	//	// Compute the vector from the line origin to the point
	//	glm::vec2 originToPoint = point - lineOrigin;

	//	// Project the point onto the line
	//	float dotValue = glm::dot(originToPoint, dir);
	//	return lineOrigin + dir * dotValue;
	//}

	//{
	//	const isProjectionHit = (minSignedDistance < 0 && maxSignedDistance > 0
	//		|| Math.abs(minSignedDistance) < rectHalfSize
	//		|| Math.abs(maxSignedDistance) < rectHalfSize);
	//}
}