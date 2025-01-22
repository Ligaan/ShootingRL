#include "DQN.h"

#include <torch/serialize/output-archive.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <iterator>
#include <random>
#include <vector>

unsigned seed;
const int BUFFER_SIZE = int(2e5);  // 1e5
const int BATCH_SIZE = /*128*/ 64;
const float GAMMA = 0.99f;
float TAU = 1e-3;
float LR = 5e-4;
int UPDATE_EVERY = 108; /*16;*/

DQN::DQN(int state_size, int action_size, int seed)
{
    this->state_size = state_size;
    this->action_size = action_size;
    this->seed = seed;

    q_network = QNetwork(4, action_size, seed);
    fixed_network = QNetwork(4, action_size, seed);
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

void DQN::addToExperienceBuffer(Optimize_Step_return value)
{
    buffer.add(value);  //(state, action, reward, next_state, done);
    timestep++;
}

void DQN::addToExperienceBufferInBulk(std::vector<Optimize_Step_return>& values)
{
    timestep += values.size();
    buffer.addBulk(values);
}

torch::Tensor convertToTensor(const sf::Image& image) {
    const sf::Uint8* pixels = image.getPixelsPtr();
    unsigned int width = image.getSize().x;
    unsigned int height = image.getSize().y;

    // Convert the image data to a PyTorch tensor
    torch::Tensor tensor_image = torch::from_blob(
        const_cast<sf::Uint8*>(pixels), { 1, 4, height, width }, torch::kByte
    );

    // Normalize to [0, 1] by converting to float and dividing by 255
    tensor_image = tensor_image.to(torch::kFloat).div(255);

    return tensor_image;
}

int DQN::act(const sf::Image& image, float epsilon)
{
    torch::NoGradGuard no_grad;

    int action;
    float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    if (r > epsilon)
    {
        torch::Tensor t_state = convertToTensor(image);
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
    torch::Tensor Q_expected = q_network->forward(experiences.states).gather(1, experiences.actions.to(torch::kLong).view({ -1, 1 }));
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


    torch::manual_seed(seed);

    conv1 = register_module("conv1", torch::nn::Conv2d(torch::nn::Conv2dOptions(4, 6, 3))); // 32 filters, kernel size 8x8, stride 4
    conv2 = register_module("conv2", torch::nn::Conv2d(torch::nn::Conv2dOptions(6, 16, 3)));            // 64 filters, kernel size 4x4, stride 2

    // Fully connected layers
    fc1 = register_module("fc1", torch::nn::Linear(torch::nn::LinearOptions(16 * 198 * 198, 120)));  // Adjust the size based on input image dimensions
    fc2 = register_module("fc2", torch::nn::Linear(torch::nn::LinearOptions(120, 84)));
    fc3 = register_module("fc3", torch::nn::Linear(torch::nn::LinearOptions(84, 7)));

}

QNetworkImpl::QNetworkImpl(int input_channels, int action_size) { QNetwork(input_channels, action_size, 0); }

int QNetworkImpl::num_flat_features(torch::Tensor x)
{
    auto sz = x.sizes().slice(1);  // All dimensions except the batch dimension
    int num_features = 1;
    for (auto s : sz) {
        num_features *= s;
    }
    return num_features;
}

torch::Tensor QNetworkImpl::forward(torch::Tensor x)
{

    // Pass through conv1, relu, and max pooling
    x = torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions({ 2, 2 }))->forward(torch::relu(conv1->forward(x)));

    // Pass through conv2, relu, and max pooling
    x = torch::nn::MaxPool2d(torch::nn::MaxPool2dOptions({ 2, 2 }))->forward(torch::relu(conv2->forward(x)));

    // Flatten the tensor for the fully connected layers
    x = x.view({ -1, num_flat_features(x) });

    // Pass through the fully connected layers with ReLU activations
    x = torch::relu(fc1->forward(x));
    x = torch::relu(fc2->forward(x));

    // Output layer (no activation here, softmax will be applied later)
    x = fc3->forward(x);

    // Apply softmax to the output (convert logits to probabilities)
    x = torch::softmax(x, /*dim=*/1);  // Apply softmax along the second dimension (action dimension)

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

void ReplayBuffer::add(Optimize_Step_return experience)
{
    experiences.push_back(experience);
    if (experiences.size() > buffer_size) experiences.erase(experiences.begin());
}

void ReplayBuffer::addBulk(std::vector<Optimize_Step_return>& experiences)
{
    this->experiences.insert(this->experiences.end(), std::make_move_iterator(experiences.begin()),
        std::make_move_iterator(experiences.end()));
    if (this->experiences.size() > buffer_size)
    {
        this->experiences.erase(this->experiences.begin(),
            this->experiences.begin() + (this->experiences.size() - buffer_size));
    }
}

Tensor_step_return ReplayBuffer::sample()
{
    Tensor_step_return tensor;
    std::vector<Optimize_Step_return> batch;
    std::sample(experiences.begin(), experiences.end(), std::back_inserter(batch), batch_size,
        std::mt19937{ std::random_device{}() });
    std::shuffle(batch.begin(), batch.end(), std::mt19937{ std::random_device{}() });

    rewards.clear();
    actions.clear();
    dones.clear();

    // Float_State state;
    torch::Tensor s_tensor;

    std::vector<torch::Tensor> currentStates;
    std::vector<torch::Tensor> nextStates;

    //torch::stack(tensors, 0);

    int i = 0;
    for (auto& experience : batch)
    {
        i++;

        currentStates.push_back(experience.state.squeeze(0));

        nextStates.push_back(experience.next_state.squeeze(0));

        rewards.push_back(experience.reward);
        actions.push_back(static_cast<float>(experience.action));
        dones.push_back(static_cast<float>(experience.terminated));
        if (i == BATCH_SIZE) break;
    }

    tensor.states = torch::stack(currentStates, 0);
    tensor.next_states = torch::stack(nextStates, 0);
    tensor.actions = torch::from_blob((float*)(actions.data()), actions.size()).unsqueeze(1);
    tensor.rewards = torch::from_blob((float*)(rewards.data()), rewards.size()).unsqueeze(1);
    tensor.dones = torch::from_blob((float*)(dones.data()), dones.size()).unsqueeze(1);
    return tensor;
}