#pragma once

#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>

enum class TalentType;
struct TalentTree;
struct Talent;
struct TreeDAGInfo;
void addChild(Talent* parent, Talent* child);
void addParent(Talent* child, Talent* parent);
void pairTalents(Talent* parent, Talent* child);
std::string getTalentString(TalentTree tree);
void printTree(TalentTree tree);
void addTalentAndChildrenToMap(Talent* talent, std::unordered_map<std::string, int>& treeRepresentation);
std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput);
Talent* createTalent(std::string name, int maxPoints);
TalentTree parseTree(std::string treeRep);
std::vector<std::string> splitString(std::string s, std::string delimiter);
void visualizeTree(TalentTree root, std::string suffix);
void visualizeTalentConnections(Talent* root, std::stringstream& connections);
std::string visualizeTalentInformation(TalentTree tree);
void getTalentInfos(Talent* talent, std::unordered_map<std::string, std::string>& talentInfos);
std::string getFillColor(Talent* talent);
std::string getShape(TalentType type);
std::string getSwitchLabel(int talentSwitch);

std::unordered_set<std::string> countConfigurations(TalentTree tree);
void pickAndIterate(Talent* talent, TalentTree tree, std::unordered_set<std::string>& configurations, int& count);
void allocateTalent(Talent* talent, TalentTree& tree);
void deallocateTalent(Talent* talent, TalentTree& tree);
std::vector<Talent*> getPossibleTalents(TalentTree tree);
void checkIfTalentPossibleRecursive(Talent* talent, std::vector<Talent*>& p);

std::unordered_map<std::uint64_t, int> countConfigurationsFast(TalentTree tree);
void expandTreeTalents(TalentTree& tree);
void expandTalentAndAdvance(Talent* talent);
void contractTreeTalents(TalentTree& tree);
void contractTalentAndAdvance(Talent*& talent);
TreeDAGInfo createSortedMinimalDAG(TalentTree tree);
void visitTalent(
    int talentIndex,
    std::vector<int> visitedTalents,
    int currentMultiplier,
    int talentPointsLeft,
    std::set<int> possibleTalents,
    const TreeDAGInfo& sortedTreeDAG,
    std::unordered_map<std::uint64_t, int>& combinations
);
std::uint64_t getBinaryCombinationIndex(std::vector<int> visitedTalents);
int getCombinationCount(const std::unordered_map<std::uint64_t, int>& combinations);

void compareCombinations(const std::unordered_map<std::uint64_t, int>& fastCombinations, const std::unordered_set<std::string>& slowCombinations, std::string suffix = "");
std::string fillOutTreeWithBinaryIndexToString(std::uint64_t comb, TalentTree tree, TreeDAGInfo treeDAG);