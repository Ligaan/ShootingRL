#include "DQN.h"

#include <torch/serialize/output-archive.h>

#include <algorithm>
#include <iostream>
#include <iterator>
#include <random>
#include <vector>

unsigned seed;
const int BUFFER_SIZE = int(2e5);  // 1e5
const int BATCH_SIZE = /*128*/ 64;
const float GAMMA = 0.99f;
float TAU = 1e-3;
float LR = 5e-4;
int UPDATE_EVERY = 32; /*16;*/

DQN::DQN(int state_size, int action_size, int seed)
{
    this->state_size = state_size;
    this->action_size = action_size;
    this->seed = seed;

    q_network = QNetwork(state_size, action_size, seed);
    fixed_network = QNetwork(state_size, action_size, seed);
    auto adamOptions = torch::optim::AdamOptions(0.0001);
    optimizer = new torch::optim::Adam(q_network->parameters(), adamOptions);
    buffer = ReplayBuffer(state_size, action_size, BUFFER_SIZE, BATCH_SIZE, seed);
}

DQN::DQN(int state_size, int action_size) { DQN(state_size, action_size, 0); }

void DQN::step()
{
    if (timestep >= UPDATE_EVERY)
    {
        if (buffer.experiences.size() > BATCH_SIZE)
        {
            Tensor_step_return sampled_experiences = buffer.sample();
            // printf("%i\n",buffer.experiences.size());
            learn(sampled_experiences);
        }
        timestep = timestep % UPDATE_EVERY;
    }
}

void DQN::addToExperienceBuffer(Step_return value)
{
    buffer.add(value);  //(state, action, reward, next_state, done);
    timestep++;
}

void DQN::addToExperienceBufferInBulk(std::vector<Step_return>& values)
{
    timestep += values.size();
    buffer.addBulk(values);
}

torch::Tensor imageToTensor(const sf::Image& image) {
    const sf::Uint8* pixels = image.getPixelsPtr();
    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;

    // Convert SFML's Uint8 RGBA data into a normalized float vector
    std::vector<float> normalizedPixels;
    normalizedPixels.reserve(width * height * 4); // RGBA has 4 channels
    for (unsigned int i = 0; i < width * height * 4; ++i) {
        normalizedPixels.push_back(pixels[i] / 255.0f); // Normalize to [0, 1]
    }

    // Convert to a LibTorch tensor
    torch::Tensor tensor = torch::from_blob(normalizedPixels.data(), { 1, 4, height, width }, torch::kFloat);

    // Rearrange dimensions from [Batch, Channels, Height, Width] if needed
    tensor = tensor.permute({ 0, 2, 3, 1 }); // Convert [1, Height, Width, Channels] -> [1, Channels, Height, Width]
    return tensor.clone(); // Clone to ensure tensor owns its memory
}

int DQN::act(const sf::Image& image, float epsilon)
{
    torch::NoGradGuard no_grad;

    int action;
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    if (r > epsilon)
    {
        torch::Tensor t_state = imageToTensor(image);
        torch::Tensor action_values;

        action_values = q_network->forward(t_state);
        action = static_cast<int>(torch::argmax(action_values).item().toInt() % action_size);
        currentStep++;
        // std::cout << t_state << "\n" << action_values << "\n" << action << "\n\n";
        currentStep -= whenToPrint;
    }
    else
    {
        action = static_cast<int>(rand() % action_size);
    }
    // std::cout << "\n" << action << "\n";
    return action;
}

void DQN::learn(Tensor_step_return experiences)
{
    // this->q_network->train();

    torch::Tensor action_values;
    torch::Tensor max_action_values;

    {
        torch::NoGradGuard no_grad;

        action_values = fixed_network->forward(experiences.next_states).detach();
        auto [ttt, stuff] = action_values.max(1);
        max_action_values = ttt.unsqueeze(1);
    }

    torch::Tensor Q_target = experiences.rewards + (GAMMA * max_action_values * (1 - experiences.dones));
    torch::Tensor Q_expected = q_network->forward(experiences.states).gather(1, experiences.actions.to(torch::kLong));
    torch::Tensor loss = torch::nn::functional::mse_loss(Q_expected, Q_target);
    // std::cout << Q_target << "\n" << Q_expected << "\n" << loss << "\n";
    optimizer->zero_grad();

    loss.backward();
    optimizer->step();

    update_fixed_network(q_network, fixed_network);

    // this->q_network->eval();
}

void DQN::update_fixed_network(QNetwork& local_model, QNetwork& target_model)
{
    torch::NoGradGuard no_grad;

    for (int i = 0; i < q_network->parameters().size(); i++)
    {
        fixed_network->parameters()[i].data().copy_(TAU * q_network->parameters()[i].data() +
            (1.0f - TAU) * fixed_network->parameters()[i].data());
    }
}

void DQN::checkpoint(std::string filepath)
{
    torch::save(q_network, (filepath + "_network.pt").c_str());
    torch::save(*optimizer, (filepath + "_optimizer.pt").c_str());
}

void DQN::loadCheckpoint(std::string filepath)
{
    torch::load(q_network, (filepath + "_network.pt").c_str());
    torch::load(*optimizer, (filepath + "_optimizer.pt").c_str());
}

void DQN::resetLearning()
{
    q_network->resetNetwork();
    fixed_network->resetNetwork();
    delete optimizer;
    auto adamOptions = torch::optim::AdamOptions(0.0001);
    optimizer = new torch::optim::Adam(q_network->parameters(), adamOptions);
}

