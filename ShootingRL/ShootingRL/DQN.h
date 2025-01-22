#pragma once
#include <torch/nn.h>
#include <torch/nn/functional.h>
#include <torch/nn/module.h>
#include <torch/optim.h>
#include <torch/torch.h>

#include <string>

#include "EnvironmentReturnValues.h"
#include "EnviromentObjectsType.h"
#include "SFML/Graphics.hpp"
#include "LevelData.h"


struct Tensor_step_return
{
	torch::Tensor states;
	torch::Tensor actions;
	torch::Tensor next_states;
	torch::Tensor rewards;
	torch::Tensor dones;
};

class QNetworkImpl : public torch::nn::Module
{
public:
	QNetworkImpl(int input_channels, int action_size, int seed);

	QNetworkImpl(int input_channels, int action_size);
	QNetworkImpl() {};
	int num_flat_features(torch::Tensor x);
	torch::Tensor forward(torch::Tensor x);
	void resetNetwork();

	torch::nn::Linear fc1{ nullptr }, fc2{ nullptr }, fc3{ nullptr };
	torch::nn::Conv2d conv1{ nullptr }, conv2{ nullptr }/*, conv3{ nullptr }*/;
};

TORCH_MODULE(QNetwork);

class ReplayBuffer
{
public:
	ReplayBuffer(int state_size, int action_size, int buffer_size, int batch_size, int seed);
	ReplayBuffer(int action_size, int buffer_size, int batch_size);
	ReplayBuffer() {};

	void add(Optimize_Step_return experience);  //(State state, Action action, float reward, State next_state, bool done);
	void addBulk(std::vector<Optimize_Step_return>& experiences);
	Tensor_step_return sample();

	int state_size;
	int action_size;
	int buffer_size;
	int batch_size;

	std::vector<Optimize_Step_return> experiences;
	int seed;

	std::vector<float> actions;
	std::vector<float> rewards;
	std::vector<float> dones;
};
class DQN
{
public:
	DQN(int state_size, int action_size, int seed);
	DQN(int state_size, int action_size);
	DQN() {};
	void step();  //(State state, Action action, float reward, State next_state, bool done);
	void addToExperienceBuffer(Optimize_Step_return value);
	void addToExperienceBufferInBulk(std::vector<Optimize_Step_return>& values);
	int act(const sf::Image& image, float epsilon);
	void learn(Tensor_step_return experiences);
	void update_fixed_network(QNetwork& local_model, QNetwork& target_model);
	void checkpoint(std::string filepath);
	void loadCheckpoint(std::string filepath);
	void resetLearning();

	int state_size, action_size, seed;

	QNetwork q_network, fixed_network;
	torch::optim::Adam* optimizer;

	ReplayBuffer buffer;
	int timestep = 0;

	int whenToPrint = 1000;
	int currentStep = 0;
};

torch::Tensor convertToTensor(const sf::Image& image);