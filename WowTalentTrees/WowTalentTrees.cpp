// WowTalentTrees.cpp : Diese Datei enthält die Funktion "main". Hier beginnt und endet die Ausführung des Programms.
//

#include "WowTalentTrees.h"

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>
#include <sstream> 
#include <fstream>
#include "Windows.h"
#include <chrono>
#include <thread>


enum class TalentType {
    ACTIVE, PASSIVE, SWITCH
};

struct TalentTree {
    std::string name = "defaultTree";
    int unspentTalentPoints = 42;
    int spentTalentPoints = 0;
    std::vector<Talent*> talentRoots;
};

struct Talent {
    std::string index = "";
    std::string name = "";
    TalentType type = TalentType::ACTIVE;
    int points = 0;
    int maxPoints = 0;
    int talentSwitch = -1;
    std::vector<Talent*> parents;
    std::vector<Talent*> children;
};

struct TreeDAGInfo {
    std::vector<std::vector<int>> minimalTreeDAG;
    std::vector<Talent*> sortedTalents;
    std::vector<int> rootIndices;
};

void addChild(Talent* parent, Talent* child) {
    parent->children.push_back(child);
}

void addParent(Talent* child, Talent* parent) {
    child->parents.push_back(parent);
}

void pairTalents(Talent* parent, Talent* child) {
    parent->children.push_back(child);
    child->parents.push_back(parent);
}

std::string getTalentString(TalentTree tree) {
    std::unordered_map<std::string, int> treeRepresentation;
    for (auto& root : tree.talentRoots) {
        addTalentAndChildrenToMap(root, treeRepresentation);
    }
    return unorderedMapToString(treeRepresentation, true);
}

void printTree(TalentTree tree) {
    std::cout << tree.name << std::endl;
    std::cout << "Unspent talent points:\t" << tree.unspentTalentPoints << std::endl;
    std::cout << "Spent talent points:\t" << tree.spentTalentPoints << std::endl;
    std::cout << getTalentString(tree) << std::endl;
}

void addTalentAndChildrenToMap(Talent* talent, std::unordered_map<std::string, int>& treeRepresentation) {
    std::string talentName = talent->index;
    if (talent->talentSwitch >= 0) {
        talentName += std::to_string(talent->talentSwitch);
    }
    treeRepresentation[talentName] = talent->points;
    for (int i = 0; i < talent->children.size(); i++) {
        addTalentAndChildrenToMap(talent->children[i], treeRepresentation);
    }
}

std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput) {
    std::vector<std::string> talentsAndPoints;
    talentsAndPoints.reserve(treeRepresentation.size());
    for (auto& talentRepresentation : treeRepresentation) {
        talentsAndPoints.push_back(talentRepresentation.first + std::to_string(talentRepresentation.second));
    }
    if (sortOutput) {
        std::sort(talentsAndPoints.begin(), talentsAndPoints.end());
    }
    std::stringstream treeString;
    for (auto& talentRepresentation : talentsAndPoints) {
        treeString << talentRepresentation << ";";
    }
    return treeString.str();
}

Talent* createTalent(std::string name, int maxPoints) {
    Talent* talent = new Talent();
    talent->index = name;
    talent->maxPoints = maxPoints;
    return talent;
}

