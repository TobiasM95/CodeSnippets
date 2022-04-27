#include "WowTalentTrees.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream> 
#include <fstream>
#include "Windows.h"
#include <chrono>
#include <thread>

// Switch talents can select/switch between 2 talents in the same slot
enum class TalentType {
    ACTIVE, PASSIVE, SWITCH
};

/*
A tree has a name, (un)spent talent points and a list of root talents (talents without parents) that are the starting point
*/
struct TalentTree {
    std::string name = "defaultTree";
    int unspentTalentPoints = 30;
    int spentTalentPoints = 0;
    std::vector<std::shared_ptr<Talent>> talentRoots;
};

/*
A talent has an index (scheme: https://github.com/Bloodmallet/simc_support/blob/feature/10-0-experiments/simc_support/game_data/full_tree_coordinates.jpg),
a name (currently not used), a type, the (max) points and a switch (might make the talent type redundant) as well as a list of all parents and children in
a simple graph structure.
*/
struct Talent {
    std::string index = "";
    std::string name = "";
    TalentType type = TalentType::ACTIVE;
    int points = 0;
    int maxPoints = 0;
    int talentSwitch = -1;
    std::vector<std::shared_ptr<Talent>> parents;
    std::vector<std::shared_ptr<Talent>> children;
};

/*
This is the container for the heavily optimized, topologically sorted DAG variant of the talent tree.
The regular talent tree has all the meta information and easy readable/debugable structures whereas this container
only has integer indices with an unconnected raw list of talents for computational efficieny.
NOTE: The talents aren't selected (i.e. Talent::points incremented) at all but a flag is set in a uint64 which is used
as an indexer. There exist routines that translate from uint64 to a regular tree and in the future maybe vice versa.
*/
struct TreeDAGInfo {
    std::vector<std::vector<int>> minimalTreeDAG;
    std::vector<std::shared_ptr<Talent>> sortedTalents;
    std::vector<int> rootIndices;
};

//Tree/talent helper functions

void addChild(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child) {
    parent->children.push_back(child);
}
void addParent(std::shared_ptr<Talent> child, std::shared_ptr<Talent> parent) {
    child->parents.push_back(parent);
}
void pairTalents(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child) {
    parent->children.push_back(child);
    child->parents.push_back(parent);
}

/*
Transforms a skilled talent tree into a string. That string does not contain the tree structure, just the selected talents.
*/
std::string getTalentString(TalentTree tree) {
    std::unordered_map<std::string, int> treeRepresentation;
    for (auto& root : tree.talentRoots) {
        addTalentAndChildrenToMap(root, treeRepresentation);
    }
    return unorderedMapToString(treeRepresentation, true);
}

/*
Prints some informations about a specific tree.
*/
void printTree(TalentTree tree) {
    std::cout << tree.name << std::endl;
    std::cout << "Unspent talent points:\t" << tree.unspentTalentPoints << std::endl;
    std::cout << "Spent talent points:\t" << tree.spentTalentPoints << std::endl;
    std::cout << getTalentString(tree) << std::endl;
}

/*
Helper function that adds a talent and its children recursively to a map (and adds talent switch information if existing).
*/
void addTalentAndChildrenToMap(std::shared_ptr<Talent> talent, std::unordered_map<std::string, int>& treeRepresentation) {
    std::string talentName = talent->index;
    if (talent->talentSwitch >= 0) {
        talentName += std::to_string(talent->talentSwitch);
    }
    treeRepresentation[talentName] = talent->points;
    for (int i = 0; i < talent->children.size(); i++) {
        addTalentAndChildrenToMap(talent->children[i], treeRepresentation);
    }
}

/*
Helper function that transforms a map that includes talents and their skill points to their respective string representation.
*/
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

/*
Helper function that creates a talent with the given index name and max points.
*/
std::shared_ptr<Talent> createTalent(std::string name, int maxPoints) {
    std::shared_ptr<Talent> talent = std::make_shared<Talent>();
    talent->index = name;
    talent->maxPoints = maxPoints;
    return talent;
}

