#include "WowTalentTrees.h"
#include "BloodmalletCounter.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <execution>
#include <sstream> 
#include <fstream>
#include "Windows.h"
#include <chrono>
#include <thread>

int main() {
    auto t1 = std::chrono::high_resolution_clock::now();

    //WowTalentTrees::bloodmalletCount(16);
    //WowTalentTrees::individualCombinationCount(30);
    WowTalentTrees::parallelCombinationCount(30);
    //WowTalentTrees::parallelCombinationCountThreaded(30);

    auto t2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
    std::cout << "Final operation time: " << ms_double.count() << " ms" << std::endl;
}

namespace WowTalentTrees {
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
        int pointsRequired = 0;
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

        if (s.length() > 0)
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
        default: return "square";
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

    void bloodmalletCount(int points) {
        std::vector<std::shared_ptr<bloodmallet::Talent>> talents;
        talents = bloodmallet::_create_talents();
        talents = bloodmallet::_talent_post_init(talents);
        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<bool>> paths = bloodmallet::igrow(talents, points - 1);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
        std::cout << points << ": " << paths.size() << std::endl;
    }

    void individualCombinationCount(int points) {
        if (false) {
            std::vector<int> foo;
            for (int i = 0; i < 43; i++)
                foo.push_back(i);
            std::for_each(
                std::execution::par,
                foo.begin(),
                foo.end(),
                [](auto&& item)
                {
#ifdef _DEBUG
                    TalentTree tree = parseTree(
                        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
                        "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
                        "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
                    );
#else
                    TalentTree tree = parseTree(
                        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1,D1;B2.1:2-A1+C2;B3.1:1-A1+C3,D2;C1.0:1-B1+E1,D1;C2.0:1-B2+D1,D2,E2;C3.0:1-B3+D2,E4,D3;D1.1:2-B1,C1,C2+E1,E2,F2;D2.1:2-B3,C2,C3+E2,F3,E4;D3.1:2-C3+E4;E1.1:3-C1,D1+F1,F2;E2.2:1_0-C2,D1,D2+F2,F3;E4.1:1-C3,D2,D3+F3,F4;"
                        "F1.1:1-E1+G1,H1;F2.1:2-D1,E1,E2+G1;F3.1:2-D2,E2,E4+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H1,H3;G3.1:1-F3,F4+H3,H4;G4.1:2-F4+H4;H1.2:1_0-F1,G1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G3,G4+I4,I5;"
                        "I1.1:1-H1+J1;I2.1:1-H1+J1,J3;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3,J5;I5.1:1-H4+J5;J1.2:1_0-I1,I2+;J3.2:1_0-I2,I3,I4+;J5.2:1_0-I4,I5+;"
                    );
#endif
                    tree.unspentTalentPoints = item;

                    auto t1 = std::chrono::high_resolution_clock::now();
                    std::vector<std::pair<std::bitset<128>, int>> fast_combinations = countConfigurationsFast(tree);
                    auto t2 = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
                    std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
                });
        }
        else {
            if (points <= 0) {
                //This snippet runs the configuration count for 1 to 42 available talent points.
                for (int i = 1; i < 43; i++) {
#ifdef _DEBUG
                    TalentTree tree = parseTree(
                        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
                        "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
                        "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
                    );
#else
                    TalentTree tree = parseTree(
                        "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1,D1;B2.1:2-A1+C2;B3.1:1-A1+C3,D2;C1.0:1-B1+E1,D1;C2.0:1-B2+D1,D2,E2;C3.0:1-B3+D2,E4,D3;D1.1:2-B1,C1,C2+E1,E2,F2;D2.1:2-B3,C2,C3+E2,F3,E4;D3.1:2-C3+E4;E1.1:3-C1,D1+F1,F2;E2.2:1_0-C2,D1,D2+F2,F3;E4.1:1-C3,D2,D3+F3,F4;"
                        "F1.1:1-E1+G1,H1;F2.1:2-D1,E1,E2+G1;F3.1:2-D2,E2,E4+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H1,H3;G3.1:1-F3,F4+H3,H4;G4.1:2-F4+H4;H1.2:1_0-F1,G1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G3,G4+I4,I5;"
                        "I1.1:1-H1+J1;I2.1:1-H1+J1,J3;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3,J5;I5.1:1-H4+J5;J1.2:1_0-I1,I2+;J3.2:1_0-I2,I3,I4+;J5.2:1_0-I4,I5+;"
                    );
#endif
                    tree.unspentTalentPoints = i;

                    auto t1 = std::chrono::high_resolution_clock::now();
                    std::vector<std::pair<std::bitset<128>, int>> fast_combinations = countConfigurationsFast(tree);
                    auto t2 = std::chrono::high_resolution_clock::now();
                    std::chrono::duration<double, std::milli> ms_double = t2 - t1;
                    std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
                }
            }
            else {
#ifdef _DEBUG
                TalentTree tree = parseTree(
                    "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
                    "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
                    "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
                );
#else
                TalentTree tree = parseTree(
                    "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1,D1;B2.1:2-A1+C2;B3.1:1-A1+C3,D2;C1.0:1-B1+E1,D1;C2.0:1-B2+D1,D2,E2;C3.0:1-B3+D2,E4,D3;D1.1:2-B1,C1,C2+E1,E2,F2;D2.1:2-B3,C2,C3+E2,F3,E4;D3.1:2-C3+E4;E1.1:3-C1,D1+F1,F2;E2.2:1_0-C2,D1,D2+F2,F3;E4.1:1-C3,D2,D3+F3,F4;"
                    "F1.1:1-E1+G1,H1;F2.1:2-D1,E1,E2+G1;F3.1:2-D2,E2,E4+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H1,H3;G3.1:1-F3,F4+H3,H4;G4.1:2-F4+H4;H1.2:1_0-F1,G1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G3,G4+I4,I5;"
                    "I1.1:1-H1+J1;I2.1:1-H1+J1,J3;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3,J5;I5.1:1-H4+J5;J1.2:1_0-I1,I2+;J3.2:1_0-I2,I3,I4+;J5.2:1_0-I4,I5+;"
                );
#endif
                tree.unspentTalentPoints = points;

                auto t1 = std::chrono::high_resolution_clock::now();
                std::vector<std::pair<std::bitset<128>, int>> fast_combinations = countConfigurationsFast(tree);
                auto t2 = std::chrono::high_resolution_clock::now();
                std::chrono::duration<double, std::milli> ms_double = t2 - t1;
                std::cout << "Fast operation time: " << ms_double.count() << " ms" << std::endl;
            }
        }

        //Additional useful functions:
        //visualizeTree(tree, "");

        //this function should not be called without knowing what it does, purely for debugging/error checking purposes.
        //but the source code contains more useful function usages to expand/contract trees, converting uint64 indices of DAGs to a tree, etc.
        //compareCombinations(fast_combinations, slow_combinations);
    }