/*
Tree representation string is of the format "NAME.TalentType:maxPoints(_ISSWITCH)-PARENT1,PARENT2+CHILD1,CHILD2;NAME:maxPoints-....."
IMPORTANT: no error checking for strings and ISSWITCH is optional and indicates if it is a selection talent!
*/
TalentTree parseTree(std::string treeRep) {
    std::vector<Talent*> roots;
    std::unordered_map<std::string, Talent*> talentTree;

    std::vector<std::string> talentStrings = splitString(treeRep, ";");

    for (int i = 0; i < talentStrings.size(); i++) {
        Talent* t = new Talent();
        std::string talentInfo = talentStrings[i];
        std::string talentName = talentInfo.substr(0, talentInfo.find("."));
        int talentType = std::stoi(talentInfo.substr(talentInfo.find(".") + 1, 1));
        if (talentTree.count(talentName)) {
            t = talentTree[talentName];
        }
        else {
            t->index = talentName;
        }
        t->type = static_cast<TalentType>(talentType);
        std::string maxPointsAndSwitch = talentInfo.substr(talentInfo.find(":") + 1, talentInfo.find("-") - talentInfo.find(":") - 1);
        if (maxPointsAndSwitch.find("_") == std::string::npos) {
            t->maxPoints = std::stoi(maxPointsAndSwitch);
        }
        else {
            t->maxPoints = std::stoi(splitString(maxPointsAndSwitch, "_")[0]);
            t->talentSwitch = 0;
        }
        talentTree[t->index] = t;
        for (auto& parent : splitString(talentInfo.substr(talentInfo.find("-") + 1, talentInfo.find("+") - talentInfo.find("-") - 1), ",")) {
            if (talentTree.count(parent)) {
                addParent(t, talentTree[parent]);
            }
            else {
                Talent* parentTalent = new Talent();
                parentTalent->index = parent;
                talentTree[parent] = parentTalent;
                addParent(t, parentTalent);
            }
        }
        for (auto& child : splitString(talentInfo.substr(talentInfo.find("+") + 1, talentInfo.size() - talentInfo.find("+") - 1), ",")) {
            if (talentTree.count(child)) {
                addChild(t, talentTree[child]);
            }
            else {
                Talent* childTalent = new Talent();
                childTalent->index = child;
                talentTree[child] = childTalent;
                addChild(t, childTalent);
            }
        }
        if (t->parents.size() == 0) {
            roots.push_back(t);
        }
    }

    TalentTree tree;
    tree.talentRoots = roots;

    return tree;
}

std::vector<std::string> splitString(std::string s, std::string delimiter) {
    std::vector<std::string> stringSplit;

    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) {
        token = s.substr(0, pos);
        stringSplit.push_back(token);
        s.erase(0, pos + delimiter.length());
    }

    if(s.length() > 0)
        stringSplit.push_back(s);

    return stringSplit;
}

void visualizeTree(TalentTree tree, std::string suffix) {
    std::cout << "Visualize tree " << tree.name << " " << suffix << std::endl;
    std::string directory = "C:\\Users\\Tobi\\Documents\\Programming\\CodeSnippets\\WowTalentTrees\\TreesInputsOutputs";
    /*
digraph G {
  { 
    node [margin=0 fontcolor=blue fontsize=32 width=0.5 shape=circle style=filled]
    b [fillcolor=yellow fixedsize=true label="a very long label"]
    d [fixedsize=shape label="an even longer label"]
  }
  a -> {c d}
  b -> {c d}
}
    */
    std::stringstream output;
    output << "strict digraph " << tree.name << " {\n";
    output << "label=\"" + tree.name + "\"\n";
    output << "fontname=\"Arial\"\n";
    //define each node individually
    output << "{\n";
    output << "node [fontname=\"Arial\" width=1 height=1 fixedsize=shape style=filled]\n";
    output << visualizeTalentInformation(tree);
    output << "}\n";
    //define node connections
    for (auto& root : tree.talentRoots) {
        std::stringstream connections;
        visualizeTalentConnections(root, connections);
        output << connections.str();
    }
    output << "}";

    std::ofstream f;
    f.open(directory + "\\tree_" + tree.name + suffix + ".txt");
    f << output.str();
    f.close();

    std::ofstream batch_file;
    batch_file.open("commands.cmd", std::ios::trunc);
    batch_file <<
        "cd " << directory << std::endl <<
        "\"C:\\Program Files\\Graphviz\\bin\\dot.exe\" -Tpng \"tree_" + tree.name + suffix + ".txt\" -o \"tree_" + tree.name + suffix +  ".png\"" << std::endl;
    batch_file.close();

    ShellExecuteW(0, L"open", L"cmd.exe", L"/c commands.cmd", 0, SW_HIDE);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    remove("commands.cmd"); // delete the batch file
}

std::string visualizeTalentInformation(TalentTree tree) {
    std::unordered_map<std::string, std::string> talentInfos;

    for (auto& root : tree.talentRoots) {
        getTalentInfos(root, talentInfos);
    }

    std::stringstream talentInfosStream;
    for (auto& talent : talentInfos) {
        talentInfosStream << talent.first << " " << talent.second << std::endl;
    }

    return talentInfosStream.str();
}

