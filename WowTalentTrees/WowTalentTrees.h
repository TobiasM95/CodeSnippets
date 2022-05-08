#pragma once

#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <memory>

namespace WowTalentTrees {
    enum class TalentType;
    struct TalentTree;
    struct Talent;
    struct TreeDAGInfo;
    void addChild(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child);
    void addParent(std::shared_ptr<Talent> child, std::shared_ptr<Talent> parent);
    void pairTalents(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child);
    std::string getTalentString(TalentTree tree);
    void printTree(TalentTree tree);
    void addTalentAndChildrenToMap(std::shared_ptr<Talent> talent, std::unordered_map<std::string, int>& treeRepresentation);
    std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput);
    std::shared_ptr<Talent> createTalent(std::string name, int maxPoints);
    TalentTree parseTree(std::string treeRep);
    std::vector<std::string> splitString(std::string s, std::string delimiter);
    void visualizeTree(TalentTree root, std::string suffix);
    void visualizeTalentConnections(std::shared_ptr<Talent> root, std::stringstream& connections);
    std::string visualizeTalentInformation(TalentTree tree);
    void getTalentInfos(std::shared_ptr<Talent> talent, std::unordered_map<std::string, std::string>& talentInfos);
    std::string getFillColor(std::shared_ptr<Talent> talent);
    std::string getShape(TalentType type);
    std::string getSwitchLabel(int talentSwitch);

    void bloodmalletCount(int points);
    void individualCombinationCount(int points);
    void parallelCombinationCount(int points);
    void testground();

    std::unordered_map<std::uint64_t, int> countConfigurationsFast(TalentTree tree);
    std::vector<std::unordered_map<std::uint64_t, int>> countConfigurationsFastParallel(TalentTree tree);
    void expandTreeTalents(TalentTree& tree);
    void expandTalentAndAdvance(std::shared_ptr<Talent> talent);
    void contractTreeTalents(TalentTree& tree);
    void contractTalentAndAdvance(std::shared_ptr<Talent>& talent);
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree);
    void visitTalent(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::unordered_map<std::uint64_t, int>& combinations,
        int& allCombinations
    );
    void visitTalentParallel(
        int talentIndex,
        std::uint64_t visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector<std::unordered_map<std::uint64_t, int>>& combinations,
        std::vector<int>& allCombinations
    );
    inline void setTalent(std::uint64_t& talent, int index);

    void compareCombinations(const std::unordered_map<std::uint64_t, int>& fastCombinations, const std::unordered_set<std::string>& slowCombinations, std::string suffix = "");
    std::string fillOutTreeWithBinaryIndexToString(std::uint64_t comb, TalentTree tree, TreeDAGInfo treeDAG);
    void insert_into_vector(std::vector<int>& v, const int& t);
    /***************************************************************************************
    LEGACY CODE
    ORIGINAL AND VERY SLOW VERSION OF A VERY VERBOSE CONFIGURATION COUNT ALGORITHM THAT DIRECTLY ACTS ON TREE OBJECTS
    ***************************************************************************************/
    std::unordered_set<std::string> countConfigurations(TalentTree tree);
    void pickAndIterate(std::shared_ptr<Talent> talent, TalentTree tree, std::unordered_set<std::string>& configurations, int& count);
    void allocateTalent(std::shared_ptr<Talent> talent, TalentTree& tree);
    void deallocateTalent(std::shared_ptr<Talent> talent, TalentTree& tree);
    std::vector<std::shared_ptr<Talent>> getPossibleTalents(TalentTree tree);
    void checkIfTalentPossibleRecursive(std::shared_ptr<Talent> talent, std::vector<std::shared_ptr<Talent>>& p);
}