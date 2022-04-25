// WowTierLootVariance.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "WowTierLootVariance.h"

#include <random>
#include <iostream>
#include <cassert>
#include <array>

#include <chrono>

int main()
{
    const int num_runs = 50;

    const int num_simulations = 200000;
    const int num_players = 21;
    const int num_players_need = 20;
    const double tier_chance_per_player = 0.1;
    assert(num_players >= num_players_need);

    auto t1 = std::chrono::high_resolution_clock::now();
    //std::vector<std::vector<int>> tier_loot_stats;
    //std::vector<std::array<int, 2>> tier_loot_stats;
    std::vector<int> tier_loot_stats;
    //int** tier_loot_stats;
    //int* tier_loot_stats;
    for (int i = 0; i < num_runs; i++)
    {
        //tier_loot_stats = generate_tier_loot_stats_vector_resize(num_simulations, num_players, num_players_need, tier_chance_per_player);
        
        //tier_loot_stats = generate_tier_loot_stats_vector_no_resize(num_simulations, num_players, num_players_need, tier_chance_per_player);

        //tier_loot_stats = generate_tier_loot_stats_vector_array_resize(num_simulations, num_players, num_players_need, tier_chance_per_player);

        //tier_loot_stats = generate_tier_loot_stats_vector_array_no_resize(num_simulations, num_players, num_players_need, tier_chance_per_player);
        
        tier_loot_stats = generate_tier_loot_stats_vector_only_resize(num_simulations, num_players, num_players_need, tier_chance_per_player);

        /*tier_loot_stats = generate_tier_loot_stats_2d_c_array(num_simulations, num_players, num_players_need, tier_chance_per_player);
        if(i + 1 < num_runs){
            for (int i = 0; i < num_simulations; ++i) {
                delete[] tier_loot_stats[i];
            }
            delete[] tier_loot_stats;
        }*/

        /*tier_loot_stats = generate_tier_loot_stats_1d_c_array(num_simulations, num_players, num_players_need, tier_chance_per_player);
        if(i + 1 < num_runs){
            delete[] tier_loot_stats;
        }*/
        
    }
    auto t2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> ms_double = t2 - t1;

    std::cout << "Operation time: " << ms_double.count() << std::endl;
    print_statistics(tier_loot_stats, num_simulations);

}

std::vector<std::vector<int>> generate_tier_loot_stats_no_minimum(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    std::vector<std::vector<int>> tier_loot_stats;
    tier_loot_stats.resize(num_simulations, std::vector<int>(2, 0));
    for (int i = 0; i < num_simulations; i++) {
        tier_loot_stats[i][0] = 0;
        tier_loot_stats[i][1] = 0;
        for (int p = 0; p < num_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                //tier piece dropped for player p
                if (p >= num_players_need)
                    tier_loot_stats[i][1] += 1;
                else
                    tier_loot_stats[i][0] += 1;
            }
        }
    }

    return tier_loot_stats;
}

std::vector<std::vector<int>> generate_tier_loot_stats_vector_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    std::vector<std::vector<int>> tier_loot_stats;
    tier_loot_stats.resize(num_simulations, std::vector<int>(2, 0));
    //tier_loot_stats.reserve(num_simulations * sizeof(std::vector<int>(2,0)));
    for (int i = 0; i < num_simulations; i++) {
        tier_loot_stats[i][0] = 0;
        tier_loot_stats[i][1] = 0;
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                tier_loot_stats[i][0] += 1;
            }
            else {
                tier_loot_stats[i][1] += 1;
            }
        }
    }

    return tier_loot_stats;
}

std::vector<int> generate_tier_loot_stats_vector_only_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    std::vector<int> tier_loot_stats;
    tier_loot_stats.resize(2 * num_simulations, 0);
    //tier_loot_stats.reserve(num_simulations * sizeof(std::vector<int>(2,0)));
    for (int i = 0; i < num_simulations; i++) {
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                tier_loot_stats[2*i] += 1;
            }
            else {
                tier_loot_stats[2*i+1] += 1;
            }
        }
    }

    return tier_loot_stats;
}