void getTalentInfos(Talent* talent, std::unordered_map<std::string, std::string>& talentInfos) {
    talentInfos[talent->index] = "[label=\"" + talent->index + " " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + getSwitchLabel(talent->talentSwitch) + "\" fillcolor=" + getFillColor(talent) + " shape=" + getShape(talent->type) + "]";
    for (auto& child : talent->children) {
        getTalentInfos(child, talentInfos);
    }
}

std::string getFillColor(Talent* talent) {
    if (talent->talentSwitch < 0) {
        if (talent->points == talent->maxPoints) {
            return "darkgoldenrod2";
        }
        else if (talent->points > 0) {
            return "chartreuse3";
        }
        else {
            return "white";
        }
    }
    else if (talent->talentSwitch == 0)
        return "aquamarine3";
    else if (talent->talentSwitch == 1)
        return "coral";
    else
        return "fuchsia";

}

std::string getShape(TalentType type) {
    switch (type) {
    case TalentType::ACTIVE: return "square";
    case TalentType::PASSIVE: return "circle";
    case TalentType::SWITCH: return "octagon";
    }
}

std::string getSwitchLabel(int talentSwitch) {
    if (talentSwitch == 0)
        return "\nleft";
    else if (talentSwitch == 1)
        return "\nright";
    else
        return "";
}

void visualizeTalentConnections(Talent* root, std::stringstream& connections) {
    if (root->children.size() == 0)
        return;
    connections << root->index << " -> {";
    for (auto& child : root->children) {
        connections << child->index << " ";
    }
    connections << "}\n";
    for (auto& child : root->children) {
        visualizeTalentConnections(child, connections);
    }
}

int main()
{
    /*
    Talent* root = createTalent("A1", 1);
    Talent* tb1 = createTalent("B1", 1);
    Talent* tb2 = createTalent("B2", 2);
    Talent* tb3 = createTalent("B3", 1);
    pairTalents(root, tb1);
    pairTalents(root, tb2);
    pairTalents(root, tb3);
    std::cout << printTree(root) << std::endl;
    */

    TalentTree tree = parseTree(
        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
        "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
        "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
    );
    printTree(tree);

    //visualizeTree(tree, "");
    
    std::cout << "Find all configurations with " << tree.unspentTalentPoints << " available talent points!" << std::endl;

    auto t1 = std::chrono::high_resolution_clock::now();
    std::unordered_map<std::uint64_t, int> fast_combinations = countConfigurationsFast(tree);
    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
    std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;

    /*
    tree = parseTree(
        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
        "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
        "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
    );
    //expandTreeTalents(tree);
    t1 = std::chrono::high_resolution_clock::now();
    std::unordered_set<std::string> slow_combinations = countConfigurations(tree);
    t2 = std::chrono::high_resolution_clock::now();
    ms_double = t2 - t1;
    std::cout << "Slow operation time: " << ms_double.count() << " ms" << std::endl;
    */

    //compareCombinations(fast_combinations, slow_combinations);
}

std::unordered_set<std::string> countConfigurations(TalentTree tree) {
    int count = 0;
    std::unordered_set<std::string> configurations;

    pickAndIterate(nullptr, tree, configurations, count);
    
    std::cout << "Found " << configurations.size() << " configurations without and " << count << " configurations with duplicates." << std::endl;
    
    return configurations;
}

void pickAndIterate(Talent* talent, TalentTree tree, std::unordered_set<std::string>& configurations, int& count) {
    if (talent != nullptr)
        allocateTalent(talent, tree);
    std::vector<Talent*> possibleTalents = getPossibleTalents(tree);
    for (auto& ptalent : possibleTalents) {
        pickAndIterate(ptalent, tree, configurations, count);
    }
    if (tree.unspentTalentPoints == 0) {
        count += 1;
        std::string talentString = getTalentString(tree);
        //if(configurations.count(talentString) == 0)
        //visualizeTree(tree, "_" + std::to_string(count));// configurations.size()));
        configurations.insert(talentString);
    }
    if (talent != nullptr)
        deallocateTalent(talent, tree);
}

void allocateTalent(Talent* talent, TalentTree& tree) {
    //try the copy variant, but in case reference variant is needed then here's the place to have a talent stack in TalentTree
    talent->points += 1;
    tree.spentTalentPoints += 1;
    tree.unspentTalentPoints -= 1;
}

