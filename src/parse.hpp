
#include "model.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <algorithm>

namespace ttp {
TimetablingProblem parseTimetablingProblem(const std::string& filename) {

    TimetablingProblem problem;
    std::ifstream file(filename);
    std::string line;
    std::string currentBlock;
    std::map<std::string, Teacher> teacherMap;
    std::map<std::string, Class> classMap;

    while (std::getline(file, line)) {
        // Ignore empty lines and comments
        if (line.empty() || line[0] == '#') {
            if (line.find("#RoomsBySubject") != std::string::npos) currentBlock = "RoomsBySubject";
            else if (line.find("#Teachers") != std::string::npos) currentBlock = "Teachers";
            else if (line.find("#Classes") != std::string::npos) currentBlock = "Classes";
            else if (line.find("#Days") != std::string::npos) currentBlock = "Days";
            else if (line.find("#UnitDuration") != std::string::npos) currentBlock = "UnitDuration";
            continue;
        }

        std::istringstream iss(line);
        if (currentBlock == "RoomsBySubject") {
            // Format: Subject,Room1,Room2,...
            std::string subjectLabel, roomLabel;
            std::getline(iss, subjectLabel, ',');
            Subject subject{subjectLabel};
            while (std::getline(iss, roomLabel, ',')) {
                problem.roomsBySubject.map[subject].push_back(Room{roomLabel});
            }
        }
        else if (currentBlock == "Teachers") {
            // Format: Name,Subject,Level1,Level2,...
            std::string name, subjectLabel, levelStr;
            std::getline(iss, name, ',');
            std::getline(iss, subjectLabel, ',');
            Subject subject{subjectLabel};
            std::set<int> levels;
            while (std::getline(iss, levelStr, ',')) {
                levels.insert(std::stoi(levelStr));
            }
            auto& teacher = teacherMap[name];
            teacher.name = name;
            teacher.capabilities[subject].insert(levels.begin(), levels.end());
        }
        else if (currentBlock == "Classes") {
            // Format: ClassLabel,Subject,Level,MinHours,MaxHours
            std::string classLabel, subjectLabel, levelStr, minHoursStr, maxHoursStr;
            std::getline(iss, classLabel, ',');
            Class& cla = classMap[classLabel];
            cla.label = classLabel;
            while (iss.good()) {
                std::getline(iss, subjectLabel, ',');
                if (subjectLabel.empty()) break;
                std::getline(iss, levelStr, ',');
                std::getline(iss, minHoursStr, ',');
                std::getline(iss, maxHoursStr, ',');
                Course course{Subject{subjectLabel}, std::stoi(levelStr), std::stof(minHoursStr), std::stof(maxHoursStr), 0, 0};
                cla.courses.push_back(course);
            }
        }
        else if (currentBlock == "Days") {
            // Format: DayLabel,SlotBegin,SlotEnd,...
            std::string dayLabel, beginStr, endStr;
            std::getline(iss, dayLabel, ',');
            Day day{dayLabel};
            while (std::getline(iss, beginStr, ',')) {
                std::getline(iss, endStr, ',');
                day.slots.push_back(Slot{std::stof(beginStr), std::stof(endStr)});
            }
            problem.days.push_back(day);
        }
        else if (currentBlock == "UnitDuration") {
            // Format: UnitDurationValue
            float unitDuration;
            iss >> unitDuration;
            problem.unitDuration = unitDuration;
        }
    }
    for (auto& [name, teacher] : teacherMap) {
        problem.teachers.push_back(teacher);
    }
    for (auto& [label, cla] : classMap) {
        problem.classes.push_back(cla);
    }
    return problem;
}
};