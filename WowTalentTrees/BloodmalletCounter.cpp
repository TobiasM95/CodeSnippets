#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>
#include <memory>
#include <iostream>
#include <chrono>

#include "BloodmalletCounter.h"

namespace bloodmallet {

    struct Talent {
        std::string name = "";
        TalentType type = TalentType::ABILITY;
        int requiredPoints = 0;
        std::vector<std::string> parentNames;
        std::vector<std::string> childrenNames;
        std::vector<std::string> siblingNames;

        int index = -1;
        std::vector<std::shared_ptr<Talent>> parents;
        std::vector<std::shared_ptr<Talent>> children;
        std::vector<std::shared_ptr<Talent>> siblings;

        bool isInitialized() {
            return (
                this->index > -1
                && this->parentNames.size() == this->parents.size()
                && this->childrenNames.size() == this->children.size()
                && this->siblingNames.size() == this->siblings.size());
        }

        bool isSelected(std::vector<bool> tree) {
            return tree[this->index];
        }

        bool hasSelectedChildren(std::vector<bool> tree) {
            if (this->children.size() == 0)
                return false;
            for (auto& talent : this->children) {
                if (talent->isSelected(tree))
                    return true;
            }
            return false;
        }

        bool hasSelectedParents(std::vector<bool> tree) {
            if (this->parents.size() == 0)
                return false;
            for (auto& talent : this->parents) {
                if (talent->isSelected(tree))
                    return true;
            }
            return false;
        }

        bool hasSelectedSiblings(std::vector<bool> tree) {
            if (this->siblings.size() == 0)
                return false;
            for (auto& talent : this->siblings) {
                if (talent->isSelected(tree))
                    return true;
            }
            return false;
        }

        bool isGateSatisfied(std::vector<bool> tree) {
            if (this->requiredPoints < 1)
                return true;
            int selectedTalents = 0;
            for (bool t : tree) {
                selectedTalents += static_cast<int>(t);
            }
            return selectedTalents >= this->requiredPoints;
        }

        std::vector<bool> select(std::vector<bool> tree) {
            if (this->isSelected(tree))
                throw std::logic_error("Node already selected!");
            if (!this->isGateSatisfied(tree))
                throw std::logic_error("Not enough points spent!");
            std::vector<bool> newTree(tree.begin(), tree.end());
            newTree[this->index] = true;
            return newTree;
        }