std::vector<std::array<int, 2>> generate_tier_loot_stats_vector_array_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    std::vector<std::array<int, 2>> tier_loot_stats;
    tier_loot_stats.resize(num_simulations, std::array<int, 2>{0, 0});
    //tier_loot_stats.reserve(num_simulations * sizeof(std::vector<int>(2,0)));
    for (int i = 0; i < num_simulations; i++) {
        tier_loot_stats[i][0] = 0;
        tier_loot_stats[i][1] = 0;
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                tier_loot_stats[i][0] += 1;
            }
            else {
                tier_loot_stats[i][1] += 1;
            }
        }
    }

    return tier_loot_stats;
}

std::vector<std::vector<int>> generate_tier_loot_stats_vector_no_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    std::vector<std::vector<int>> tier_loot_stats;
    for (int i = 0; i < num_simulations; i++) {
        std::vector<int> cur_run = { 0,0 };
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                cur_run[0] += 1;
            }
            else {
                cur_run[1] += 1;
            }
        }
        tier_loot_stats.push_back(cur_run);
    }

    return tier_loot_stats;
}

std::vector<std::array<int, 2>> generate_tier_loot_stats_vector_array_no_resize(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    std::vector<std::array<int, 2>> tier_loot_stats;
    for (int i = 0; i < num_simulations; i++) {
        std::array<int, 2> cur_run = { 0,0 };
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                cur_run[0] += 1;
            }
            else {
                cur_run[1] += 1;
            }
        }
        tier_loot_stats.push_back(cur_run);
    }

    return tier_loot_stats;
}

int** generate_tier_loot_stats_2d_c_array(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    int** tier_loot_stats = new int* [num_simulations];
    for (int i = 0; i < num_simulations; i++) {
        tier_loot_stats[i] = new int[2];
        tier_loot_stats[i][0] = 0;
        tier_loot_stats[i][1] = 0;
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                tier_loot_stats[i][0] += 1;
            }
            else {
                tier_loot_stats[i][1] += 1;
            }
        }
    }

    return tier_loot_stats;
}

int* generate_tier_loot_stats_1d_c_array(const int num_simulations, const int num_players, const int num_players_need, const double tier_chance_per_player) {
    std::random_device rd;
    std::mt19937 mt(rd());
    std::uniform_real_distribution<double> dist(0.0, std::nextafter(1.0, DBL_MAX));

    //first do the loot simulation and collect stats in 2d array
    //row is simulation index
    //column is tier pieces that were needed, tier pieces that were "wasted"
    int* tier_loot_stats = new int[2*num_simulations];
    for (int i = 0; i < num_simulations; i++) {
        tier_loot_stats[2*i] = 0;
        tier_loot_stats[2*i+1] = 0;
        //guaranteed tier pieces
        int guaranteed_tier_pieces = num_players / 10;
        int leftover_players = num_players - guaranteed_tier_pieces * 10;
        int leftover_tier_pieces = 0;
        for (int p = 0; p < leftover_players; p++) {
            if (dist(mt) <= tier_chance_per_player) {
                leftover_tier_pieces += 1;
            }
        }
        int dropped_tier_pieces = guaranteed_tier_pieces + leftover_tier_pieces;
        for (int t = 0; t < dropped_tier_pieces; t++) {
            if (dist(mt) <= num_players_need * 1.0 / num_players) {
                tier_loot_stats[2*i] += 1;
            }
            else {
                tier_loot_stats[2*i+1] += 1;
            }
        }
    }

    return tier_loot_stats;
}

