#include "LevelData.h"
#include "DQN.h"

float average(std::vector<float>& scores)
{
	float total = 0.0f;
	for (auto& value : scores) total += value;
	total /= scores.size();
	return total;
}

struct TrainingEnv
{
	LevelData* env;
	float Score = 0.0f;
	float step_score = 0.0f;
	int steps = 0;
	bool done = true;
};

DQN* agent;
const int nEnv = 32;
//std::vector<TrainingEnv> envs;
TrainingEnv env;

//debug values
bool seePreviouseMeanScores = false;
int Episode = 0;
float Score = 0.0f;
float step_score = 0.0f;
float mean_score = 0.0f;
float eps;
int selectedPerspective = 0;
std::vector<float> scores;
std::vector<float> mean_scores;

bool done = false;
bool trainingDone = false;
bool crashed;
bool slowStep = false;
bool continueStep = false;
std::string path, newtorkPath;
unsigned _seed;

const int max_episodes = 20000;
const int max_steps = 10000;
const int print_every = 320;

const float eps_start = 0.9f;
const float eps_decay = /*0.999f;*/ 500000;
const float eps_min = 0.01f;


void train(float dt, sf::RenderWindow& window)
{
	Action action;

	static int episode = 0;
	static int stepsDone = 0;
	static float score = 0;
	static int t = max_steps;

	std::vector<Step_return> steps;
	//env.env->StateToFloat_State(envs[i].env->squizzForNetwork(envs[i].env->currentState))
	//State state;
	bool done = true;
	Step_return step_return;
	if (episode <= max_episodes)
	{
		done = env.done;
		if (!done)
		{
			if (!env.done)
			{
				eps = eps_min + (eps_start - eps_min) * exp(-1. * stepsDone / eps_decay);
				action = static_cast<Action>(agent->act(
					env.env->prevStep, eps));
				step_return = env.env->Update(/*dt*/ 0.005f, action);

				if (!env.env->IsSimulationRunning())
					env.env->SelectModWindow();
				env.env->SaveLoadWindow();
				env.env->RunSimulation();
				ImGui::End();

				window.clear();
				env.env->Draw(window);
				ImGui::SFML::Render(window);
				window.display();

				sf::Texture texture;

				// Capture the window contents to the texture
				texture.create(window.getSize().x, window.getSize().y);
				texture.update(window);

				// Create an image from the texture
				sf::Image screenshot = texture.copyToImage();

				step_return.next_state = env.env->prevStep = screenshot;


				// agent->addToExperienceBuffer(envs[i].env->StepReturnToFullFLoatStepReturn(step_return));
				steps.push_back(step_return);
				env.steps++;
				env.step_score = step_return.reward;
				env.Score += step_return.reward;

				step_score = step_return.reward;

				if (step_return.terminated || env.steps >= max_steps)
				{
					env.done = true;
				}

				stepsDone++;
			}
			agent->addToExperienceBufferInBulk(steps);
			agent->step();
		}
		else
		{
			// debug values
			Episode = episode;
			Score = env.Score;
			scores.push_back(env.Score);
			env.env->LoadData(std::string(env.env->lastLoadedFile));
			env.done = false;
			env.Score = 0.0f;
			env.steps = 0;
			episode++;

			if (episode % print_every == 0)
			{
				mean_score = average(scores);
				mean_scores.push_back(mean_score);
				std::string path;
				std::cout << mean_score << "\n";
				path = "Checkpoints/" + std::to_string(episode);
				//path = Engine.FileIO().GetPath(bee::FileIO::Directory::Asset, path);
				//agent->checkpoint(path);
			}
			window.clear();
			env.env->Draw(window);
			ImGui::SFML::Render(window);
			window.display();

			sf::Texture texture;

			// Capture the window contents to the texture
			texture.create(window.getSize().x, window.getSize().y);
			texture.update(window);

			// Create an image from the texture
			sf::Image screenshot = texture.copyToImage();

			step_return.next_state = env.env->prevStep = screenshot;
		}
	}
	else
	{
		/*for (auto& env : envs)
		{
			env.env->envState = EnvironmentState::Run;
			env.env->player = Player::Ai;
		}*/
		agent->q_network->eval();
	}
}

int main()
{
	agent = new DQN(4, 7, 0);  //(8, 4, 0);
	env = TrainingEnv{};
	env.env = new LevelData();
	//////////////
	auto window = sf::RenderWindow({ /*1920u, 1080u*/ 800u,800u }, "CMake SFML Project");
	window.setFramerateLimit(144);
	if (!ImGui::SFML::Init(window))
		return -1;

	//LevelData levelData;

	sf::Clock clock;
	sf::Time timer;
	while (window.isOpen())
	{

		ImGui::SFML::Update(window, timer = clock.restart());
		env.env->ResetInput();
		ImGui::Begin("Hello, world!");

		for (auto event = sf::Event(); window.pollEvent(event);)
		{
			ImGui::SFML::ProcessEvent(window, event);

			if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Escape))
			{
				window.close();
			}
			env.env->CheckWindowEvent(event, window);
		}

		if (!env.env->IsTraining()) {
			if (!env.env->IsSimulationRunning())
				env.env->SelectModWindow();
			env.env->SaveLoadWindow();
			env.env->RunSimulation();
			ImGui::End();

			window.clear();
			env.env->Draw(window);
			ImGui::SFML::Render(window);
			window.display();


			sf::Texture texture;

			// Capture the window contents to the texture
			texture.create(window.getSize().x, window.getSize().y);
			texture.update(window);

			// Create an image from the texture
			sf::Image screenshot = texture.copyToImage();

			if (env.env->IsSimulationRunning())
				env.env->Update(timer.asSeconds(), Action::Down);
		}
		else {
			train(timer.asSeconds(), window);
		}

	}

	ImGui::SFML::Shutdown();
}