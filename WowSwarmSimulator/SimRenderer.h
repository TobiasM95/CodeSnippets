#pragma once

#include "imgui.h"

#include "WowSwarmSimulator.h"

void renderRealtimeSim(const SimParameters& parameters, const SimData& simData);
ImVec4 getStackColor(int stacks);