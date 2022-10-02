#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <stdexcept>
#include <numeric>
#include <fstream>
#include <filesystem>

// Choose a random mean between 1 and 6
std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> dist13(1, 3);
std::uniform_int_distribution<std::mt19937::result_type> dist15(1, 5);
std::uniform_int_distribution<std::mt19937::result_type> dist12(1, 2);
std::uniform_real_distribution<> splitDist(0.0, 1.0);


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

constexpr float g_splitChance = 0.6f;
static int g_maxDuration = 12;
constexpr int g_cooldown = 25;
constexpr int g_maxTraveltime = 3;
std::uniform_int_distribution<std::mt19937::result_type> travelDist(0, g_maxTraveltime);

struct Entity {
    bool isEnemy = true;
    int stacks = 0;
    int duration = 0;
};
typedef std::vector<Entity> E_vec;

struct Entities {
    E_vec* group;
    E_vec* enemies;
    size_t size;
    size_t g_size;
    size_t e_size;

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
    int targetIndex = 0;
    int hasArrived = 0;
    int stacks = 0;
};

struct SwarmStats {
    size_t simTime;
    std::array<std::array<int, 6>, 2> targetCasts{ 0 }; // vector of 2 vectors which holds cast target stats (
    int enemyTicks = 0;
    int friendlyTicks = 0;
};

struct SimResult {
    int groupSize = 0;
    int enemyCount = 0;
    bool hasCircle = false;
    char stratName[30] = "";
    std::array<int, 6> friendlyCasts{0};
    std::array<int, 6> enemyCasts{0};
    float ticksPerSecond = 0.0f;
    float enemyTicksPerSecond = 0.0f;
    float friendlyTicksPerSecond = 0.0f;
};

enum class InitialConfiguration {
    EMPTY, RANDOM, REALISTIC
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

std::vector<int> getRandomIndices(size_t interval, size_t count) {
    std::vector<int> v(interval);
    std::iota(std::begin(v), std::end(v), 0);
    for (int i = 0; i < count; i++) {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)interval - 1 - i);
        int j = dist(rng);
        v[i] = v[j];
        v[j] = i;
    }

    std::vector<int> ret(count);
    std::copy(v.begin(), v.begin() + count, ret.begin());
    return ret;
}

void InitializeEntities(E_vec& group, E_vec& enemies, Entities entities, InitialConfiguration config) {
    for (auto& friendly : group) {
        friendly.isEnemy = false;
    }
    switch (config) {
    case InitialConfiguration::EMPTY: return;
    case InitialConfiguration::RANDOM: {
        for (auto& e : group) {
            e.duration = g_maxDuration;
            e.stacks = dist13(rng);
        }
        for (auto& e : enemies) {
            e.duration = g_maxDuration;
            e.stacks = dist13(rng);
        }
    }return;
    case InitialConfiguration::REALISTIC: {
        std::uniform_int_distribution<std::mt19937::result_type> dist0es(0, (int)group.size() + (int)enemies.size() - 1);
        std::vector<int> targetIndices = getRandomIndices(group.size() + enemies.size(), 6);
        for (int i = 0; i < 4; i++) {
            int& index = targetIndices[i];
            entities[index].duration = g_maxDuration;
            entities[index].stacks = dist12(rng);
        }
        for (int i = 4; i < 6; i++) {
            int& index = targetIndices[i];
            entities[index].duration = g_maxDuration;
            entities[index].stacks = dist15(rng);
        }
    }return;
    }
}