void deallocateTalent(Talent* talent, TalentTree& tree) {
    talent->points -= 1;
    tree.spentTalentPoints -= 1;
    tree.unspentTalentPoints += 1;
}

std::vector<Talent*> getPossibleTalents(TalentTree tree) {
    std::vector<Talent*> p;
    if (tree.unspentTalentPoints == 0)
        return p;
    for (auto& root : tree.talentRoots) {
        checkIfTalentPossibleRecursive(root, p);
    }
    return p;
}

void checkIfTalentPossibleRecursive(Talent* talent, std::vector<Talent*>& p) {
    if (talent->points < talent->maxPoints) {
        p.push_back(talent);
    }
    else {
        for (auto& child : talent->children) {
            checkIfTalentPossibleRecursive(child, p);
        }
    }
}

std::unordered_map<std::uint64_t, int> countConfigurationsFast(TalentTree tree) {
    //expand notes in tree
    expandTreeTalents(tree);
    //visualizeTree(tree, "expanded");
    //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
    //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
    TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
    if (sortedTreeDAG.sortedTalents.size() > 64)
        throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
    std::unordered_map<std::uint64_t, int> combinations;

    //iterate through all possible combinations in order:
    //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
    //weight of combination = factor of 2 for every switch talent in path
    std::vector<int> visitedTalents;
    visitedTalents.reserve(sortedTreeDAG.minimalTreeDAG.size());
    int talentPointsLeft = tree.unspentTalentPoints;
    //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
    std::set<int> possibleTalents;
    for (auto& root : sortedTreeDAG.rootIndices) {
        possibleTalents.insert(root);
    }
    for (auto& talent : possibleTalents) {
        visitTalent(talent, visitedTalents, 1, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations);
    }
    std::cout << "Number of configurations without switch talents: " << combinations.size() << " and with: " << getCombinationCount(combinations) << std::endl;

    return combinations;
}

void visitTalent(
    int talentIndex, 
    std::vector<int> visitedTalents, 
    int currentMultiplier,
    int talentPointsLeft, 
    std::set<int> possibleTalents, 
    const TreeDAGInfo& sortedTreeDAG, 
    std::unordered_map<std::uint64_t, int>& combinations
) {
    //for each node visited add child nodes (in DAG array) to the vector of possible nodes and reduce talent points left
    //check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
    //iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
    //if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
    visitedTalents.push_back(talentIndex);
    talentPointsLeft -= 1;
    currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndex][0];
    if (talentPointsLeft == 0) {
        //finish, perform bit shift
        std::uint64_t binaryCombinationIndex = getBinaryCombinationIndex(visitedTalents);
        combinations[binaryCombinationIndex] = currentMultiplier;
        //std::cout << "Insert combination " << binaryCombinationIndex << std::endl;
        return;
    }
    if (sortedTreeDAG.sortedTalents.size() - talentIndex - 1 < talentPointsLeft) {
        //cannot use up all the leftover talent points, therefore incomplete
        return;
    }
    for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndex].size(); i++) {
        possibleTalents.insert(sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
    }
    for (auto& nextTalent : possibleTalents) {
        if (nextTalent > talentIndex) {
            visitTalent(nextTalent, visitedTalents, currentMultiplier, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations);
        }
    }
}

void expandTreeTalents(TalentTree& tree) {
    for (auto& root : tree.talentRoots) {
        expandTalentAndAdvance(root);
    }
}

void expandTalentAndAdvance(Talent* talent) {
    if (talent->maxPoints > 1) {
        std::vector<Talent*> talentParts;
        std::vector<Talent*> originalChildren;
        for (auto& child : talent->children) {
            originalChildren.push_back(child);
        }
        talent->children.clear();
        talentParts.push_back(talent);
        for (int i = 0; i < talent->maxPoints - 1; i++) {
            Talent* t = new Talent();
            t->index = talent->index + "_" + std::to_string(i + 1);
            t->name = talent->name + "_" + std::to_string(i + 1);
            t->type = talent->type;
            t->points = 0;
            t->maxPoints = 1;
            t->talentSwitch = talent->talentSwitch;
            t->parents.push_back(talentParts[i]);
            talentParts.push_back(t);
            talentParts[i]->children.push_back(t);
        }
        talent->index += "_0";
        talent->name += "_0";
        for (auto& child : originalChildren) {
            talentParts[talent->maxPoints - 1]->children = originalChildren;
        }
        for (auto& child : originalChildren) {
            std::vector<Talent*>::iterator i = std::find(child->parents.begin(), child->parents.end(), talent);
            if (i != child->parents.end()) {
                (*i) = talentParts[talent->maxPoints - 1];
            }
            else {
                //If this happens then n has m as child but m does not have n as parent! Bug!
                throw std::logic_error("child has missing parent");
            }
        }
        talent->maxPoints = 1;
        for (auto& child : originalChildren) {
            expandTalentAndAdvance(child);
        }
    }
    else {
        for (auto& child : talent->children) {
            expandTalentAndAdvance(child);
        }
    }
}

