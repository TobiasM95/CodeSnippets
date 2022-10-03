
#include "WowSwarmSimulator.h"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>

// Choose a random mean between 1 and 6
std::random_device dev;
std::mt19937 rng(dev());
std::uniform_int_distribution<std::mt19937::result_type> dist13(1, 3);
std::uniform_int_distribution<std::mt19937::result_type> dist15(1, 5);
std::uniform_int_distribution<std::mt19937::result_type> dist12(1, 2);
std::uniform_real_distribution<> splitDist(0.0, 1.0);

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

void InitializeEntities(E_vec& group, E_vec& enemies, Entities& entities, InitialConfiguration config, const SimParameters& parameters) {
    for (auto& friendly : group) {
        friendly.isEnemy = false;
    }
    switch (config) {
    case InitialConfiguration::EMPTY: return;
    case InitialConfiguration::RANDOM: {
        for (auto& e : group) {
            e.duration = parameters.g_maxDuration;
            e.stacks = dist13(rng);
        }
        for (auto& e : enemies) {
            e.duration = parameters.g_maxDuration;
            e.stacks = dist13(rng);
        }
    }return;
    case InitialConfiguration::REALISTIC: {
        std::uniform_int_distribution<std::mt19937::result_type> dist0es(0, (int)group.size() + (int)enemies.size() - 1);
        std::vector<int> targetIndices = getRandomIndices(group.size() + enemies.size(), 6);
        for (int i = 0; i < 4; i++) {
            int& index = targetIndices[i];
            entities[index].duration = parameters.g_maxDuration;
            entities[index].stacks = dist12(rng);
        }
        for (int i = 4; i < 6; i++) {
            int& index = targetIndices[i];
            entities[index].duration = parameters.g_maxDuration;
            entities[index].stacks = dist15(rng);
        }
    }return;
    }
}

void castSwarm(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const TargetStrategy strategy, SwarmStats& stats, const SimParameters& parameters) {
    std::uniform_real_distribution<float> travelDist(0.0, parameters.g_maxTraveltime);
    float travelTime = travelDist(rng);

    switch (strategy) {
    case TargetStrategy::FOCUSFRIENDLY: {
        travelingSwarms.push_back({ 0, 0, -travelTime, travelTime, 3 });
        stats.targetCasts[0][entities[0].stacks]++;
    }return;
    case TargetStrategy::FOCUSENEMY: {
        travelingSwarms.push_back({ 0, (int)entities.g_size, -travelTime, travelTime, 3 });
        stats.targetCasts[1][entities[entities.g_size].stacks]++;
    }return;
    case TargetStrategy::ONESTACK: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::ONESTACKENEMYFIRST: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::ONESTACKENEMY: {
        for (int targetStacks = 1; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, (int)entities.g_size + index, -travelTime, travelTime, 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats, parameters);
        return;
    };
    case TargetStrategy::ONESTACKFRIENDLY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats, parameters);
        return;
    };
    case TargetStrategy::TWOSTACKS: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::TWOSTACKSENEMYFIRST: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::TWOSTACKSENEMY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, (int)entities.g_size + index, -travelTime, travelTime, 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats, parameters);
        return;
    };
    case TargetStrategy::TWOSTACKSFRIENDLY: {
        for (int targetStacks = 2; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats, parameters);
        return;
    };
    case TargetStrategy::THREESTACKS: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.size; index++) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::THREESTACKSENEMYFIRST: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = (int)entities.size - 1; index >= 0; index--) {
                int& stacks = entities[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[index >= entities.g_size][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOM, stats, parameters);
        return;
    };
    case TargetStrategy::THREESTACKSENEMY: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.e_size; index++) {
                int& stacks = (*entities.enemies)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, (int)entities.g_size + index, -travelTime, travelTime, 3 });
                    stats.targetCasts[1][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMENEMY, stats, parameters);
        return;
    };
    case TargetStrategy::THREESTACKSFRIENDLY: {
        for (int targetStacks = 3; targetStacks >= 0; targetStacks--) {
            for (int index = 0; index < entities.g_size; index++) {
                int& stacks = (*entities.group)[index].stacks;
                if (stacks == targetStacks) {
                    travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
                    stats.targetCasts[0][stacks]++;
                    return;
                }
            }
        }
        castSwarm(entities, travelingSwarms, TargetStrategy::RANDOMFRIENDLY, stats, parameters);
        return;
    };
    case TargetStrategy::RANDOM: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.size - 1);
        int index = dist(rng);
        travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
        stats.targetCasts[index >= entities.g_size][entities[index].stacks]++;
    }return;
    case TargetStrategy::RANDOMENEMY: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.e_size - 1);
        int index = (int)entities.g_size + dist(rng);
        travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
        stats.targetCasts[index >= entities.g_size][entities[index].stacks]++;
    }return;
    case TargetStrategy::RANDOMFRIENDLY: {
        std::uniform_int_distribution<std::mt19937::result_type> dist(0, (int)entities.g_size - 1);
        int index = dist(rng);
        travelingSwarms.push_back({ 0, index, -travelTime, travelTime, 3 });
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
        travelingSwarms.push_back({ 0, lowestIndex, -travelTime, travelTime, 3 });
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
        travelingSwarms.push_back({ 0, lowestIndex, -travelTime, travelTime, 3 });
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
        travelingSwarms.push_back({ 0, (int)entities.g_size + lowestIndex, -travelTime, travelTime, 3 });
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
        travelingSwarms.push_back({ 0, lowestIndex, -travelTime, travelTime, 3 });
        stats.targetCasts[0][lowestStacks]++;
    }return;
    }
}