        static std::vector<std::shared_ptr<Talent>> createRanks(
            std::string name,
            TalentType type,
            int maxRank,
            int requiredPoints,
            std::vector<std::string> parentNames,
            std::vector<std::string> childrenNames
        ) {
            std::vector<std::shared_ptr<Talent>> talents;
            for (int rank = 1; rank < maxRank + 1; rank++) {
                if (rank == 1) {
                    std::vector<std::string> sibling;
                    std::vector<std::string> childName;
                    childName.push_back(name + std::to_string(rank + 1));
                    std::shared_ptr<Talent> t = createTalent(
                        name + std::to_string(rank),
                        type,
                        requiredPoints,
                        parentNames,
                        childName,
                        sibling
                    );
                    talents.push_back(t);
                }
                else if (rank == maxRank) {
                    std::vector<std::string> sibling;
                    std::vector<std::string> parentName;
                    parentName.push_back(name + std::to_string(rank - 1));
                    std::shared_ptr<Talent> t = createTalent(
                        name + std::to_string(rank),
                        type,
                        requiredPoints,
                        parentName,
                        childrenNames,
                        sibling
                    );
                    talents.push_back(t);
                }
                else {
                    std::vector<std::string> sibling;
                    std::vector<std::string> childName;
                    childName.push_back(name + std::to_string(rank + 1));
                    std::vector<std::string> parentName;
                    parentName.push_back(name + std::to_string(rank - 1));
                    std::shared_ptr<Talent> t = createTalent(
                        name + std::to_string(rank),
                        type,
                        requiredPoints,
                        parentName,
                        childName,
                        sibling
                    );
                    talents.push_back(t);
                }
            }
            return talents;
        }

    };

    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type,
        int requiredPoints,
        std::vector<std::string> parentNames,
        std::vector<std::string> childrenNames,
        std::vector<std::string> siblingNames) {
        std::shared_ptr<Talent> t = std::make_shared<Talent>();
        t->name = name;
        t->type = type;
        t->requiredPoints = requiredPoints;
        t->parentNames = parentNames;
        t->childrenNames = childrenNames;
        t->siblingNames = siblingNames;

        t->index = -1;

        return t;
    }

    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type) {
        return createTalent(name, type, 0, std::vector<std::string>(), std::vector<std::string>(), std::vector<std::string>());
    }

    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type,
        std::vector<std::string> parentNames) {
        return createTalent(name, type, 0, parentNames, std::vector<std::string>(), std::vector<std::string>());
    }

    std::shared_ptr<Talent> createTalent(std::shared_ptr<Talent>& t) {
        return createTalent(
            t->name,
            t->type,
            t->requiredPoints,
            t->parentNames,
            t->childrenNames,
            t->siblingNames
        );
    }

    std::vector<std::shared_ptr<Talent>> _talent_post_init(std::vector<std::shared_ptr<Talent>>& talents) {
        std::unordered_map<std::string, std::shared_ptr<Talent>> tDict;
        for (auto& t : talents) {
            tDict[t->name] = t;
        }

        for (int index = 0; index < talents.size(); index++) {
            for (std::string name : talents[index]->parentNames) {
                std::vector<std::string> talentParentNames;
                for (auto& tPair : tDict) {
                    if (tPair.first.find(name) != std::string::npos) {
                        talentParentNames.push_back(tPair.first);
                    }
                }
                std::sort(talentParentNames.begin(), talentParentNames.end());
                talents[index]->parents.push_back(tDict[talentParentNames[talentParentNames.size() - 1]]);

                if (std::find(
                    tDict[talentParentNames[talentParentNames.size() - 1]]->childrenNames.begin(),
                    tDict[talentParentNames[talentParentNames.size() - 1]]->childrenNames.end(),
                    talents[index]->name)
                    == tDict[talentParentNames[talentParentNames.size() - 1]]->childrenNames.end()) {
                    tDict[talentParentNames[talentParentNames.size() - 1]]->childrenNames.push_back(talents[index]->name);
                }
                if (std::find(
                    tDict[talentParentNames[talentParentNames.size() - 1]]->children.begin(),
                    tDict[talentParentNames[talentParentNames.size() - 1]]->children.end(),
                    talents[index])
                    == tDict[talentParentNames[talentParentNames.size() - 1]]->children.end()) {
                    tDict[talentParentNames[talentParentNames.size() - 1]]->children.push_back(talents[index]);
                }
            }
            for (std::string name : talents[index]->siblingNames) {
                talents[index]->siblings.push_back(tDict[name]);
            }
            talents[index]->index = index;
        }
        for (auto& t : talents) {
            if (!t->isInitialized())
                throw std::logic_error("Talent wasn't initialized!");
        }
        return talents;
    }

    std::vector<std::shared_ptr<Talent>> _create_talents() {
        class HelperTalents {
        public:
            std::vector<std::shared_ptr<Talent>> talents;

            void append(std::shared_ptr<Talent> t) {
                this->talents.push_back(t);
            }

            void append(std::vector<std::shared_ptr<Talent>> v) {
                for (auto& t : v) {
                    this->talents.push_back(t);
                }
            }
        };

        HelperTalents talents = HelperTalents();

        std::vector<std::shared_ptr<Talent>> v;
        std::shared_ptr<Talent> t = createTalent("A1", TalentType::ABILITY);
        talents.append(t);
        t = createTalent("B1", TalentType::ABILITY, std::vector<std::string>{"A1"});
        talents.append(t);
        v = Talent::createRanks("B2", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"A1"}, std::vector<std::string>());
        talents.append(v);
        t = createTalent("B3", TalentType::PASSIVE, std::vector<std::string>{"A1"});
        talents.append(t);
        t = createTalent("C1", TalentType::ABILITY, std::vector<std::string>{"B1"});
        talents.append(t);
        t = createTalent("C2", TalentType::ABILITY, std::vector<std::string>{"B2"});
        talents.append(t);
        t = createTalent("C3", TalentType::ABILITY, std::vector<std::string>{"B3"});
        talents.append(t);
        v = Talent::createRanks("D1", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"C1"}, std::vector<std::string>());
        talents.append(v);
        v = Talent::createRanks("D2", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"C3"}, std::vector<std::string>());
        talents.append(v);
        v = Talent::createRanks("D3", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"C3"}, std::vector<std::string>());
        talents.append(v);
        v = Talent::createRanks("E1", TalentType::PASSIVE, 3, 0, std::vector<std::string>{"C1"}, std::vector<std::string>());
        talents.append(v);
        t = createTalent("E2", TalentType::CHOICE, 0, std::vector<std::string>{"D1", "D2"}, std::vector<std::string>(), std::vector<std::string>{"E3"});
        talents.append(t);
        t = createTalent("E3", TalentType::CHOICE, 0, std::vector<std::string>{"D1", "D2"}, std::vector<std::string>(), std::vector<std::string>{"E2"});
        talents.append(t);
        t = createTalent("E4", TalentType::PASSIVE, 0, std::vector<std::string>{"C3"}, std::vector<std::string>(), std::vector<std::string>());
        talents.append(t);
        t = createTalent("F1", TalentType::PASSIVE, std::vector<std::string>{"E1"});
        talents.append(t);
        v = Talent::createRanks("F2", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"E2", "E3"}, std::vector<std::string>());
        talents.append(v);
        v = Talent::createRanks("F3", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"E2", "E3"}, std::vector<std::string>());
        talents.append(v);
        t = createTalent("F4", TalentType::PASSIVE, std::vector<std::string>{"E4"});
        talents.append(t);
        t = createTalent("G1", TalentType::CHOICE, 0, std::vector<std::string>{"F1", "F2"}, std::vector<std::string>(), std::vector<std::string>{"G2"});
        talents.append(t);
        t = createTalent("G2", TalentType::CHOICE, 0, std::vector<std::string>{"F1", "F2"}, std::vector<std::string>(), std::vector<std::string>{"G1"});
        talents.append(t);
        t = createTalent("G3", TalentType::PASSIVE, std::vector<std::string>{"F3", "F4"});
        talents.append(t);
        v = Talent::createRanks("G4", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"F4"}, std::vector<std::string>());
        talents.append(v);
        t = createTalent("H1", TalentType::CHOICE, 0, std::vector<std::string>{"F1"}, std::vector<std::string>(), std::vector<std::string>{"H2"});
        talents.append(t);
        t = createTalent("H2", TalentType::CHOICE, 0, std::vector<std::string>{"F1"}, std::vector<std::string>(), std::vector<std::string>{"H1"});
        talents.append(t);
        t = createTalent("H3", TalentType::PASSIVE, std::vector<std::string>{"G1", "G2", "G3"});
        talents.append(t);
        t = createTalent("H4", TalentType::ABILITY, std::vector<std::string>{"G4"});
        talents.append(t);
        t = createTalent("I1", TalentType::PASSIVE, std::vector<std::string>{"H1", "H2"});
        talents.append(t);
        t = createTalent("I2", TalentType::PASSIVE, std::vector<std::string>{"H1", "H2"});
        talents.append(t);
        v = Talent::createRanks("I3", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"H1", "H2", "H3"}, std::vector<std::string>());
        talents.append(v);
        v = Talent::createRanks("I4", TalentType::PASSIVE, 2, 0, std::vector<std::string>{"H3", "H4"}, std::vector<std::string>());
        talents.append(v);
        t = createTalent("I5", TalentType::PASSIVE, std::vector<std::string>{"H4"});
        talents.append(t);
        t = createTalent("J1", TalentType::CHOICE, 0, std::vector<std::string>{"I1"}, std::vector<std::string>(), std::vector<std::string>{"J2"});
        talents.append(t);
        t = createTalent("J2", TalentType::CHOICE, 0, std::vector<std::string>{"I1"}, std::vector<std::string>(), std::vector<std::string>{"J1"});
        talents.append(t);
        t = createTalent("J3", TalentType::CHOICE, 0, std::vector<std::string>{"I3", "I4"}, std::vector<std::string>(), std::vector<std::string>{"J4"});
        talents.append(t);
        t = createTalent("J4", TalentType::CHOICE, 0, std::vector<std::string>{"I3", "I4"}, std::vector<std::string>(), std::vector<std::string>{"J3"});
        talents.append(t);
        t = createTalent("J5", TalentType::CHOICE, 0, std::vector<std::string>{"I5"}, std::vector<std::string>(), std::vector<std::string>{"J6"});
        talents.append(t);
        t = createTalent("J6", TalentType::CHOICE, 0, std::vector<std::string>{"I5"}, std::vector<std::string>(), std::vector<std::string>{"J5"});
        talents.append(t);

        return talents.talents;
    }



    //TALENTS: typing.Tuple[Talent, ...] = _talent_post_init(_create_talents())

    std::vector<std::shared_ptr<Talent>> removeChoices(std::vector<std::shared_ptr<Talent>> talents) {
        std::vector<std::shared_ptr<Talent>> singleChoiced;
        std::vector<std::string> ignoredNodes;
        for (auto& t : talents) {
            bool flag = false;
            for (auto& sibling : t->siblingNames) {
                if (t->name >= sibling) {
                    flag = true;
                }
            }
            if (!flag) {
                singleChoiced.push_back(createTalent(t));
            }
            else {
                ignoredNodes.push_back(t->name);
            }
        }
        for (auto& t : singleChoiced) {
            t->siblingNames.clear();
            t->siblings.clear();
            t->parents.clear();
            t->children.clear();
            t->childrenNames.clear();
            std::vector<std::string> newParentNames;
            for (auto& p : t->parentNames) {
                if (std::find(
                    ignoredNodes.begin(),
                    ignoredNodes.end(),
                    p)
                    == ignoredNodes.end()) {
                    newParentNames.push_back(p);
                }
            }
            t->parentNames = newParentNames;
        }
        return _talent_post_init(singleChoiced);
    }

    std::vector<std::vector<bool>> readdChoices(std::vector<std::shared_ptr<Talent>> talents, std::vector<std::shared_ptr<Talent>> singleChoiceTalents, std::vector<std::vector<bool>> paths) {
        std::unordered_map<std::shared_ptr<Talent>, std::shared_ptr<Talent>> noChoiceToTalentMap;
        for (auto& n : singleChoiceTalents) {
            for (auto& t : talents) {
                if (n->name == t->name) {
                    noChoiceToTalentMap[n] = t;
                }
            }
        }
        std::unordered_map<std::shared_ptr<Talent>, std::vector<std::shared_ptr<Talent>>> _originalChoices;
        for (auto& n : talents) {
            if (n->siblingNames.size() > 0) {
                std::vector<std::shared_ptr<Talent>> siblings;
                siblings.push_back(n);
                for (auto& s : n->siblings) {
                    siblings.push_back(s);
                }
                _originalChoices[n] = siblings;
            }
        }

        std::unordered_map<std::shared_ptr<Talent>, std::vector<std::shared_ptr<Talent>>> preparedChoices;
        for (auto& n : singleChoiceTalents) {
            for (auto& c : _originalChoices) {
                if (n->name == c.first->name) {
                    preparedChoices[n] = _originalChoices[c.first];
                }
            }
        }

        std::vector<std::vector<bool>> trees;
        for (auto& path : paths) {
            std::unordered_map<std::shared_ptr<Talent>, std::vector<std::shared_ptr<Talent>>> includedChoiceNodes;
            for (auto& nv : preparedChoices) {
                if (nv.first->isSelected(path)) {
                    includedChoiceNodes[nv.first] = nv.second;
                }
            }
            std::vector<bool> blueprint = std::vector<bool>(talents.size());
            for (auto& talent : singleChoiceTalents) {
                if (talent->isSelected(path) && includedChoiceNodes.count(talent) == 0) {
                    blueprint[noChoiceToTalentMap[talent]->index] = true;
                }
            }

            std::vector<std::vector<std::shared_ptr<Talent>>> includedChoiceNodesVector;
            for (auto& cn : includedChoiceNodes) {
                includedChoiceNodesVector.push_back(cn.second);
            }
            std::vector <std::vector<std::shared_ptr<Talent> >> choiceCombinations = cart_product(includedChoiceNodesVector);
            for (auto& combination : choiceCombinations) {
                std::vector<bool> localCopy = std::vector<bool>();
                for (auto b : blueprint) {
                    localCopy.push_back(b);
                }
                for (auto& talent : combination) {
                    localCopy = talent->select(localCopy);
                }
                trees.push_back(localCopy);
            }
        }

        return trees;
    }

    std::vector<std::vector<std::shared_ptr<Talent>>> cart_product(std::vector<std::vector<std::shared_ptr<Talent>>>& v) {
        std::vector<std::vector<std::shared_ptr<Talent>>> s = { {} };
        for (auto& u : v) {
            std::vector<std::vector<std::shared_ptr<Talent>>> r;
            for (const auto& x : s) {
                for (auto y : u) {
                    r.push_back(x);
                    r.back().push_back(y);
                }
            }
            s = move(r);
        }
        return s;
    }

    std::vector<std::vector<bool>> igrow(std::vector<std::shared_ptr<Talent>> talents, int points) {
        std::vector<std::shared_ptr<Talent>> preparedTalents = removeChoices(talents);
        std::vector<bool> emptyPath;
        emptyPath.resize(preparedTalents.size(), false);
        std::unordered_map<std::vector<bool>, std::unordered_set<std::shared_ptr<Talent>>> existingPaths;
        for (auto& t : preparedTalents) {
            if (t->parents.size() == 0)
                existingPaths[emptyPath].insert(t);
        }

        for (int investedPoints = 0; investedPoints < points + 1; investedPoints++) {
            //std::cout << std::endl << investedPoints << ":" << std::endl << "___________" << std::endl;
            std::unordered_map<std::vector<bool>, std::unordered_set<std::shared_ptr<Talent>>> newPaths;
            for (auto pEP : existingPaths) {
                //std::cout << pEP.second.size() << std::endl;
                //for (int i = 0; i < pEP.first.size(); i++) {
                //    if (pEP.first[i])
                //        std::cout << i << " - ";
                //}
                //std::cout << std::endl;
                for (auto talent : pEP.second) {
                    //std::cout << talent->name << std::endl;
                    std::vector<bool> newPath;
                    try {
                        newPath = talent->select(pEP.first);
                        if (!newPaths.count(newPath)) {
                            std::unordered_set<std::shared_ptr<Talent>> newEntryPoints(pEP.second.begin(), pEP.second.end());
                            newEntryPoints.erase(talent);
                            for (auto& child : talent->children) {
                                newEntryPoints.insert(child);
                            }
                            newPaths[newPath] = newEntryPoints;
                        }
                    }
                    catch (std::exception& e) {
                        std::cout << e.what() << std::endl;
                        continue;
                    }
                }
            }
            existingPaths = newPaths;
        }

        std::vector<std::vector<bool>> ePK;
        for (auto& ep : existingPaths) {
            ePK.push_back(ep.first);
        }
        //std::cout << "Before unpacking: " << ePK.size() << std::endl;
        auto t1 = std::chrono::high_resolution_clock::now();
        std::vector<std::vector<bool>> trees = readdChoices(talents, preparedTalents, ePK);
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        std::cout << "Unpack time: " << ms_double.count() << " ms" << std::endl;
        return trees;
    }
}