    void parallelCombinationCount(int points) {
#ifdef _DEBUG
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
#else
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1,D1;B2.1:2-A1+C2;B3.1:1-A1+C3,D2;C1.0:1-B1+E1,D1;C2.0:1-B2+D1,D2,E2;C3.0:1-B3+D2,E4,D3;D1.1:2-B1,C1,C2+E1,E2,F2;D2.1:2-B3,C2,C3+E2,F3,E4;D3.1:2-C3+E4;E1.1:3-C1,D1+F1,F2;E2.2:1_0-C2,D1,D2+F2,F3;E4.1:1-C3,D2,D3+F3,F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-D1,E1,E2+G1;F3.1:2-D2,E2,E4+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H1,H3;G3.1:1-F3,F4+H3,H4;G4.1:2-F4+H4;H1.2:1_0-F1,G1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G3,G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+J1,J3;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3,J5;I5.1:1-H4+J5;J1.2:1_0-I1,I2+;J3.2:1_0-I2,I3,I4+;J5.2:1_0-I4,I5+;"
        );
#endif
        tree.unspentTalentPoints = points;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<std::pair<std::bitset<128>, int>>> fast_combinations = countConfigurationsFastParallel(tree);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast parallel operation time: " << ms_double.count() << " ms" << std::endl;
    }

    void parallelCombinationCountThreaded(int points) {
#ifdef _DEBUG
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1;B2.1:2-A1+C2;B3.1:1-A1+C3;C1.0:1-B1+E1,D1;C2.0:1-B2+;C3.0:1-B3+D2,E4,D3;D1.1:2-C1+E2;D2.1:2-C3+E2;D3.1:2-C3+;E1.1:3-C1+F1;E2.2:1_0-D1,D2+F2,F3;E4.1:1-C3+F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-E2+G1;F3.1:2-E2+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H3;G3.1:1-F3,F4+H3;G4.1:2-F4+H4;H1.2:1_0-F1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3;I5.1:1-H4+J5;J1.2:1_0-I1+;J3.2:1_0-I3,I4+;J5.2:1_0-I5+;"
        );
