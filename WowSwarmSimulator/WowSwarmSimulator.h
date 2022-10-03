#pragma once

#include <vector>
#include <array>
#include <stdexcept>
#include <random>
#include <numeric>
#include <atomic>

// settings: 
// 5 man party or 20 man raid
// enemy count
// start swarm configuration (empty, random between 1 to 3, realistic random that could happen -> 2 targets have between 1 and 5 and 4 targets between 1 and 2)
// continuous mode: cast X swarms each with ~2 steps in between

// technique:
// count dot swarms of configuration with swarms added and without
// then store the net gain based on if you casted on friendly/enemy
// and at what stack count with group size and enemy count
//

// information:
// 25s CD // 9s debuff/buff
// one charge
// prioritizes targets without swarm (enemy or friendly)
// 1 enemy means 1 split swarm goes to enemy and one to friendly

struct Entity {
    bool isEnemy = true;
    int stacks = 0;
    float duration = 0;
};
typedef std::vector<Entity> E_vec;

struct Entities {
    E_vec* group = nullptr;
    E_vec* enemies = nullptr;
    size_t size = 0;
    size_t g_size = 0;
    size_t e_size = 0;

    Entities() {}

    Entities(E_vec& g, E_vec& e) {
        group = &g;
        enemies = &e;
        size = g.size() + e.size();
        g_size = g.size();
        e_size = e.size();
    }

    Entity& operator[](size_t index) {
        if (index >= size) {
            throw std::invalid_argument("received negative value");
        }
        if (index < group->size()) {
            return (*group)[index];
        }
        else {
            return (*enemies)[index - group->size()];
        }
    }
};

struct TravelingSwarm {
    int sourceIndex = 0;
    int targetIndex = 0;
    float timeToArrival = 0;
    float travelTime = 0;
    int stacks = 0;
};

struct SwarmStats {
    float simTime;
    std::array<std::array<int, 6>, 2> targetCasts{ 0 }; // vector of 2 vectors which holds cast target stats (
    int enemyTicks = 0;
    int friendlyTicks = 0;
};

struct SimResult {
    int groupSize = 0;
    int enemyCount = 0;
    bool hasCircle = false;
    char stratName[30] = "";
    std::array<int, 6> friendlyCasts{ 0 };
    std::array<int, 6> enemyCasts{ 0 };
    float ticksPerSecond = 0.0;
    float enemyTicksPerSecond = 0.0;
    float friendlyTicksPerSecond = 0.0;
};

enum class InitialConfiguration {
    EMPTY, RANDOM, REALISTIC
};

constexpr std::array<std::string_view, 3> initConfigNames{
    "EMPTY",
    "RANDOM",
    "REALISTIC"
};

enum class TargetStrategy {
    FOCUSFRIENDLY, // always cast on first friendly
    FOCUSENEMY, // always cast on first enemy
    RANDOM, // targets random enemy/friendly regardless of stacks
    RANDOMENEMY, // targets random enemy regardless of stacks
    RANDOMFRIENDLY, // targets random friendly regardless of stacks
    ONESTACK, // targets first enemy/friendly with 2 stacks then 1 stacks then 0 stacks in priority order
    ONESTACKENEMYFIRST, // targets first enemy/friendly with 1 stack then 0 stacks in priority order but begins searching enemy first
    ONESTACKENEMY, // targets first enemy with 1 stacks then 0 stacks in priority order
    ONESTACKFRIENDLY, // targets first friendly with 1 stacks then 0 stacks in priority order
    TWOSTACKS, // targets first enemy/friendly with 2 stacks then 1 stacks then 0 stacks in priority order
    TWOSTACKSENEMYFIRST, // targets first enemy/friendly with 2 stacks then 1 stacks then 0 stacks in priority order but begins searching enemy first
    TWOSTACKSENEMY, // targets first enemy with 2 stacks then 1 stacks then 0 stacks in priority order
    TWOSTACKSFRIENDLY, // targets first friendly with 2 stacks then 1 stacks then 0 stacks in priority order
    THREESTACKS, // targets first enemy/friendly with 3 stacks then 2 stacks then 1 stacks then 0 stacks in priority order
    THREESTACKSENEMYFIRST, // targets first enemy/friendly with 3 stacks then 2 stacks then 1 stacks then 0 stacks in priority order but begins searching enemy first
    THREESTACKSENEMY, // targets first enemy with 3 stacks then 2 stacks then 1 stacks then 0 stacks in priority order
    THREESTACKSFRIENDLY, // targets first friendly with 3 stacks then 2 stacks then 1 stacks then 0 stacks in priority order
    LOWEST, // targets first enemy/friendly with the lowest amount of stacks
    LOWESTENEMYFIRST, // targets first enemy/friendly with the lowest amount of stacks but begins searching enemies
    LOWESTENEMY, // targets random enemy with the lowest amount of stacks
    LOWESTFRIENDLY // targets random friendly with the lowest amount of stacks
};

