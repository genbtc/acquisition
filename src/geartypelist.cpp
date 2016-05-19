/*
    Copyright 2014 Ilya Zhuravlev

    This file is part of Acquisition.

    Acquisition is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Acquisition is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Acquisition.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "geartypelist.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QStringList>

#include "item.h"
#include "porting.h"
#include "util.h"

// Actual list of mods is computed at runtime
QStringList gear_string_list;

// These are just summed, and the mod named as the first element of a vector is generated with value equaling the sum.
// Both implicit and explicit fields are considered.
// This is pretty much the same list as poe.trade uses
const std::vector<std::vector<std::string>> gear_simple_sum = {
		{ "Ring" },
		{ "Amulet" },
		{ "Helmet" },
		{ "Chest" },
		{ "Belt" },
		{ "Gloves" },
		{ "Boots" },
		{ "Axe" },
		{ "Claw" },
		{ "Bow" },
		{ "Dagger" },
		{ "Mace" },
		{ "Quiver" },
		{ "Sceptre" },
		{ "Staff" },
		{ "Sword" },
		{ "Shield" },
		{ "Wand" },
		{ "Flask" },
		{ "Map" },
		{ "QuestItem" },
		{ "DivinationCard" },
		{ "Jewel" },
		{ "Talisman" },
		{ "Unknown" }
};

std::vector<std::unique_ptr<SumGearGenerator>> gear_generators;

//One of the first thing main.cpp does is call InitModlist(); InitGearlist(); to populate the gear dropdown
void InitGearlist() {
	for (auto &list : gear_simple_sum) {
		gear_string_list.push_back(list[0].c_str());

		gear_generators.push_back(std::make_unique<SumGearGenerator>(list[0], list));
    }
}

//constructor:
SumGearGenerator::SumGearGenerator(const std::string &name, const std::vector<std::string> &sum) :
    name_(name),
    matches_(sum)
{}

bool SumGearGenerator::Match(const char *gear, double *output) {
    bool found = false;
    *output = 0.0;
    for (auto &match : matches_) {
        double result = 0.0;
		if (Util::MatchMod(match.c_str(), gear, &result)) {
            *output += result;
            found = true;
        }
    }

    return found;
}

void SumGearGenerator::Generate(const rapidjson::Value &json, GearTable *output) {
    bool gear_present = false;
    double sum = 0;

    for (auto &mod : json["properties"]) {
        double result;
        if (Match(mod.GetString(), &result)) {
            sum += result;
			gear_present = true;
        }
    }

	if (gear_present)
        (*output)[name_] = sum;
}
