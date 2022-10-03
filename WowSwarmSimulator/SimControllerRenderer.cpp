#include "SimControllerRenderer.h"

#include <thread>

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_stdlib.h"

void renderControllerWindow(SimParameters& parameters, SimData& simData) {
	auto windowFlags = ImGuiWindowFlags_None;
	if (!ImGui::Begin("Sim Controller", NULL, windowFlags)) {
		ImGui::End();
		return;
	}

	ImGui::SliderFloat("Split chance", &parameters.g_splitChance, 0.0f, 1.0f);
	ImGui::SliderFloat("Max duration (w/o Circle)", &parameters.g_maxDuration, 1.0f, 30.0f);
	ImGui::SliderFloat("Cooldown", &parameters.g_cooldown, 1.0f, 60.0f);
	ImGui::SliderFloat("Max travel time", &parameters.g_maxTraveltime, 0.0f, 10.0f);
	ImGui::SliderFloat("Simulation duration", &parameters.simTime, 1.0f, 100000.0f);
	ImGui::SliderFloat("Time step", &parameters.timeDelta, 0.001f, 1.0f);
	ImGui::SliderFloat("Simulation speed", &parameters.simulationSpeed, 0.0f, 5.0f);

	const char* initConfigComboPreviewValue = initConfigNames[(int)parameters.initConfig].data();
	if (ImGui::BeginCombo("Initial configuration", initConfigComboPreviewValue))
	{
		for (int n = 0; n < initConfigNames.size(); n++)
		{
			const bool is_selected = (initConfigComboPreviewValue == initConfigNames[n]);
			if (ImGui::Selectable(initConfigNames[n].data(), is_selected)) {
				parameters.initConfig = static_cast<InitialConfiguration>(n);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const char* strategyComboPreviewValue = stratNames[(int)parameters.strategy].data();
	if (ImGui::BeginCombo("Swarm cast strategy", strategyComboPreviewValue))
	{
		for (int n = 0; n < stratNames.size(); n++)
		{
			const bool is_selected = (strategyComboPreviewValue == stratNames[n]);
			if (ImGui::Selectable(stratNames[n].data(), is_selected)) {
				parameters.strategy = static_cast<TargetStrategy>(n);
			}

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::SliderInt("Friendly count", &parameters.friendCount, 1, 20);
	ImGui::SliderInt("Enemy count", &parameters.enemyCount, 1, 20);
	ImGui::Checkbox("Has Circle", &parameters.hasCircle);

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	if (ImGui::Button("Reset parameters")) {
		parameters = SimParameters();
	}

	if (ImGui::Button("Start individual simulation")) {

		simData.group = E_vec{ (size_t)parameters.friendCount };
		simData.enemies = E_vec{ (size_t)parameters.enemyCount };
		simData.entities = Entities{ simData.group, simData.enemies };
		simData.travelingSwarms.clear();
		simData.stats = SwarmStats();
		simData.stats.simTime = parameters.simTime;

		std::thread t(RunRealtimeSim,
			std::ref(parameters),
			std::ref(simData));
		t.detach();
	}
	ImGui::Checkbox("Pause sim", &parameters.pauseSim);
	if (ImGui::Button("Stop sim")) {
		simData.isRunning = false;
	}

	if (ImGui::Button("Run all combinations")) {
		RunSim(parameters);
	}

	ImGui::End();
}