constexpr std::array<std::string_view, 21> stratNames{
    "FOCUSFRIENDLY",
    "FOCUSENEMY",
    "RANDOM",
    "RANDOMENEMY",
    "RANDOMFRIENDLY",
    "ONESTACK",
    "ONESTACKENEMYFIRST",
    "ONESTACKFRIENDLY",
    "ONESTACKENEMY",
    "TWOSTACKS",
    "TWOSTACKSENEMYFIRST",
    "TWOSTACKSFRIENDLY",
    "TWOSTACKSENEMY",
    "THREESTACKS",
    "THREESTACKSENEMYFIRST",
    "THREESTACKSFRIENDLY",
    "THREESTACKSENEMY",
    "LOWEST",
    "LOWESTENEMYFIRST",
    "LOWESTENEMY",
    "LOWESTFRIENDLY"
};

struct SimParameters {
    /*
    float g_splitChance = 0.6f;
    float g_maxDuration = 12.0f;
    float g_cooldown = 25.0f;
    float g_maxTraveltime = 3.0f;
    float simTime = 10000.0f;
    float timeDelta = 0.1f;
    float simulationSpeed = 0.0f;
    InitialConfiguration initConfig = InitialConfiguration::EMPTY;

    TargetStrategy strategy = TargetStrategy::LOWEST;
    int friendCount = 1;
    int enemyCount = 1;
    bool hasCircle = false;
    */
    float g_splitChance = 0.6f;
    float g_maxDuration = 2.0f;
    float g_cooldown = 2.0f;
    float g_maxTraveltime = 5.0f;
    float simTime = 10000.0f;
    float timeDelta = 0.1f;
    float simulationSpeed = 1.0f;
    InitialConfiguration initConfig = InitialConfiguration::EMPTY;

    TargetStrategy strategy = TargetStrategy::RANDOMENEMY;
    int friendCount = 5;
    int enemyCount = 5;
    bool hasCircle = false;

    bool pauseSim = false;
};

struct SimData {
    std::atomic<bool> isBeingFetched = false;
    std::atomic<bool> isBusy = false;

    bool isRunning = false;
    E_vec group;
    E_vec enemies;
    Entities entities;
    std::vector<TravelingSwarm> travelingSwarms;
    SwarmStats stats;
};

std::vector<int> getRandomIndices(size_t interval, size_t count);
void InitializeEntities(E_vec& group, E_vec& enemies, Entities& entities, InitialConfiguration config, const SimParameters& parameters);
void castSwarm(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const TargetStrategy strategy, SwarmStats& stats, const SimParameters& parameters);
void propagateSwarm(int sourceIndex, int stacks, bool wasFriendly, Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const SimParameters& parameters);
void advanceTime(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const SimParameters& parameters);
void recordSwarmStats(Entities& entities, SwarmStats& stats);
void printStats(size_t friendlyCount, size_t enemyCount, SwarmStats stats, std::string_view name);
template<size_t T>
void exportResults(std::array<SimResult, T>* results);
void RunRealtimeSim(SimParameters& parameters, SimData& simData);
void RunSim(const SimParameters& parameters);