QNetworkImpl::QNetworkImpl(int input_channels, int action_size, int seed)
{
    /*torch::manual_seed(seed);
    fc1 = register_module("fc1", torch::nn::Linear(input_channels, 128));
    fc2 = register_module("fc2", torch::nn::Linear(128, 128));
    fc3 = register_module("fc3", torch::nn::Linear(128, action_size));*/

    torch::manual_seed(seed);

    // Convolutional layers
    conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(input_channels, 32, 8).stride(4))); // 32 filters, kernel size 8x8, stride 4
    conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(32, 64, 4).stride(2)));            // 64 filters, kernel size 4x4, stride 2
    conv3 = register_module("conv3", torch::nn::Conv2d(torch::nn::Conv2dOptions(64, 64, 3).stride(1)));            // 64 filters, kernel size 3x3, stride 1

    // Fully connected layers
    fc1 = register_module("fc1", torch::nn::Linear(64 * 9 * 9, 512));  // Adjust the size based on input image dimensions
    fc2 = register_module("fc2", torch::nn::Linear(512, action_size));
}

QNetworkImpl::QNetworkImpl(int input_channels, int action_size) { QNetwork(input_channels, action_size, 0); }

torch::Tensor QNetworkImpl::forward(torch::Tensor x)
{
    /*x = fc1(x);
    x = torch::relu(x);
    x = fc2(x);
    x = torch::relu(x);
    x = fc3(x);*/
    // Apply convolutional layers with ReLU activation
    x = torch::relu(conv1(x));
    x = torch::relu(conv2(x));
    x = torch::relu(conv3(x));

    // Flatten the tensor for fully connected layers
    x = x.view({ x.size(0), -1 }); // Flatten [Batch, Channels, Height, Width] -> [Batch, Features]

    // Fully connected layers
    x = torch::relu(fc1(x));
    x = fc2(x);

    return x;
}

void QNetworkImpl::resetNetwork()
{
    for (auto& layer : this->children())
    {
        layer.reset();
    }
}

ReplayBuffer::ReplayBuffer(int state_size, int action_size, int buffer_size, int batch_size, int seed)
{
    this->state_size = state_size;
    this->action_size = action_size;
    this->buffer_size = buffer_size;
    this->batch_size = batch_size;
    this->seed = seed;
}

ReplayBuffer::ReplayBuffer(int action_size, int buffer_size, int batch_size)
{
    this->buffer_size = buffer_size;
    this->batch_size = batch_size;
    this->seed = seed;
}

void ReplayBuffer::add(Step_return experience)
{
    experiences.push_back(experience);
    if (experiences.size() > buffer_size) experiences.erase(experiences.begin());
}

void ReplayBuffer::addBulk(std::vector<Step_return>& experiences)
{
    this->experiences.insert(this->experiences.end(), std::make_move_iterator(experiences.begin()),
        std::make_move_iterator(experiences.end()));
    if (this->experiences.size() > buffer_size)
    {
        /*std::list<Full_Float_Step_Return>::iterator itr1, itr2;
        itr1 = this->experiences.begin();
        itr2 = this->experiences.begin();
        std::advance(itr2, static_cast<int>(this->experiences.size()) - buffer_size);
        this->experiences.erase(itr1, itr2);*/
        this->experiences.erase(this->experiences.begin(),
            this->experiences.begin() + (this->experiences.size() - buffer_size));
    }
}

Tensor_step_return ReplayBuffer::sample()
{
    Tensor_step_return tensor;
    std::vector<Step_return> batch;
    std::sample(experiences.begin(), experiences.end(), std::back_inserter(batch), batch_size,
        std::mt19937{ std::random_device{}() });
    std::shuffle(batch.begin(), batch.end(), std::mt19937{ std::random_device{}() });

    rewards.clear();
    actions.clear();
    dones.clear();

    // Float_State state;
    torch::Tensor ns_tensor, s_tensor;

    int i = 0;
    for (auto& experience : batch)
    {
        i++;

        // state = StateToFloat_State(experience.state);
        s_tensor = imageToTensor(experience.state); /*torch::from_blob((float*)(experience.data.data()), state_size);*/
        if (i > 1)
            tensor.states = torch::cat({ tensor.states, s_tensor.unsqueeze(0) }, 0);
        else
            tensor.states = torch::cat({ s_tensor.unsqueeze(0) }, 0);

        // state = StateToFloat_State(experience.next_state);
        s_tensor = imageToTensor(experience.next_state);/*torch::from_blob((float*)(experience.data.data() + state_size), state_size);*/
        if (i > 1)
            tensor.next_states = torch::cat({ tensor.next_states, s_tensor.unsqueeze(0) }, 0);
        else
            tensor.next_states = torch::cat({ s_tensor.unsqueeze(0) }, 0);

        rewards.push_back(experience.reward);
        actions.push_back(static_cast<float>(experience.action));
        dones.push_back(static_cast<float>(experience.terminated));
        if (i == BATCH_SIZE) break;
    }

    tensor.actions = torch::from_blob((float*)(actions.data()), actions.size()).unsqueeze(1);
    tensor.rewards = torch::from_blob((float*)(rewards.data()), rewards.size()).unsqueeze(1);
    tensor.dones = torch::from_blob((float*)(dones.data()), dones.size()).unsqueeze(1);
    return tensor;
}