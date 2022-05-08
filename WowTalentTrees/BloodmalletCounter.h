#pragma once

namespace bloodmallet {

    enum class TalentType {
        PASSIVE, ABILITY, CHOICE
    };

	struct Talent;
    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type,
        int requiredPoints,
        std::vector<std::string> parentNames,
        std::vector<std::string> childrenNames,
        std::vector<std::string> siblingNames);
    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type);
    std::shared_ptr<Talent> createTalent(std::string name,
        TalentType type,
        std::vector<std::string> parentNames);
    std::shared_ptr<Talent> createTalent(std::shared_ptr<Talent>& t);

    std::vector<std::shared_ptr<Talent>> _talent_post_init(std::vector<std::shared_ptr<Talent>>& talents);
    std::vector<std::shared_ptr<Talent>> _create_talents();
    std::vector<std::shared_ptr<Talent>> removeChoices(std::vector<std::shared_ptr<Talent>> talents);
    std::vector<std::vector<bool>> readdChoices(std::vector<std::shared_ptr<Talent>> talents, std::vector<std::shared_ptr<Talent>> singleChoiceTalents, std::vector<std::vector<bool>> paths);
    std::vector<std::vector<std::shared_ptr<Talent>>> cart_product(std::vector<std::vector<std::shared_ptr<Talent>>>& v);
    std::vector<std::vector<bool>> igrow(std::vector<std::shared_ptr<Talent>> talents, int points);

}