void print_statistics(std::vector<std::vector<int>> tier_loot_stats, int num_simulations) {
    //do some statistics
    double tier_piece_useful = 0;
    double tier_piece_wasted = 0;
    double tier_piece_useful_squared = 0;
    double tier_piece_wasted_squared = 0;
    for (int i = 0; i < num_simulations; i++) {
        tier_piece_useful += tier_loot_stats[i][0];
        tier_piece_wasted += tier_loot_stats[i][1];
        tier_piece_useful_squared += tier_loot_stats[i][0] * tier_loot_stats[i][0];
        tier_piece_wasted_squared += tier_loot_stats[i][1] * tier_loot_stats[i][1];
    }

    tier_piece_useful /= (double)num_simulations;
    tier_piece_wasted /= (double)num_simulations;
    tier_piece_useful_squared /= (double)num_simulations;
    tier_piece_wasted_squared /= (double)num_simulations;

    double variance_tier_piece_useful = tier_piece_useful_squared - tier_piece_useful * tier_piece_useful;
    double variance_tier_piece_wasted = tier_piece_wasted_squared - tier_piece_wasted * tier_piece_wasted;

    std::cout << tier_piece_useful << "  " << tier_piece_wasted << std::endl;
    std::cout << std::sqrt(variance_tier_piece_useful) << "  " << std::sqrt(variance_tier_piece_wasted) << std::endl;
}

void print_statistics(std::vector<std::array<int, 2>> tier_loot_stats, int num_simulations) {
    //do some statistics
    double tier_piece_useful = 0;
    double tier_piece_wasted = 0;
    double tier_piece_useful_squared = 0;
    double tier_piece_wasted_squared = 0;
    for (int i = 0; i < num_simulations; i++) {
        tier_piece_useful += tier_loot_stats[i][0];
        tier_piece_wasted += tier_loot_stats[i][1];
        tier_piece_useful_squared += tier_loot_stats[i][0] * tier_loot_stats[i][0];
        tier_piece_wasted_squared += tier_loot_stats[i][1] * tier_loot_stats[i][1];
    }

    tier_piece_useful /= (double)num_simulations;
    tier_piece_wasted /= (double)num_simulations;
    tier_piece_useful_squared /= (double)num_simulations;
    tier_piece_wasted_squared /= (double)num_simulations;

    double variance_tier_piece_useful = tier_piece_useful_squared - tier_piece_useful * tier_piece_useful;
    double variance_tier_piece_wasted = tier_piece_wasted_squared - tier_piece_wasted * tier_piece_wasted;

    std::cout << tier_piece_useful << "  " << tier_piece_wasted << std::endl;
    std::cout << std::sqrt(variance_tier_piece_useful) << "  " << std::sqrt(variance_tier_piece_wasted) << std::endl;
}

void print_statistics(std::vector<int> tier_loot_stats, int num_simulations) {
    std::vector<std::vector<int>> vec;
    vec.resize(num_simulations, std::vector<int>(2, 0));
    for (int i = 0; i < num_simulations; i++) {
        vec[i][0] = tier_loot_stats[2 * i];
        vec[i][1] = tier_loot_stats[2 * i + 1];
    }
    print_statistics(vec, num_simulations);
}

void print_statistics(int** tier_loot_stats, int num_simulations) {
    std::vector<std::vector<int>> vec;
    vec.resize(num_simulations, std::vector<int>(2, 0));
    for (int i = 0; i < num_simulations; i++) {
        vec[i][0] = tier_loot_stats[i][0];
        vec[i][1] = tier_loot_stats[i][1];
    }
    print_statistics(vec, num_simulations);
}

void print_statistics(int* tier_loot_stats, int num_simulations) {
    std::vector<std::vector<int>> vec;
    vec.resize(num_simulations, std::vector<int>(2, 0));
    for (int i = 0; i < num_simulations; i++) {
        vec[i][0] = tier_loot_stats[2*i];
        vec[i][1] = tier_loot_stats[2*i+1];
    }
    print_statistics(vec, num_simulations);
}