void contractTreeTalents(TalentTree& tree) {
    for (auto& root : tree.talentRoots) {
        contractTalentAndAdvance(root);
    }
}

void contractTalentAndAdvance(Talent*& talent) {
    std::vector<std::string> splitIndex = splitString(talent->index, "_");
    if (splitIndex.size() > 1) {
        std::vector<std::string> splitName = splitString(talent->name, "_");
        //talent has to be contracted
        //expanded Talents have 1 child at most and talent chain is at least 2 talents long
        std::string baseIndex = splitIndex[0];
        std::vector<Talent*> talentParts;
        Talent* currTalent = talent;
        talentParts.push_back(talent);
        Talent* childTalent = talent->children[0];
        while(splitString(childTalent->index, "_")[0] == baseIndex) {
            talentParts.push_back(childTalent);
            currTalent = childTalent;
            if (currTalent->children.size() == 0)
                break;
            childTalent = currTalent->children[0];
        }
        Talent* t = new Talent();
        t->index = baseIndex;
        t->name = splitName[0];
        t->type = talent->type;
        t->points = 0;
        for (auto& talent : talentParts) {
            t->points += talent->points;
        }
        t->maxPoints = talentParts.size();
        t->talentSwitch = talent->talentSwitch;
        t->parents = talentParts[0]->parents;
        t->children = talentParts[talentParts.size() - 1]->children;

        //replace talent pointer
        talent = t;
        //iterate through children
        for (auto& child : talent->children) {
            contractTalentAndAdvance(child);
        }
    }
    else {
        for (auto& child : talent->children) {
            contractTalentAndAdvance(child);
        }
    }
}

TreeDAGInfo createSortedMinimalDAG(TalentTree tree) {
    //note: EVEN THOUGH TREE IS PASSED BY COPY ALL PARENTS WILL BE DELETED FROM TALENTS!
    TreeDAGInfo info;
    for (int i = 0; i < tree.talentRoots.size(); i++) {
        info.rootIndices.push_back(i);
    }
    /*
    L ← Empty list that will contain the sorted elements    #info.minimalTreeDAG / info.sortedTalents
    S ← Set of all nodes with no incoming edge              #tree.talentRoots

    while S is not empty do
        remove a node n from S
        add n to L
        for each node m with an edge e from n to m do
            remove edge e from the graph
            if m has no other incoming edges then
                insert m into S

    if graph has edges then
        return error   (graph has at least one cycle)
    else 
        return L   (a topologically sorted order)
    */
    //while tree.talentRoots is not empty do
    while (tree.talentRoots.size() > 0) {
        //remove a node n from tree.talentRoots
        Talent* n = tree.talentRoots.front();
        //note: terrible in theory but should not matter as vectors are small, otherwise use deque
        tree.talentRoots.erase(tree.talentRoots.begin());
        //add n to info.sortedTalents
        //note: this could be simplified by changing the order of removing from tree.talentRoots and adding n to info.minimalTreeDAG
        info.sortedTalents.push_back(n);
        //for each node m with an edge e from n to m do
        for (auto& m : n->children) {
            //remove edge e from the graph
            //note: it should suffice to just remove the parent in m since every node is only visited once so n->children does not have to be changed inside loop
            if (m->parents.size() > 1) {
                std::vector<Talent*>::iterator i = std::find(m->parents.begin(), m->parents.end(), n);
                if(i != m->parents.end()) {
                    m->parents.erase(i);
                }
                else {
                    //If this happens then n has m as child but m does not have n as parent! Bug!
                    throw std::logic_error("child has missing parent");
                }
            }
            else {
                m->parents.clear();
            }
            //if m has no other incoming edges then
            if (m->parents.size() == 0) {
                //insert m into S
                tree.talentRoots.push_back(m);
            }
        }
    }

    //convert sorted talents to minimalTreeDAG
    for (auto& talent : info.sortedTalents) {
        std::vector<int> child_indices(talent->children.size() + 1);
        child_indices[0] = talent->type == TalentType::SWITCH ? 2 : 1;
        for (int i = 0; i < talent->children.size(); i++) {
            ptrdiff_t pos = std::distance(info.sortedTalents.begin(), std::find(info.sortedTalents.begin(), info.sortedTalents.end(), talent->children[i]));
            if (pos >= info.sortedTalents.size()) {
                throw std::logic_error("child does not appear in info.sortedTalents");
            }
            child_indices[i + 1] = pos;
        }
        info.minimalTreeDAG.push_back(child_indices);
    }

    /*
    for (int i = 0; i < info.minimalTreeDAG.size(); i++) {
        std::cout << i << " (" << info.sortedTalents[i]->index << ")  \t: ";
        for (auto& index : info.minimalTreeDAG[i]) {
            std::cout << index << ", ";
        }
        std::cout << std::endl;
    }
    */

    return info;
}