void castSwarm(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const TargetStrategy strategy, SwarmStats& stats) {
    switch (strategy) {
    case TargetStrategy::FOCUSFRIENDLY: {
        travelingSwarms.push_back({ 0, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[0][entities[0].stacks]++;
    }return;
    case TargetStrategy::FOCUSENEMY: {
        travelingSwarms.push_back({ (int)entities.g_size, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[1][entities[entities.g_size].stacks]++;
    }return;
    case TargetStrategy::ONESTACK: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::ONESTACKENEMYFIRST: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::ONESTACKENEMY: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ (int)entities.g_size + index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats);
        return;
    };
    case TargetStrategy::ONESTACKFRIENDLY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats);
        return;
    };
    case TargetStrategy::TWOSTACKS: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::TWOSTACKSENEMYFIRST: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::TWOSTACKSENEMY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ (int)entities.g_size + index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats);
        return;
    };
    case TargetStrategy::TWOSTACKSFRIENDLY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats);
        return;
    };
    case TargetStrategy::THREESTACKS: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::THREESTACKSENEMYFIRST: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats);
        return;
    };
    case TargetStrategy::THREESTACKSENEMY: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ (int)entities.g_size + index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats);
        return;
    };
    case TargetStrategy::THREESTACKSFRIENDLY: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats);
        return;
    };
    case TargetStrategy::RANDOM: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.size - 1);
        int index = dist(rng);
        travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[index >= entities.g_size][entities[index].stacks]++;
    }return;
    case TargetStrategy::RANDOMENEMY: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.e_size - 1);
        int index = (int)entities.g_size + dist(rng);
        travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[index >= entities.g_size][entities[index].stacks]++;
    }return;
    case TargetStrategy::RANDOMFRIENDLY: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.g_size - 1);
        int index = dist(rng);
        travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[index >= entities.g_size][entities[index].stacks]++;
    }return;
    case TargetStrategy::LOWEST: {
        int lowestStacks = INT_MAX;
        int lowestIndex = 0;
        for (int index = 0; index < entities.size; index++) {
            int& stacks = entities[index].stacks;
            if (stacks < lowestStacks) {
                lowestStacks = stacks;
                lowestIndex = index;
            }
        }
        travelingSwarms.push_back({ lowestIndex, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[lowestIndex >= entities.g_size][lowestStacks]++;
    }return;
    case TargetStrategy::LOWESTENEMYFIRST: {
        int lowestStacks = INT_MAX;
        int lowestIndex = 0;
        for (int index = 0; index < entities.size; index++) {
            int& stacks = entities[index].stacks;
            if (stacks <= lowestStacks) {
                lowestStacks = stacks;
                lowestIndex = index;
            }
        }
        travelingSwarms.push_back({ lowestIndex, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[lowestIndex >= entities.g_size][lowestStacks]++;
    }return;
    case TargetStrategy::LOWESTENEMY: {
        int lowestStacks = INT_MAX;
        int lowestIndex = 0;
        for (int index = 0; index < entities.e_size; index++) {
            int& stacks = (*entities.enemies)[index].stacks;
            if (stacks < lowestStacks) {
                lowestStacks = stacks;
                lowestIndex = index;
            }
        }
        travelingSwarms.push_back({ (int)entities.g_size + lowestIndex, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[1][lowestStacks]++;
    }return;
    case TargetStrategy::LOWESTFRIENDLY: {
        int lowestStacks = INT_MAX;
        int lowestIndex = 0;
        for (int index = 0; index < entities.g_size; index++) {
            int& stacks = (*entities.group)[index].stacks;
            if (stacks < lowestStacks) {
                lowestStacks = stacks;
                lowestIndex = index;
            }
        }
        travelingSwarms.push_back({ lowestIndex, 1 - (int)travelDist(rng), 3 });
        stats.targetCasts[0][lowestStacks]++;
    }return;
    }
}

void propagateSwarm(int index, int stacks, bool wasFriendly, Entities& entities, std::vector<TravelingSwarm>& travelingSwarms) {
    if (wasFriendly) {
        int possibleTargetsCount = 0;
        for (int i = 0; i < entities.e_size; i++) {
            if ((*entities.enemies)[i].stacks == 0) {
                possibleTargetsCount++;
            }
        }
        if (possibleTargetsCount == 0) {
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.e_size - 1);
            int index = dist(rng);
            travelingSwarms.push_back({ (int)entities.g_size + index, 1 - (int)travelDist(rng), stacks - 1 });
        }
        else {
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, possibleTargetsCount - 1);
            int preIndex = dist(rng);
            int index = 0;
            for (int i = 0; i < entities.e_size; i++) {
                if ((*entities.enemies)[i].stacks == 0) {
                    index = i;
                    preIndex--;
                    if (preIndex < 0) {
                        break;
                    }
                }
            }
            travelingSwarms.push_back({ (int)entities.g_size + index, 1 - (int)travelDist(rng), stacks - 1 });
        }
    }
    else {
        int possibleTargetsCount = 0;
        for (int i = 0; i < entities.g_size; i++) {
            if ((*entities.group)[i].stacks == 0) {
                possibleTargetsCount++;
            }
        }
        if (possibleTargetsCount == 0) {
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.g_size - 1);
            int index = dist(rng);
            travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), stacks - 1 });
        }
        else {
            std::uniform_int_distribution<std::mt19937::result_type> dist(0, possibleTargetsCount - 1);
            int preIndex = dist(rng);
            int index = 0;
            for (int i = 0; i < entities.g_size; i++) {
                if ((*entities.group)[i].stacks == 0) {
                    index = i;
                    preIndex--;
                    if (preIndex < 0) {
                        break;
                    }
                }
            }
            travelingSwarms.push_back({ index, 1 - (int)travelDist(rng), stacks - 1 });
        }
    }
}

