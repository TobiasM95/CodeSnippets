#include "SimRenderer.h"

#include "imgui_internal.h"
#include "imgui_stdlib.h"


void renderRealtimeSim(const SimParameters& parameters, const SimData& simData) {
	ImGui::PushFont(ImGui::GetCurrentContext()->IO.Fonts->Fonts[1]);
	constexpr float buttonMaxSize = 100.0f;
	constexpr float travelButtonSize = 50.0f;
	float buttonPadding = ImGui::GetStyle().ItemSpacing.x;
	
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImVec2 windowSize = ImVec2{ viewport->Size.x * 0.7f, viewport->Size.y };
	ImGui::SetNextWindowSize(windowSize);
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove;
	if (!ImGui::Begin("RealtimeSimWindow", NULL, windowFlags)) {
		ImGui::End();
		return;
	}
	ImVec2 renderArea = ImGui::GetContentRegionAvail();

	float enemyButtonSize = (windowSize.x - (simData.enemies.size() + 1) * buttonPadding) / (simData.enemies.size());
	enemyButtonSize = enemyButtonSize > buttonMaxSize ? buttonMaxSize : enemyButtonSize;
	ImGui::SetCursorPos(ImVec2{ buttonPadding, buttonPadding });
	for (int i = 0; i < simData.enemies.size(); i++) {
		const int& stacks = simData.enemies[i].stacks;
		ImGui::PushStyleColor(ImGuiCol_Button, getStackColor(stacks));
		char buffer[20];  // maximum expected length of the float
		std::snprintf(buffer, 20, "%d\n%.1f", stacks, simData.enemies[i].duration);
		ImGui::Button(buffer, ImVec2{enemyButtonSize, enemyButtonSize});
		ImGui::PopStyleColor();
		ImGui::SameLine();
	}

	float friendlyButtonSize = (windowSize.x - (simData.group.size() + 1) * buttonPadding) / (simData.group.size());
	friendlyButtonSize = friendlyButtonSize > buttonMaxSize ? buttonMaxSize : friendlyButtonSize;
	for (int i = 0; i < simData.group.size(); i++) {
		ImGui::SetCursorPos(ImVec2{ buttonPadding + i * (friendlyButtonSize + buttonPadding), windowSize.y - friendlyButtonSize - buttonPadding});
		const int& stacks = simData.group[i].stacks;
		ImGui::PushStyleColor(ImGuiCol_Button, getStackColor(stacks));
		char buffer[20];  // maximum expected length of the float
		std::snprintf(buffer, 20, "%d\n%.1f", stacks, simData.group[i].duration);
		ImGui::Button(buffer, ImVec2{ friendlyButtonSize, friendlyButtonSize });
		ImGui::PopStyleColor();
	}

	for (int i = 0; i < simData.travelingSwarms.size(); i++) {
		const TravelingSwarm& swarm = simData.travelingSwarms[i];
		float progress = 1.0f + swarm.timeToArrival / swarm.travelTime;
		
		bool sourceIsFriend = swarm.sourceIndex < simData.group.size();
		bool targetIsFriend = swarm.targetIndex < simData.group.size();
		int startIndex = swarm.sourceIndex >= simData.group.size() ? swarm.sourceIndex - simData.group.size() : swarm.sourceIndex;
		int endIndex = swarm.targetIndex >= simData.group.size() ? swarm.targetIndex - simData.group.size() : swarm.targetIndex;
		float startX = sourceIsFriend ? buttonPadding + startIndex * (friendlyButtonSize + buttonPadding) : buttonPadding + startIndex * (enemyButtonSize + buttonPadding);
		float endX = targetIsFriend ? buttonPadding + endIndex * (friendlyButtonSize + buttonPadding) : buttonPadding + endIndex * (enemyButtonSize + buttonPadding);
		float currentX = progress * endX + (1.0f - progress) * startX;

		float startY = sourceIsFriend ? windowSize.y - 2.0f * buttonPadding - friendlyButtonSize - travelButtonSize : 2.0f * buttonPadding + enemyButtonSize;
		float endY = targetIsFriend ? windowSize.y - 2.0f * buttonPadding - friendlyButtonSize - travelButtonSize : 2.0f * buttonPadding + enemyButtonSize;
		float currentY = progress < 0.5f ? progress * windowSize.y + (1.0f - 2.0f * progress) * startY : 2.0f * (progress - 0.5f) * endY + (1.0f - 2.0f * (progress - 0.5f)) * 0.5f * windowSize.y;

		ImGui::SetCursorPos(ImVec2{ currentX, currentY});

		const int& stacks = swarm.stacks;
		ImGui::PushStyleColor(ImGuiCol_Button, getStackColor(stacks));
		ImGui::Button(std::to_string(stacks).c_str(), ImVec2{ travelButtonSize, travelButtonSize });
		ImGui::PopStyleColor();
	}

	ImGui::End();
	ImGui::PopFont();
}

ImVec4 getStackColor(int stacks) {
	switch (stacks) {
	case 0: return ImVec4{ 0.9f, 0.3f, 0.3f, 1.0f };
	case 1: return ImVec4{ 0.9f, 0.5f, 0.3f, 1.0f };
	case 2: return ImVec4{ 0.9f, 0.7f, 0.3f, 1.0f };
	case 3: return ImVec4{ 0.7f, 0.9f, 0.3f, 1.0f };
	case 4: return ImVec4{ 0.5f, 0.9f, 0.3f, 1.0f };
	case 5: return ImVec4{ 0.3f, 0.9f, 0.3f, 1.0f };
	default: return ImVec4{ 1.0f, 0.0f, 1.0f, 1.0f };
	}
}