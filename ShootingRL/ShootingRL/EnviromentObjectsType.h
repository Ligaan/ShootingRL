#pragma once
#include <torch/torch.h>

enum class ShapeType {
	EnvironmentLine,
	StaticTarget,
	MovingTarget,
	Player,
	None
};

enum class Action
{
	Up,
	Down,
	Left,
	Right,
	TurnLeft,
	TurnRight,
	Shoot
};

struct Optimize_Step_return
{
	torch::Tensor state;
	torch::Tensor next_state;
	Action action;
	float reward;
	bool terminated;
	bool truncated;
};