#else
        TalentTree tree = parseTree(
            "A1.0:1-+B1,B2,B3;B1.0:1-A1+C1,D1;B2.1:2-A1+C2;B3.1:1-A1+C3,D2;C1.0:1-B1+E1,D1;C2.0:1-B2+D1,D2,E2;C3.0:1-B3+D2,E4,D3;D1.1:2-B1,C1,C2+E1,E2,F2;D2.1:2-B3,C2,C3+E2,F3,E4;D3.1:2-C3+E4;E1.1:3-C1,D1+F1,F2;E2.2:1_0-C2,D1,D2+F2,F3;E4.1:1-C3,D2,D3+F3,F4;"
            "F1.1:1-E1+G1,H1;F2.1:2-D1,E1,E2+G1;F3.1:2-D2,E2,E4+G3;F4.1:1-E4+G3,G4;G1.2:1_0-F1,F2+H1,H3;G3.1:1-F3,F4+H3,H4;G4.1:2-F4+H4;H1.2:1_0-F1,G1+I1,I2,I3;H3.1:1-G1,G3+I3,I4;H4.0:1-G3,G4+I4,I5;"
            "I1.1:1-H1+J1;I2.1:1-H1+J1,J3;I3.1:2-H1,H3+J3;I4.1:2-H3,H4+J3,J5;I5.1:1-H4+J5;J1.2:1_0-I1,I2+;J3.2:1_0-I2,I3,I4+;J5.2:1_0-I4,I5+;"
        );
