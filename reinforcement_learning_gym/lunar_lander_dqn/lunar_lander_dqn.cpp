/**
 * This notebook shows how to get started with training reinforcement learning agents,
 * particularly DQN agents, using mlpack. Here, we train a
 * [Simple DQN](https://www.cs.toronto.edu/~vmnih/docs/dqn.pdf) agent to get high 
 * scores for the [LunarLander-v2](https://gym.openai.com/envs/LunarLander-v2/)
 * environment.
 * We make the agent train and test on OpenAI Gym toolkit's GUI interface provided 
 * through a distributed infrastructure (TCP API). More details can be found 
 * [here](https://github.com/zoq/gym_tcp_api).
 * A video of the trained agent can be seen in the end.
 */

// Including necessary libraries and namespaces
#include <mlpack.hpp>

#include "../gym/environment.hpp"

using namespace mlpack;
using namespace ens;

// Set up the state and action space.
constexpr size_t stateDimension = 8;
constexpr size_t actionSize = 4;

template<typename EnvironmentType,
         typename NetworkType,
         typename UpdaterType,
         typename PolicyType,
         typename ReplayType = RandomReplay<EnvironmentType>>
void Train(
    gym::Environment& env,
    QLearning<EnvironmentType, NetworkType, UpdaterType, PolicyType>& agent,
    RandomReplay<EnvironmentType>& replayMethod,
    TrainingConfig& config,
    std::vector<double>& returnList,
    size_t& episodes,
    size_t& consecutiveEpisodes,
    const size_t numSteps)
{
  agent.Deterministic() = false;
  std::cout << "Training for " << numSteps << " steps." << std::endl;
  while (agent.TotalSteps() < numSteps)
  {
    double episodeReturn = 0;
    env.reset();
    do
    {
      agent.State().Data() = env.observation;
      agent.SelectAction();
      arma::mat action = {double(agent.Action().action)};

      env.step(action);
      DiscreteActionEnv<stateDimension, actionSize>::State nextState;
      nextState.Data() = env.observation;

      replayMethod.Store(
          agent.State(), agent.Action(), env.reward, nextState, env.done, 0.99);
      episodeReturn += env.reward;
      agent.TotalSteps()++;
      if (agent.Deterministic()
          || agent.TotalSteps() < config.ExplorationSteps())
      {
        continue;
      }
      agent.TrainAgent();
    } while (!env.done);
    returnList.push_back(episodeReturn);
    episodes += 1;

    if (returnList.size() > consecutiveEpisodes)
      returnList.erase(returnList.begin());

    double averageReturn =
        std::accumulate(returnList.begin(), returnList.end(), 0.0)
        / returnList.size();
    if (episodes % 5 == 0)
    {
      std::cout << "Avg return in last " << returnList.size()
                << " episodes: " << averageReturn << "\t" << episodes
                << "the episode return: " << episodeReturn
                << "\t Steps: " << agent.TotalSteps() << std::endl;
    }
  }
}

int main()
{
  // Initializing the agent.
  // Set up the network.
  FFN<MeanSquaredError, GaussianInitialization> network(
      MeanSquaredError(), GaussianInitialization(0, 1));
  network.Add<Linear>(128);
  network.Add<ReLU>();
  network.Add<Linear>(actionSize);

  SimpleDQN<> model(network);

  // Set up the policy and replay method.
  GreedyPolicy<DiscreteActionEnv<stateDimension, actionSize>> 
      policy(1.0, 2000, 0.1, 0.99);
  RandomReplay<DiscreteActionEnv<stateDimension, actionSize>> 
      replayMethod(64, 100000);

  // Set up training configurations.
  TrainingConfig config;
  config.ExplorationSteps() = 100;
  config.DoubleQLearning() = false;

  // Set up DQN agent.
  QLearning<DiscreteActionEnv<stateDimension, actionSize>,
            decltype(model),
            AdamUpdate,
            decltype(policy)>
      agent(config, model, policy, replayMethod);

  // Preparation for training the agent.
  // Set up the gym training environment.
  gym::Environment env("localhost", "4040", "LunarLander-v2");

  // Initializing training variables.
  std::vector<double> returnList;
  size_t episodes = 0;
  bool converged = true;

  // The number of episode returns to keep track of.
  size_t consecutiveEpisodes = 50;

  // Let the training begin.
  // Training the agent for a total of at least 10000 steps.
  Train(env,
        agent,
        replayMethod,
        config,
        returnList,
        episodes,
        consecutiveEpisodes,
        10000);

  // Testing the trained agent.
  agent.Deterministic() = true;

  // Creating and setting up the gym environment for testing.
  gym::Environment envTest("localhost", "4040", "LunarLander-v2");
  envTest.monitor.start("./dummy/", true, true);

  // Resets the environment.
  envTest.reset();
  envTest.render();

  double totalReward = 0;
  size_t totalSteps = 0;

  // Testing the agent on gym's environment.
  while (true)
  {
    // State from the environment is passed to the agent's internal representation.
    agent.State().Data() = envTest.observation;

    // With the given state, the agent selects an action according to its defined policy.
    agent.SelectAction();

    // Action to take, decided by the policy.
    arma::mat action = {double(agent.Action().action)};

    envTest.step(action);
    totalReward += envTest.reward;
    totalSteps += 1;

    if (envTest.done)
    {
      std::cout << " Total steps: " << totalSteps
                << "\t Total reward: " << totalReward << std::endl;
      break;
    }

    // Uncomment the following lines to see the reward and action in each step.
    // std::cout << " Current step: " << totalSteps << \"\t current reward: "
    //   << totalReward << \"\t Action taken: " << action;
  }

  std::cout << envTest.url() << std::endl;

  // A little more training...
  Train(env,
        agent,
        replayMethod,
        config,
        returnList,
        episodes,
        consecutiveEpisodes,
        100000);

  /*
   * Final agent testing!
   * *Note*: If you don't find a satisfactory output, please rerun the code block
   * below. It's not guaranteed that the agent will receive high rewards on all
   * test runs.
   */

  agent.Deterministic() = true;

  envTest.monitor.start("./dummy/", true, true);

  // Resets the environment.
  envTest.reset();

  totalReward = 0;
  totalSteps = 0;

  // Testing the agent on gym's environment.
  while (true)
  {
    // State from the environment is passed to the agent's internal representation.
    agent.State().Data() = envTest.observation;

    // With the given state, the agent selects an action according to its defined policy.
    agent.SelectAction();

    // Action to take, decided by the policy.
    arma::mat action = {double(agent.Action().action)};

    envTest.step(action);
    totalReward += envTest.reward;
    totalSteps += 1;

    if (envTest.done)
    {
      std::cout << " Total steps: " << totalSteps
                << "\t Total reward: " << totalReward << std::endl;
      break;
    }

    // Uncomment the following lines to see the reward and action in each step.
    // std::cout << " Current step: " << totalSteps << \"\t current reward: "
    //   << totalReward << \"\t Action taken: " << action;
  }

  envTest.close();
  std::cout << envTest.url() << std::endl;
}