std::uint64_t getBinaryCombinationIndex(std::vector<int> visitedTalents) {
    //if (visitedTalents.size() != 6)
    //    throw std::logic_error("not exactly 6 talent points spent!");
    std::uint64_t index = 0;
    for (auto& talent : visitedTalents) {
        index |= 1ULL << talent;
    }
    return index;
}

int getCombinationCount(const std::unordered_map<std::uint64_t, int>& combinations) {
    int count = 0;
    for (auto& item : combinations) {
        count += item.second;
    }
    return count;
}

void compareCombinations(const std::unordered_map<std::uint64_t, int>& fastCombinations, const std::unordered_set<std::string>& slowCombinations, std::string suffix) {
    std::string directory = "C:\\Users\\Tobi\\Documents\\Programming\\CodeSnippets\\WowTalentTrees\\TreesInputsOutputs";

    std::vector<std::string> fastCombinationsOrdered;
    for (auto& comb : fastCombinations) {
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
        expandTreeTalents(tree);
        //creating a treeDAG destroys all parents in the tree, but should not be necessary anyway
        TreeDAGInfo treeDAG = createSortedMinimalDAG(tree);
        fastCombinationsOrdered.push_back(fillOutTreeWithBinaryIndexToString(comb.first, tree, treeDAG));
    }
    std::sort(fastCombinationsOrdered.begin(), fastCombinationsOrdered.end());

    std::ofstream output_file_fast(directory + "\\trees_comparison_" + suffix + "_fast.txt");
    std::ostream_iterator<std::string> output_iterator_fast(output_file_fast, "\n");
    std::copy(fastCombinationsOrdered.begin(), fastCombinationsOrdered.end(), output_iterator_fast);

    std::vector<std::string> slowCombinationsOrdered;
    for (auto& comb : slowCombinations) {
        slowCombinationsOrdered.push_back(comb);
    }
    std::sort(slowCombinationsOrdered.begin(), slowCombinationsOrdered.end());
    
    std::ofstream output_file_slow(directory + "\\trees_comparison_" + suffix + "_slow.txt");
    std::ostream_iterator<std::string> output_iterator_slow(output_file_slow, "\n");
    std::copy(slowCombinationsOrdered.begin(), slowCombinationsOrdered.end(), output_iterator_slow);
}

std::string fillOutTreeWithBinaryIndexToString(std::uint64_t comb, TalentTree tree, TreeDAGInfo treeDAG) {
    for (int i = 0; i < 64; i++) {
        //check if bit is on
        bool bitSet = (comb >> i) & 1U;
        //select talent in tree
        if (bitSet) {
            if (i >= treeDAG.sortedTalents.size())
                throw std::logic_error("bit of a talent that does not exist is set!");
            treeDAG.sortedTalents[i]->points = 1;
        }
    }
    contractTreeTalents(tree);
    //visualizeTree(tree, "debugContracted");

    return getTalentString(tree);
}