void advanceTime(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms) {
    std::vector<TravelingSwarm>::iterator it = travelingSwarms.begin();
    while(it != travelingSwarms.end()) {
        if (it->hasArrived <= 0) {
            it->hasArrived++;
            it++;
        }
        else {
            Entity& e = entities[it->targetIndex];
            e.duration = g_maxDuration;
            e.stacks = e.stacks + it->stacks > 5 ? 5 : e.stacks + it->stacks;
            it = travelingSwarms.erase(it);
        }
    }

    for (int index = 0; index < entities.g_size; index++) {
        Entity& e = (*entities.group)[index];
        if (e.duration > 0) {
            e.duration--;
            if (e.duration == 0) {
                if (e.stacks > 1) {
                    if (splitDist(rng) > g_splitChance) {
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms);
                    }
                    else {
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms);
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms);
                    }
                }
                e.stacks = 0;
            }
        }
    }

    for (int index = 0; index < entities.e_size; index++) {
        Entity& e = (*entities.enemies)[index];
        if (e.duration > 0) {
            e.duration--;
            if (e.duration == 0) {
                if (e.stacks > 1) {
                    if (splitDist(rng) > g_splitChance) {
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms);
                    }
                    else {
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms);
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms);
                    }
                }
                e.stacks = 0;
            }
        }
    }
}

void recordSwarmStats(Entities& entities, SwarmStats& stats) {
    for (int i = 0; i < entities.g_size; i++) {
        if ((*entities.group)[i].stacks > 0) {
            stats.friendlyTicks++;
        }
    }
    for (int i = 0; i < entities.e_size; i++) {
        if ((*entities.enemies)[i].stacks > 0) {
            stats.enemyTicks++;
        }
    }
}

void printStats(size_t friendlyCount, size_t enemyCount, SwarmStats stats, std::string_view name) {
    std::cout << "Group size:  " << friendlyCount << "\n";
    std::cout << "Enemy count: " << enemyCount << "\n";
    std::cout << name << ":\n";
    std::cout << "Enemy casts: ";
    for (int i = 0; i < 6; i++) {
        std::cout << stats.targetCasts[1][i] << ", ";
    }
    std::cout << "\n";
    std::cout << "Friendly casts: ";
    for (int i = 0; i < 6; i++) {
        std::cout << stats.targetCasts[0][i] << ", ";
    }
    std::cout << "\n";
    std::cout << "Ticks/s:          " << (stats.enemyTicks + stats.friendlyTicks) * 1.0f / stats.simTime << "\n";
    std::cout << "Enemy ticks/s:    " << stats.enemyTicks * 1.0f / stats.simTime << "\n";
    std::cout << "Friendly ticks/s: " << stats.friendlyTicks * 1.0f / stats.simTime << "\n\n";
}

template<size_t T>
void exportResults(std::array<SimResult, T>* results) {
    std::filesystem::path resPath = "C:\\Users\\Tobi\\Documents\\Programming\\Small_projects\\wow_swarm_simulator_visualization\\SwarmResults.csv";
    std::ofstream outFile{ resPath };
    outFile <<
        "group_size;enemyCount;hasCircle;stratName;"
        "friendlyCasts0;friendlyCasts1;friendlyCasts2;friendlyCasts3;friendlyCasts4;friendlyCasts5;"
        "enemyCasts0;enemyCasts1;enemyCasts2;enemyCasts3;enemyCasts4;enemyCasts5;"
        "ticksPerSecond;friendlyTicksPerTime;enemyTicksPerTime\n";
    for (SimResult& res : *results) {
        outFile << res.groupSize << ";" << res.enemyCount << ";";
        if (res.hasCircle) {
            outFile << "1;";
        }
        else {
            outFile << "0;";
        }
        outFile << res.stratName << ";";
        for (int i = 0; i < 6; i++) {
            outFile << res.friendlyCasts[i] << ";";
        }
        for (int i = 0; i < 6; i++) {
            outFile << res.enemyCasts[i] << ";";
        }
        outFile << res.ticksPerSecond << ";" << res.friendlyTicksPerSecond << ";" << res.enemyTicksPerSecond << "\n";
    }
    outFile.close();
}

