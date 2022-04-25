#pragma once

#include <vector>


std::vector<std::vector<int>> generate_tier_loot_stats_no_minimum(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
std::vector<std::vector<int>> generate_tier_loot_stats_vector_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
std::vector<int> generate_tier_loot_stats_vector_only_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
std::vector<std::vector<int>> generate_tier_loot_stats_vector_no_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
std::vector<std::array<int, 2>> generate_tier_loot_stats_vector_array_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
std::vector<std::array<int, 2>> generate_tier_loot_stats_vector_array_no_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
int** generate_tier_loot_stats_2d_c_array(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
int* generate_tier_loot_stats_1d_c_array(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player);
void print_statistics(std::vector<std::vector<int>> tier_loot_stats, int num_simulations);
void print_statistics(std::vector<std::array<int, 2>> tier_loot_stats, int num_simulations);
void print_statistics(std::vector<int> tier_loot_stats, int num_simulations);
void print_statistics(int** tier_loot_stats, int num_simulations);
void print_statistics(int* tier_loot_stats, int num_simulations);