/*
Tree representation string is of the format "NAME.TalentType:maxPoints(_ISSWITCH)-PARENT1,PARENT2+CHILD1,CHILD2;NAME:maxPoints-....."
IMPORTANT: no error checking for strings and ISSWITCH is optional and indicates if it is a selection talent!
*/
TalentTree parseTree(std::string treeRep) {
    std::vector<std::shared_ptr<Talent>> roots;
    std::unordered_map<std::string, std::shared_ptr<Talent>> talentTree;

    std::vector<std::string> talentStrings = splitString(treeRep, ";");

    for (int i = 0; i < talentStrings.size(); i++) {
        std::shared_ptr<Talent> t = std::make_shared<Talent>();
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
                std::shared_ptr<Talent> parentTalent = std::make_shared<Talent>();
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
                std::shared_ptr<Talent> childTalent = std::make_shared<Talent>();
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

/*
Helper function that splits a string given the delimiter, if string does not contain delimiter then whole string is returned.
*/
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


/*
Visualizes a given tree with graphviz. Needs to be installed and the paths have to exist. Generally not safe to use without careful skimming through it.
*/
void visualizeTree(TalentTree tree, std::string suffix) {
    std::cout << "Visualize tree " << tree.name << " " << suffix << std::endl;
    std::string directory = "C:\\Users\\Tobi\\Documents\\Programming\\CodeSnippets\\WowTalentTrees\\TreesInputsOutputs";

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

    //output txt file in graphviz format
    std::ofstream f;
    f.open(directory + "\\tree_" + tree.name + suffix + ".txt");
    f << output.str();
    f.close();

    std::string command = "\"\"C:\\Program Files\\Graphviz\\bin\\dot.exe\" -Tpng \"" + directory + "\\tree_" + tree.name + suffix + ".txt\" -o \"" + directory + "\\tree_" + tree.name + suffix + ".png\"\"";
    system(command.c_str());
}

/*
Helper function that gathers all individual graphviz talent visualization strings in graphviz language into one string.
*/
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

/*
Helper function that recursively gets the specific graphviz visualization string for a talent and its children.
*/
void getTalentInfos(std::shared_ptr<Talent> talent, std::unordered_map<std::string, std::string>& talentInfos) {
    talentInfos[talent->index] = "[label=\"" + talent->index + " " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + getSwitchLabel(talent->talentSwitch) + "\" fillcolor=" + getFillColor(talent) + " shape=" + getShape(talent->type) + "]";
    for (auto& child : talent->children) {
        getTalentInfos(child, talentInfos);
    }
}

/*
Helper function that returns the appropriate fill color for different types of talents and points allocations.
*/
std::string getFillColor(std::shared_ptr<Talent> talent) {
    if (talent->points == 0) {
        return "white";
    }
    else if (talent->talentSwitch < 0) {
        if (talent->points == talent->maxPoints) {
            return "darkgoldenrod2";
        }
        else {
            return "chartreuse3";
        }
    }
    else if (talent->talentSwitch == 0)
        return "aquamarine3";
    else if (talent->talentSwitch == 1)
        return "coral";
    else
        return "fuchsia";

}

/*
Helper function that defines the shape of each talent type.
*/
std::string getShape(TalentType type) {
    switch (type) {
    case TalentType::ACTIVE: return "square";
    case TalentType::PASSIVE: return "circle";
    case TalentType::SWITCH: return "octagon";
    }
}

/*
Helper function that displays the switch state of a given talent.
*/
std::string getSwitchLabel(int talentSwitch) {
    if (talentSwitch == 0)
        return "\nleft";
    else if (talentSwitch == 1)
        return "\nright";
    else
        return "";
}

/*
Helper function that recursively creates the graphviz visualization string of all talent connections. (We only need connections from parents to children.
Also, graph is defined as strict digraph so we don't need to take care of duplicate arrows.
*/
void visualizeTalentConnections(std::shared_ptr<Talent> root, std::stringstream& connections) {
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

void main() {
    //This snippet runs the configuration count for 1 to 42 available talent points.
    for (int i = 1; i < 43; i++) {
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
        tree.unspentTalentPoints = i;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::unordered_map<std::uint64_t, int> fast_combinations = countConfigurationsFast(tree);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
    }

    //Additional useful functions:
    //visualizeTree(tree, "");
    
    //this function should not be called without knowing what it does, purely for debugging/error checking purposes.
    //but the source code contains more useful function usages to expand/contract trees, converting uint64 indices of DAGs to a tree, etc.
    //compareCombinations(fast_combinations, slow_combinations);
}

void testground()
{
    /*
    std::shared_ptr<Talent> root = createTalent("A1", 1);
    std::shared_ptr<Talent> tb1 = createTalent("B1", 1);
    std::shared_ptr<Talent> tb2 = createTalent("B2", 2);
    std::shared_ptr<Talent> tb3 = createTalent("B3", 1);
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

    std::unordered_set<std::string> slow_combinations;
    compareCombinations(fast_combinations, slow_combinations);
}


/*
Counts configurations of a tree with given amount of talent points by topologically sorting the tree and iterating through valid paths (i.e.
paths with monotonically increasing talent indices). See Wikipedia DAGs (which Wow Talent Trees are) and Topological Sorting.
Todo: Create a bulk count algorithm that does not employ early stopping if talent tree can't be filled anymore but keeps track of all sub tree binary indices
to do all different talent points calculations in a single run
*/
std::unordered_map<std::uint64_t, int> countConfigurationsFast(TalentTree tree) {
    int talentPoints = tree.unspentTalentPoints;
    //expand notes in tree
    expandTreeTalents(tree);
    //visualizeTree(tree, "expanded");
    //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
    //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
    TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
    if (sortedTreeDAG.sortedTalents.size() > 64)
        throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
    std::unordered_map<std::uint64_t, int> combinations;
    int allCombinations = 0;

    //iterate through all possible combinations in order:
    //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
    //weight of combination = factor of 2 for every switch talent in path
    std::uint64_t visitedTalents = 0;
    int talentPointsLeft = tree.unspentTalentPoints;
    //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
    std::set<int> possibleTalents;
    for (auto& root : sortedTreeDAG.rootIndices) {
        possibleTalents.insert(root);
    }
    for (auto& talent : possibleTalents) {
        visitTalent(talent, visitedTalents, 1, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
    }
    std::cout << "Number of configurations for " << talentPoints << " talent points without switch talents: " << combinations.size() << " and with : " << allCombinations << std::endl;

    return combinations;
}

void visitTalent(
    int talentIndex,
    std::uint64_t visitedTalents,
    int currentMultiplier,
    int talentPointsLeft,
    std::set<int> possibleTalents,
    const TreeDAGInfo& sortedTreeDAG,
    std::unordered_map<std::uint64_t, int>& combinations,
    int& allCombinations
) {
    //for each node visited add child nodes (in DAG array) to the vector of possible nodes and reduce talent points left
    //check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
    //iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
    //if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
    setTalent(visitedTalents, talentIndex);
    talentPointsLeft -= 1;
    currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndex][0];
    if (talentPointsLeft == 0) {
        //finish, perform bit shift
        combinations[visitedTalents] = currentMultiplier;
        allCombinations += currentMultiplier;
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
            visitTalent(nextTalent, visitedTalents, currentMultiplier, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
    }
}

void expandTreeTalents(TalentTree& tree) {
    for (auto& root : tree.talentRoots) {
        expandTalentAndAdvance(root);
    }
}

void expandTalentAndAdvance(std::shared_ptr<Talent> talent) {
    if (talent->maxPoints > 1) {
        std::vector<std::shared_ptr<Talent>> talentParts;
        std::vector<std::shared_ptr<Talent>> originalChildren;
        for (auto& child : talent->children) {
            originalChildren.push_back(child);
        }
        talent->children.clear();
        talentParts.push_back(talent);
        for (int i = 0; i < talent->maxPoints - 1; i++) {
            std::shared_ptr<Talent> t = std::make_shared<Talent>();
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
            std::vector<std::shared_ptr<Talent>>::iterator i = std::find(child->parents.begin(), child->parents.end(), talent);
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

void contractTalentAndAdvance(std::shared_ptr<Talent>& talent) {
    std::vector<std::string> splitIndex = splitString(talent->index, "_");
    if (splitIndex.size() > 1) {
        std::vector<std::string> splitName = splitString(talent->name, "_");
        //talent has to be contracted
        //expanded Talents have 1 child at most and talent chain is at least 2 talents long
        std::string baseIndex = splitIndex[0];
        std::vector<std::shared_ptr<Talent>> talentParts;
        std::shared_ptr<Talent> currTalent = talent;
        talentParts.push_back(talent);
        std::shared_ptr<Talent> childTalent = talent->children[0];
        while(splitString(childTalent->index, "_")[0] == baseIndex) {
            talentParts.push_back(childTalent);
            currTalent = childTalent;
            if (currTalent->children.size() == 0)
                break;
            childTalent = currTalent->children[0];
        }
        std::shared_ptr<Talent> t = std::make_shared<Talent>();
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
        std::shared_ptr<Talent> n = tree.talentRoots.front();
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
                std::vector<std::shared_ptr<Talent>>::iterator i = std::find(m->parents.begin(), m->parents.end(), n);
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

inline void setTalent(std::uint64_t& talent, int index) {
    talent |= 1ULL << index;
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
    int count = 0;
    for (auto& comb : fastCombinations) {
        if (count++ % 100 == 0)
            std::cout << count << std::endl;
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
    bool filterFlag = false;
    for (int i = 0; i < 64; i++) {
        //check if bit is on
        bool bitSet = (comb >> i) & 1U;
        //select talent in tree
        if (bitSet) {
            if (i >= treeDAG.sortedTalents.size())
                throw std::logic_error("bit of a talent that does not exist is set!");
            treeDAG.sortedTalents[i]->points = 1;
            if (i == 190000)
                filterFlag = true;
        }
    }
    contractTreeTalents(tree);
    if(filterFlag)
        visualizeTree(tree, "7points_E2_" + std::to_string(comb));

    return getTalentString(tree);
}








/***************************************************************************************
LEGACY CODE
ORIGINAL AND VERY SLOW VERSION OF A VERY VERBOSE CONFIGURATION COUNT ALGORITHM THAT DIRECTLY ACTS ON TREE OBJECTS
NOTE: Is extremely slow cause it runs through every possible path to the end even if it ends in duplicates.
This can be fixed by keeping track of partial paths in a set (with binary indexers) but would be make it less verbose and isn't faster
than topologically sorting trees anyway so it's not worth to do it anyway. This is for academic purposes anyway. 
Anything above 10 talent points runs extremely slow.
***************************************************************************************/

/*
Creates every possible path (multiple paths can have same talents selected but got there by different selection orders.
This algorithm does not prevent dupliactes and therefore is unusably slow) by walking through the tree individually and sequentially.
*/
std::unordered_set<std::string> countConfigurations(TalentTree tree) {
    int count = 0;
    std::unordered_set<std::string> configurations;

    pickAndIterate(nullptr, tree, configurations, count);

    std::cout << "Found " << configurations.size() << " configurations without and " << count << " configurations with duplicates." << std::endl;

    return configurations;
}

/*
Recursive function that picks a talent (iterates through the list of possible talents) allocates it and calls the same function again.
It operates on a single physical tree (since it runs sequentially) to save memory, tradeoff is that you have to deallocate a talent at the end.
*/
void pickAndIterate(std::shared_ptr<Talent> talent, TalentTree tree, std::unordered_set<std::string>& configurations, int& count) {
    if (talent != nullptr)
        allocateTalent(talent, tree);
    std::vector<std::shared_ptr<Talent>> possibleTalents = getPossibleTalents(tree);
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

/*
Helper function that allocates a talent in the tree.
*/
void allocateTalent(std::shared_ptr<Talent> talent, TalentTree& tree) {
    talent->points += 1;
    tree.spentTalentPoints += 1;
    tree.unspentTalentPoints -= 1;
}

/*
Helper function that deallocates a talent in the tree.
*/
void deallocateTalent(std::shared_ptr<Talent> talent, TalentTree& tree) {
    talent->points -= 1;
    tree.spentTalentPoints -= 1;
    tree.unspentTalentPoints += 1;
}

/*
Iterates through the tree recursively to check if a talent is available according to the standard rules.
*/
std::vector<std::shared_ptr<Talent>> getPossibleTalents(TalentTree tree) {
    std::vector<std::shared_ptr<Talent>> p;
    if (tree.unspentTalentPoints == 0)
        return p;
    for (auto& root : tree.talentRoots) {
        checkIfTalentPossibleRecursive(root, p);
    }
    return p;
}

/*
Checks if a talent is available (points < max points) or if it's filled already and moves on to children.
Does not need to check parent filled status cause it runs recursively and only advances to children if talent is filled.
*/
void checkIfTalentPossibleRecursive(std::shared_ptr<Talent> talent, std::vector<std::shared_ptr<Talent>>& p) {
    if (talent->points < talent->maxPoints) {
        p.push_back(talent);
    }
    else {
        for (auto& child : talent->children) {
            checkIfTalentPossibleRecursive(child, p);
        }
    }
}