void propagateSwarm(int sourceIndex, int stacks, bool wasFriendly, Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const SimParameters& parameters) {
    std::uniform_real_distribution<float> travelDist(0.0, parameters.g_maxTraveltime);
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
            float travelTime = travelDist(rng);
            travelingSwarms.push_back({ sourceIndex, (int)entities.g_size + index, -travelTime, travelTime, stacks - 1 });
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
            float travelTime = travelDist(rng);
            travelingSwarms.push_back({ sourceIndex, (int)entities.g_size + index, -travelTime, travelTime, stacks - 1 });
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
            float travelTime = travelDist(rng);
            travelingSwarms.push_back({ sourceIndex, index, -travelTime, travelTime, stacks - 1 });
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
            float travelTime = travelDist(rng);
            travelingSwarms.push_back({ sourceIndex, index, -travelTime, travelTime, stacks - 1 });
        }
    }
}

void advanceTime(Entities& entities, std::vector<TravelingSwarm>& travelingSwarms, const SimParameters& parameters) {
    std::vector<TravelingSwarm>::iterator it = travelingSwarms.begin();
    while(it != travelingSwarms.end()) {
        if (it->timeToArrival < 0) {
            it->timeToArrival += parameters.timeDelta;
            it++;
        }
        else {
            Entity& e = entities[it->targetIndex];
            e.duration = parameters.g_maxDuration;
            e.stacks = e.stacks + it->stacks > 5 ? 5 : e.stacks + it->stacks;
            it = travelingSwarms.erase(it);
        }
    }

    for (int index = 0; index < entities.g_size; index++) {
        Entity& e = (*entities.group)[index];
        if (e.duration > 0) {
            e.duration -= parameters.timeDelta;
            if (e.duration <= 0) {
                if (e.stacks > 1) {
                    if (splitDist(rng) > parameters.g_splitChance) {
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms, parameters);
                    }
                    else {
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms, parameters);
                        propagateSwarm(index, e.stacks, true, entities, travelingSwarms, parameters);
                    }
                }
                e.stacks = 0;
            }
        }
    }

    for (int index = 0; index < entities.e_size; index++) {
        Entity& e = (*entities.enemies)[index];
        if (e.duration > 0) {
            e.duration -= parameters.timeDelta;
            if (e.duration <= 0) {
                if (e.stacks > 1) {
                    if (splitDist(rng) > parameters.g_splitChance) {
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms, parameters);
                    }
                    else {
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms, parameters);
                        propagateSwarm((int)entities.g_size + index, e.stacks, false, entities, travelingSwarms, parameters);
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
    if (!std::filesystem::is_directory(resPath.parent_path())) {
        return;
    }
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

void RunRealtimeSim(SimParameters& parameters, SimData& simData) {
    simData.isRunning = true;

    InitializeEntities(simData.group, simData.enemies, simData.entities, parameters.initConfig, parameters);

    double cooldown = 0.0;
    std::unique_lock<std::mutex> lck(simData.m);
    lck.unlock();
    for (double time = 0; time < parameters.simTime;) {
        auto targetTime = std::chrono::steady_clock::now() + std::chrono::microseconds(static_cast<long>(parameters.simulationSpeed * parameters.timeDelta * 1000000));

        if (parameters.pauseSim) {
            continue;
        }
        lck.lock();
        cooldown -= parameters.timeDelta;
        if (cooldown <= 0) {
            cooldown += parameters.g_cooldown;
            castSwarm(simData.entities, simData.travelingSwarms, parameters.strategy, simData.stats, parameters);
        }

        advanceTime(simData.entities, simData.travelingSwarms, parameters);

        recordSwarmStats(simData.entities, simData.stats);

        time += parameters.timeDelta;
        lck.unlock();
        if (!simData.isRunning) {
            break;
        }

        if (parameters.simulationSpeed > 0.0f) {
            //std::this_thread::sleep_until(targetTime);
            while (std::chrono::steady_clock::now() < targetTime) {
                std::this_thread::yield();
            }
        }
    }

    simData.isRunning = false;
}

void RunSim(const SimParameters& parameters)
{
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

    constexpr std::array<size_t, 3> friendlyCounts{1, 5, 20};
    constexpr std::array<size_t, 10> enemyCounts{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    std::array<SimResult, 2 * friendlyCounts.size() * enemyCounts.size() * stratNames.size()>* results = new std::array<SimResult, 2 * friendlyCounts.size()* enemyCounts.size()* stratNames.size()>();
    size_t resCounter = 0;

    for (auto& hasCircle : { false, true }) {
        double resMaxDuration = hasCircle ? parameters.g_maxDuration * 0.75 : parameters.g_maxDuration;
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
                    stats.simTime = parameters.simTime;

                    InitializeEntities(group, enemies, entities, InitialConfiguration::EMPTY, parameters);

                    double cooldown = parameters.g_cooldown;
                    for (double time = 0; time < parameters.simTime; time += parameters.timeDelta) {
                        cooldown -= parameters.timeDelta;
                        if (cooldown <= 0) {
                            cooldown += parameters.g_cooldown;
                            castSwarm(entities, travelingSwarms, strategy, stats, parameters);
                        }

                        advanceTime(entities, travelingSwarms, parameters);

                        recordSwarmStats(entities, stats);
                    }
                    //printStats(...)
                    SimResult& r = (*results)[resCounter];
                    r.groupSize = (int)friendlyCount;
                    r.enemyCount = (int)enemyCount;
                    r.hasCircle = hasCircle;
                    strcpy_s(r.stratName, name.data());
                    r.ticksPerSecond = (stats.enemyTicks + stats.friendlyTicks) * parameters.timeDelta / stats.simTime;
                    r.enemyTicksPerSecond = stats.enemyTicks * parameters.timeDelta / stats.simTime;
                    r.friendlyTicksPerSecond = stats.friendlyTicks * parameters.timeDelta / stats.simTime;
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