#endif
        tree.unspentTalentPoints = points;

        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<std::vector<std::pair<std::bitset<128>, int>>>> fast_combinations = countConfigurationsFastParallelThreaded(tree);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Fast parallel operation time: " << ms_double.count() << " ms" << std::endl;
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
        std::vector<std::pair<std::bitset<128>, int>> fast_combinations = countConfigurationsFast(tree);
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
        //compareCombinations(fast_combinations, slow_combinations);
    }


    /*
    Counts configurations of a tree with given amount of talent points by topologically sorting the tree and iterating through valid paths (i.e.
    paths with monotonically increasing talent indices). See Wikipedia DAGs (which Wow Talent Trees are) and Topological Sorting.
    Todo: Create a bulk count algorithm that does not employ early stopping if talent tree can't be filled anymore but keeps track of all sub tree binary indices
    to do all different talent points calculations in a single run
    */
    std::vector<std::pair<std::bitset<128>, int>> countConfigurationsFast(TalentTree tree) {
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::pair<std::bitset<128>, int>> combinations;
        int allCombinations = 0;
        int* mDAG = convertMinimalTreeDAGToArray(sortedTreeDAG);
        int* ptsReq = convertMinimalTreeDAGToPtsReqArray(sortedTreeDAG);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::bitset<128> visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::vector<int> possibleTalents;
        possibleTalents.reserve(sortedTreeDAG.minimalTreeDAG.size());
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(root);
        }
        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired == 0)
                visitTalent(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, mDAG, ptsReq, combinations, allCombinations);
        }
        std::cout << "Number of configurations for " << talentPoints << " talent points without switch talents: " << combinations.size() << " and with : " << allCombinations << std::endl;

        free(mDAG);
        free(ptsReq);
        return combinations;
    }

    /*
    Core recursive function in fast combination counting. Keeps track of selected talents, checks if combination is complete or cannot be finished (early stopping),
    and iterates through possible children in a sorted fashion to prevent duplicates.
    */
    void visitTalent(
        int talentIndex,
        std::bitset<128> visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        int* mDAG,
        int* ptsReq,
        std::vector<std::pair<std::bitset<128>, int>>& combinations,
        int& allCombinations
    ) {
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndex);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= getValueFromMDAGArray(mDAG, talentIndex, 0);
        //check if path is complete
        if (talentPointsLeft == 0) {
            combinations.push_back(std::pair<std::bitset<128>, int>(visitedTalents, currentMultiplier));
            allCombinations += currentMultiplier;
            return;
        }
        //check if path can be finished (due to sorting and early stopping some paths are ignored even though in practice you could complete them but
        //sorting guarantees that these paths were visited earlier already)
        if (*mDAG - talentIndex - 1 < talentPointsLeft) {
            //cannot use up all the leftover talent points, therefore incomplete
            return;
        }
        //add all possible children to the set for iteration
        for (int i = 1; i < getConnectionCountFromMDAGArray(mDAG, talentIndex); i++) {
            insert_into_vector(possibleTalents, getValueFromMDAGArray(mDAG, talentIndex, i));
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check if next talent is in right order andn talentPointsSpent is >= next talent points required
            if (possibleTalents[i] > talentIndex &&
                talentPointsSpent >= *(ptsReq+possibleTalents[i])) {
                visitTalent(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, mDAG, ptsReq, combinations, allCombinations);
            }
        }
    }


    /*
    Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
    compared to single N count but includes all combinations for 1 up to N talent points.
    */
    std::vector<std::vector<std::pair<std::bitset<128>, int>>> countConfigurationsFastParallel(TalentTree tree) {
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::vector<std::pair<std::bitset<128>, int>>> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
        std::bitset<128> visitedTalents = 0;
        int talentPointsLeft = tree.unspentTalentPoints;
        //std::vector<StartPoint> startPoints = getStartPoints(sortedTreeDAG, talentPointsLeft);
        //note:this will auto sort (not necessary but also doesn't hurt) and prevent duplicates
        std::vector<int> possibleTalents;
        //add roots to the list of possible talents first, then iterate recursively with visitTalent
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(root);
        }

        for (int i = 0; i < possibleTalents.size(); i++) {
            //only start with root nodes that have points required == 0, prevents from starting at root nodes that might come later in the tree (e.g. druid wild charge)
            if (sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired == 0) 
                    visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, 1, 0, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
        }
        for (int i = 0; i < talentPoints; i++) {
            std::cout << "Number of configurations for " << i + 1 << " talent points without switch talents: " << combinations[i].size() << " and with : " << allCombinations[i] << std::endl;
        }

        return combinations;
    }

    /*
 Parallel version of fast configuration counting that runs slower for individual Ns (where N is the amount of available talent points and N >= smallest path from top to bottom)
 compared to single N count but includes all combinations for 1 up to N talent points.
 */
    std::vector<std::vector<std::vector<std::pair<std::bitset<128>, int>>>> countConfigurationsFastParallelThreaded(TalentTree tree) {
        int talentPoints = tree.unspentTalentPoints;
        //expand notes in tree
        expandTreeTalents(tree);
        //visualizeTree(tree, "expanded");

        //create sorted DAG (is vector of vector and at most nx(m+1) Array where n = # nodes and m is the max amount of connections a node has to childs and 
        //+1 because first column contains the weight (1 for regular talents and 2 for switch talents))
        TreeDAGInfo sortedTreeDAG = createSortedMinimalDAG(tree);
        if (sortedTreeDAG.sortedTalents.size() > 64)
            throw std::logic_error("Number of talents exceeds 64, need different indexing type instead of uint64");
        std::vector<std::vector<std::pair<std::bitset<128>, int>>> combinations;
        combinations.resize(talentPoints);
        std::vector<int> allCombinations;
        allCombinations.resize(talentPoints, 0);

        int talentPointsLeft = tree.unspentTalentPoints;
        int threadCount = 100;
        std::vector<StartPoint> startPoints = getStartPoints(sortedTreeDAG, talentPointsLeft, threadCount);
        std::vector< std::vector<std::vector<std::pair<std::bitset<128>, int>>>> threadCombinations;
        threadCombinations.resize(startPoints.size(), combinations);
        std::vector<std::vector<int>> threadAllCombinations;
        threadAllCombinations.resize(startPoints.size(), allCombinations);

        //iterate through all possible combinations in order:
        //have 4 variables: visited nodes (int vector with capacity = # talent points), num talent points left, int vector of possible nodes to visit, weight of combination
        //weight of combination = factor of 2 for every switch talent in path
#pragma omp parallel for
        for (int i = 0; i < startPoints.size(); i++) {
            std::cout << i << "\n";
            StartPoint sp = startPoints[i];
            std::vector<std::vector<std::pair<std::bitset<128>, int>>> tcombinations;
            tcombinations.resize(talentPoints);
            std::vector<int> tallCombinations;
            tallCombinations.resize(talentPoints, 0);
            visitTalentParallel(sp.talentIndex, sp.visitedTalents, sp.currentPosTalIndex, sp.currentMultiplier, sp.talentPointsSpent, sp.talentPointsLeft, sp.possibleTalents, sortedTreeDAG, tcombinations, tallCombinations);
            threadCombinations[i] = tcombinations;
            threadAllCombinations[i] = tallCombinations;
            std::cout << i << "D\n";
        }

        std::vector<std::pair<int, int>> result(talentPoints);
        for (int i = 0; i < talentPoints; i++) {
            for (int j = 0; j < threadCombinations.size(); j++) {
                result[i].first += static_cast<int>(threadCombinations[j][i].size());
                result[i].second += threadAllCombinations[j][i];
            }
        }
        for (int i = 0; i < talentPoints; i++) {
            std::cout << "Number of configurations for " << i + 1 << " talent points without switch talents: " << result[i].first << " and with : " << result[i].second << std::endl;
        }
        return threadCombinations;
    }

    std::vector<StartPoint> getStartPoints(const TreeDAGInfo& sortedTreeDAG, int talentPointsLeft, int numThreads) {
        std::deque<StartPoint> spQ;

        std::vector<int> possibleTalents;
        for (auto& root : sortedTreeDAG.rootIndices) {
            possibleTalents.push_back(root);
        }
        for (int i = 0; i < possibleTalents.size(); i++) {
            if (sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired == 0) {
                spQ.push_back({ possibleTalents[i], 0, i + 1, 1, 0, talentPointsLeft, possibleTalents });
            }
        }

        while (spQ.size() < numThreads) {
            StartPoint spQf = spQ.front();
            spQ.pop_front();
            setTalent(spQf.visitedTalents, spQf.talentIndex);
            //add all possible children to the set for iteration
            for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[spQf.talentIndex].size(); i++) {
                insert_into_vector(spQf.possibleTalents, sortedTreeDAG.minimalTreeDAG[spQf.talentIndex][i]);
            }
            for (int i = static_cast<int>(spQf.possibleTalents.size() - 1); i >= spQf.currentPosTalIndex; i--) {
                StartPoint spC = {
                    spQf.possibleTalents[i],
                    spQf.visitedTalents,
                    i + 1,
                    spQf.currentMultiplier * sortedTreeDAG.minimalTreeDAG[spQf.talentIndex][0],
                    spQf.talentPointsSpent + 1,
                    spQf.talentPointsLeft - 1,
                    spQf.possibleTalents
                };
                spQ.push_back(spC);
            }
        }
        
        return {spQ.begin(), spQ.end()};
    }    

    /*
    Parallel version of recursive talent visitation that does not early stop and keeps track of all paths shorter than max path length.
    */
    void visitTalentParallel(
        int talentIndex,
        std::bitset<128> visitedTalents,
        int currentPosTalIndex,
        int currentMultiplier,
        int talentPointsSpent,
        int talentPointsLeft,
        std::vector<int> possibleTalents,
        const TreeDAGInfo& sortedTreeDAG,
        std::vector < std::vector < std::pair< std::bitset<128>, int>>> &combinations,
        std::vector<int>& allCombinations
    ) {
        /*
        for each node visited add child nodes(in DAG array) to the vector of possible nodesand reduce talent points left
        check if talent points left == 0 (finish) or num_nodes - current_node < talent_points_left (check for off by one error) (cancel cause talent tree can't be filled)
        iterate through all nodes in vector possible nodes to visit but only visit nodes whose index > current index
        if finished perform bit shift on uint64 to get unique tree index and put it in configuration set
        */
        //do combination housekeeping
        setTalent(visitedTalents, talentIndex);
        talentPointsSpent += 1;
        talentPointsLeft -= 1;
        currentMultiplier *= sortedTreeDAG.minimalTreeDAG[talentIndex][0];

        combinations[talentPointsSpent - 1].push_back(std::pair<std::bitset<128>, int>(visitedTalents, currentMultiplier));
        allCombinations[talentPointsSpent - 1] += currentMultiplier;
        if (talentPointsLeft == 0)
            return;

        //add all possible children to the set for iteration
        for (int i = 1; i < sortedTreeDAG.minimalTreeDAG[talentIndex].size(); i++) {
            insert_into_vector(possibleTalents, sortedTreeDAG.minimalTreeDAG[talentIndex][i]);
        }
        //visit all possible children while keeping correct order
        for (int i = currentPosTalIndex; i < possibleTalents.size(); i++) {
            //check order is correct and if talentPointsSpent is >= next talent points required
            if (possibleTalents[i] > talentIndex &&
                talentPointsSpent >= sortedTreeDAG.sortedTalents[possibleTalents[i]]->pointsRequired) {
                visitTalentParallel(possibleTalents[i], visitedTalents, i + 1, currentMultiplier, talentPointsSpent, talentPointsLeft, possibleTalents, sortedTreeDAG, combinations, allCombinations);
            }
        }
    }


    /*
    Transforms tree with "complex" talents (that can hold mutliple skill points) to "simple" tree with only talents that can hold a single talent point
    */
    void expandTreeTalents(TalentTree& tree) {
        for (auto& root : tree.talentRoots) {
            expandTalentAndAdvance(root);
        }
    }

    /*
    Creates all the necessary single point talents to replace a multi point talent and inserts them with correct parents/children
    */
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
                //talentParts[talent->maxPoints - 1]->children = originalChildren;
                talentParts[talent->maxPoints - 1]->children.push_back(child);
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

    /*
    Transforms tree with "simple" talents (that can only hold one skill point) to "complex" tree with multi-skill-point talents
    */
    void contractTreeTalents(TalentTree& tree) {
        for (auto& root : tree.talentRoots) {
            contractTalentAndAdvance(root);
        }
    }

    /*
    Creates all the necessary multi point talents to replace a single point talent and inserts them with correct parents/children
    */
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
            while (splitString(childTalent->index, "_")[0] == baseIndex) {
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
            t->maxPoints = static_cast<int>(talentParts.size());
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

    /*
    Creates a minimal representation of the TalentTree object as a vector of integer vectors, where each vector has informations about a talent such as switch multiplier
    and child nodes. Wow talent trees are essentially DAGs and can therefore be topologically sorted. TreeDAGInfo contains the minimal tree representation,
    a vector of raw Talents which correspond to the indices in the min. tree rep. and indices of root talents.
    */
    TreeDAGInfo createSortedMinimalDAG(TalentTree tree) {
        //Note: Guarantees that the tree is sorted from left to right first, top to bottom second with each layer of the tree guaranteed to have a lower index than
        //the following layer. This makes it possible to implement min. talent points required for a layer to unlock, checked while iterating in visitTalent, while keeping
        //the algorithm the same and improving speed.
        //note: EVEN THOUGH TREE IS PASSED BY COPY ALL PARENTS WILL BE DELETED FROM TALENTS!
        TreeDAGInfo info;
        for (int i = 0; i < tree.talentRoots.size(); i++) {
            info.rootIndices.push_back(i);
        }
        //Original Kahn's algorithm description from https://en.wikipedia.org/wiki/Topological_sorting
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
        std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](std::shared_ptr<Talent> a, std::shared_ptr<Talent> b) {
            return a->pointsRequired > b->pointsRequired;
            });
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
                    if (i != m->parents.end()) {
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
                    std::sort(tree.talentRoots.begin(), tree.talentRoots.end(), [](std::shared_ptr<Talent> a, std::shared_ptr<Talent> b) {
                        return a->pointsRequired > b->pointsRequired;
                        });
                }
            }
        }

        //convert sorted talents to minimalTreeDAG representation (raw talents -> integer index vectors)
        for (auto& talent : info.sortedTalents) {
            std::vector<int> child_indices(talent->children.size() + 1);
            child_indices[0] = talent->type == TalentType::SWITCH ? 2 : 1;
            for (int i = 0; i < talent->children.size(); i++) {
                ptrdiff_t pos = std::distance(info.sortedTalents.begin(), std::find(info.sortedTalents.begin(), info.sortedTalents.end(), talent->children[i]));
                if (pos >= static_cast<int>(info.sortedTalents.size())) {
                    throw std::logic_error("child does not appear in info.sortedTalents");
                }
                child_indices[i + 1] = static_cast<int>(pos);
            }
            info.minimalTreeDAG.push_back(child_indices);
        }

        return info;
    }

    /*
    Helper function to set a talent as selected (simple bit flip function)
    */
    inline void setTalent(std::bitset<128>& talent, int index) {
        talent |= 1ULL << index;
    }

    /*
    Debug function to compare the combinations of the slow legacy method with the fast counting for error checking purposes.
    Creates two files that hold all combinations in the string representation after sorting to make file diff easy and quick.
    */
    void compareCombinations(const std::unordered_map<std::bitset<128>, int>& fastCombinations, const std::unordered_set<std::string>& slowCombinations, std::string suffix) {
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

    /*
    Recreates a full multi point talents TalentTree based on a uint64 index that holds selected talents. Has the option to filter out trees and visualize them.
    */
    std::string fillOutTreeWithBinaryIndexToString(std::bitset<128> comb, TalentTree tree, TreeDAGInfo treeDAG) {
        bool filterFlag = false;
        for (int i = 0; i < 64; i++) {
            //check if bit is on
            bool bitSet = comb[i];
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
        if (filterFlag)
            visualizeTree(tree, "7points_E2_" + comb.to_string());

        return getTalentString(tree);
    }

    void insert_into_vector(std::vector<int>& v, const int& t) {
        std::vector<int>::iterator i = std::lower_bound(v.begin(), v.end(), t);
        if (i == v.end() || t < *i)
            v.insert(i, t);
    }

    int* convertMinimalTreeDAGToArray(TreeDAGInfo& DAG) {
        auto& mDAG = DAG.minimalTreeDAG;
        int nodeCount = static_cast<int>(mDAG.size());
        int connectionCount = 0;
        for (int i = 0; i < nodeCount; i++) {
            connectionCount += static_cast<int>(mDAG[i].size());
        }
        int* arr = (int*)malloc((nodeCount + connectionCount) * sizeof(int));
        if (!arr) {
            return nullptr;
        }
        int shift = 0;
        for (int i = 0; i < nodeCount; i++) {
            int currShift = static_cast<int>(mDAG[i].size());
            *(arr + i) = nodeCount + shift;
            for (int j = 0; j < mDAG[i].size(); j++) {
                *(arr + nodeCount + shift + j) = mDAG[i][j];
            }
            shift += currShift;
        }
        return arr;
    }

    inline int getValueFromMDAGArray(int* arr, int i1, int i2) {
        return arr[arr[i1] + i2];
    }

    inline int getConnectionCountFromMDAGArray(int* mDAG, int talentIndex) {
        if (talentIndex >= (*mDAG) - 1) {
            //last connection has to have 0 connections but 1 multiplier
            return 1;
        }
        return *(mDAG + talentIndex + 1) - *(mDAG + talentIndex);
    }

    int* convertMinimalTreeDAGToPtsReqArray(TreeDAGInfo& DAG) {
        int nodeCount = static_cast<int>(DAG.sortedTalents.size());
        int* arr = (int*)malloc(nodeCount * sizeof(int));
        if (!arr) {
            return arr;
        }
        for (int i = 0; i < nodeCount; i++) {
            *(arr + i) = DAG.sortedTalents[i]->pointsRequired;
        }
        return arr;
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
}