int main()
{
    constexpr size_t simTime = 10000;
    constexpr std::array<TargetStrategy, 21> strategies{ 
        TargetStrategy::FOCUSFRIENDLY,
        TargetStrategy::FOCUSENEMY,
        TargetStrategy::LOWEST,
        TargetStrategy::LOWESTENEMYFIRST,
        TargetStrategy::LOWESTENEMY,
        TargetStrategy::LOWESTFRIENDLY,
        TargetStrategy::RANDOM,
        TargetStrategy::RANDOMENEMY, 
        TargetStrategy::RANDOMFRIENDLY,
        TargetStrategy::ONESTACK,
        TargetStrategy::ONESTACKENEMYFIRST,
        TargetStrategy::ONESTACKFRIENDLY,
        TargetStrategy::ONESTACKENEMY,
        TargetStrategy::TWOSTACKS,
        TargetStrategy::TWOSTACKSENEMYFIRST,
        TargetStrategy::TWOSTACKSFRIENDLY,
        TargetStrategy::TWOSTACKSENEMY,
        TargetStrategy::THREESTACKS,
        TargetStrategy::THREESTACKSENEMYFIRST,
        TargetStrategy::THREESTACKSFRIENDLY,
        TargetStrategy::THREESTACKSENEMY,
    };

    constexpr std::array<std::string_view, 21> stratNames{
        "FOCUSFRIENDLY",
        "FOCUSENEMY",
        "LOWEST",
        "LOWESTENEMYFIRST",
        "LOWESTENEMY",
        "LOWESTFRIENDLY",
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
        "THREESTACKSENEMY"
    };

    constexpr std::array<size_t, 3> friendlyCounts{1, 5, 20};
    constexpr std::array<size_t, 10> enemyCounts{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    std::array<SimResult, 2 * friendlyCounts.size() * enemyCounts.size() * stratNames.size()>* results = new std::array<SimResult, 2 * friendlyCounts.size()* enemyCounts.size()* stratNames.size()>();
    size_t resCounter = 0;

    for (auto& hasCircle : { false, true }) {
        if (hasCircle) {
            g_maxDuration = 9;
        }
        else {
            g_maxDuration = 12;
        }
        for (auto& friendlyCount : friendlyCounts) {
            for (auto& enemyCount : enemyCounts) {
                for (size_t i = 0; i < strategies.size(); i++) {
                    const auto& strategy = strategies[i];
                    const auto& name = stratNames[i];

                    E_vec group{ friendlyCount };
                    E_vec enemies{ enemyCount };
                    Entities entities{ group, enemies };
                    std::vector<TravelingSwarm> travelingSwarms;
                    SwarmStats stats;
                    stats.simTime = simTime;

                    InitializeEntities(group, enemies, entities, InitialConfiguration::EMPTY);

                    int cooldown = g_cooldown;
                    for (size_t time = 0; time < simTime; time++) {
                        cooldown--;
                        if (cooldown == 0) {
                            cooldown = g_cooldown;
                            castSwarm(entities, travelingSwarms, strategy, stats);
                        }

                        advanceTime(entities, travelingSwarms);

                        recordSwarmStats(entities, stats);
                    }
                    //printStats(...)
                    SimResult& r = (*results)[resCounter];
                    r.groupSize = (int)friendlyCount;
                    r.enemyCount = (int)enemyCount;
                    r.hasCircle = hasCircle;
                    strcpy_s(r.stratName, name.data());
                    r.ticksPerSecond = (stats.enemyTicks + stats.friendlyTicks) * 1.0f / stats.simTime;
                    r.enemyTicksPerSecond = stats.enemyTicks * 1.0f / stats.simTime;
                    r.friendlyTicksPerSecond = stats.friendlyTicks * 1.0f / stats.simTime;
                    r.friendlyCasts = stats.targetCasts[0];
                    r.enemyCasts = stats.targetCasts[1];

                    resCounter++;
                }
            }
        }
    }

    exportResults(results);
    delete results;
}