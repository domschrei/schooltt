
#pragma once

#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#define SUBJECT_LABEL_FREETIME "__free"

// A taught subject, e.g., "English", "Maths", SUBJECT_LABEL_FREETIME
struct Subject {
    std::string label;
    bool operator<(const Subject& other) const {return this->label < other.label;}
    bool operator==(const Subject& other) const {return this->label == other.label;}
    bool operator!=(const Subject& other) const {return this->label != other.label;}
};

// A room, e.g., identified by number or some letter-number combination
struct Room {
    std::string label;
    bool operator<(const Room& other) const {return this->label < other.label;}
    bool operator==(const Room& other) const {return this->label == other.label;}
    bool operator!=(const Room& other) const {return this->label != other.label;}
};

// A map of each subject to a list of rooms where the subject can be taught.
struct RoomsBySubject {
    std::map<Subject, std::vector<Room>> map;
    std::vector<Room> getSuitableRooms(const std::vector<Subject>& subjects) const {
        std::set<Room> out;
        for (auto& subj : subjects)
            if (map.count(subj)) out.insert(map.at(subj).begin(), map.at(subj).end());
        return {out.begin(), out.end()};
    }
};

// Concrete subject that some class needs to be taught, e.g., four week-hours of English level 5.
struct Course {
    Subject subject;
    int level;
    float minUnitsPerWeek;
    float maxUnitsPerWeek;

    // ignore these fields for now
    int minFlatSlots;
    int maxFlatSlots;

    bool operator<(const Course& other) const {return this->subject < other.subject;}
    bool operator==(const Course& other) const {return this->subject == other.subject;}
    bool operator!=(const Course& other) const {return this->subject != other.subject;}
};

// Concrete class, e.g., "7b", with a list of course obligations.
struct Class {
    std::string label;
    std::vector<Course> courses;

    bool operator<(const Class& other) const {return this->label < other.label;}
    bool operator==(const Class& other) const {return this->label == other.label;}
    bool operator!=(const Class& other) const {return this->label != other.label;}
};

// Natural teaching person
struct Teacher {
    std::string name;
    // map each subject to a range (or set) of levels they can teach
    std::map<Subject, std::set<int>> capabilities;

    bool isEligible(const Course& course) const {
        auto it = capabilities.find(course.subject);
        if (it == capabilities.end()) return false;
        return it->second.count(course.level);
    }
    std::vector<Subject> getTaughtSubjects() const {
        std::set<Subject> set;
        for (auto& [subj, levels] : capabilities) set.insert(subj);
        return std::vector<Subject> {set.begin(), set.end()};
    }
    bool operator<(const Teacher& other) const {return this->name < other.name;}
    bool operator==(const Teacher& other) const {return this->name == other.name;}
    bool operator!=(const Teacher& other) const {return this->name != other.name;}
};

// Time slot in a certain day, marked by beginning and end
struct Slot {
    float begin;
    float end;
    float duration() const {return end-begin;}
    std::string toStr() const {
        std::string out;
        for (float t : {begin, end}) {
            const int tHrs = (int) t;
            const int tMin = (int) (60 * (t - (int) t));
            std::string timeStr = (tHrs < 10 ? "0" : "") + std::to_string(tHrs)
                + (tMin < 10 ? ":0" : ":") + std::to_string(tMin);
            out += timeStr + " - ";
        }
        return out.substr(0, out.size() - 3);
    }
    bool operator<(const Slot& other) const {return this->begin < other.begin || (this->begin == other.begin && this->end < other.end);}
    bool operator==(const Slot& other) const {return !(*this < other) && !(other < *this);}
    bool operator!=(const Slot& other) const {return !(*this == other);}
};

// A single day (e.g., "Monday") with a list of time slots.
struct Day {
    std::string label;
    std::vector<Slot> slots;
    bool operator<(const Day& other) const {return this->label < other.label;}
    bool operator==(const Day& other) const {return this->label == other.label;}
    bool operator!=(const Day& other) const {return this->label != other.label;}
};

// The entire timetabling problem.
struct TimetablingProblem {
    RoomsBySubject roomsBySubject;
    std::vector<Teacher> teachers;
    std::vector<Class> classes;
    std::vector<Day> days;
    // what is the common base duration of which each slot is a multiple, e.g., 0.75 (= 45 minutes)
    float unitDuration;
};

struct FlatSlot {
    int id;
    int numParts;
    bool operator<(const FlatSlot& other) const {return this->id < other.id;}
    bool operator==(const FlatSlot& other) const {return this->id == other.id;}
    bool operator!=(const FlatSlot& other) const {return this->id != other.id;}
};

struct FlatTimetablingProblem {
    RoomsBySubject roomsBySubject;
    std::vector<Teacher> teachers;
    std::vector<Class> classes;
    std::vector<Day> days;
    std::vector<FlatSlot> slots;
    std::map<FlatSlot, std::pair<Day, Slot>> slotsBackConversion;

    FlatTimetablingProblem(TimetablingProblem&& ttp) {
        roomsBySubject = std::move(ttp.roomsBySubject);
        teachers = std::move(ttp.teachers);
        classes = std::move(ttp.classes);
        days = std::move(ttp.days);
        for (auto& cla : classes) for (auto& course : cla.courses) {
            course.minFlatSlots = (int) std::round(course.minUnitsPerWeek);
            course.maxFlatSlots = (int) std::round(course.maxUnitsPerWeek);
        }
        int flatSlotId = 1;
        for (auto& day : days) for (auto& slot : day.slots) {
            FlatSlot flatSlot({flatSlotId++, (int) std::round(slot.duration() / ttp.unitDuration)});
            slots.push_back(flatSlot);
            slotsBackConversion[flatSlot] = {day, slot};
        }
    }
};

struct Timetable {
    FlatTimetablingProblem fttp;
    std::map<FlatSlot, std::map<Room, std::tuple<Teacher, Class, Course>>> map;
    std::string toStr() const {
        std::string out;
        for (auto& [flatSlot, roomMap] : map) {
            auto [day, slot] = fttp.slotsBackConversion.at(flatSlot);
            out += day.label + ", " + std::to_string(slot.begin) + " - " + std::to_string(slot.end) + ":\n";
            for (auto& [room, tcc] : roomMap) {
                out += "  " + room.label + ": ";
                auto [teacher, cla, course] = tcc;
                out += teacher.name + " -> " + cla.label + " - " + course.subject.label + "\n";
            }
        }
        return out;
    }
    std::vector<std::vector<std::string>> formatTimetable(const std::map<Day, std::map<FlatSlot, std::string>>& ttPerDay,
            const std::set<Slot>& sortedSlots) const {

        std::vector<std::vector<std::string>> table;

        // Header row
        table.push_back({});
        table.back().push_back("Time");
        for (auto& day : fttp.days) {
            table.back().push_back(day.label);
        }
        // Content rows
        for (auto& slot : sortedSlots) {
            table.push_back({});
            table.back().push_back(slot.toStr());
            for (auto& day : fttp.days) {
                if (ttPerDay.count(day)) {
                    bool found = false;
                    for (auto& [flatSlot, str] : ttPerDay.at(day)) {
                        if (fttp.slotsBackConversion.at(flatSlot).second == slot) {
                            table.back().push_back(str);
                            found = true;
                            break;
                        }
                    }
                    if (!found) table.back().push_back("");
                } else table.back().push_back("");
            }
        }
        return table;
    }
    std::vector<std::vector<std::string>> formatTimetableFor(const Class& cla) const {

        std::map<Day, std::map<FlatSlot, std::string>> ttPerDay;
        std::set<Slot> sortedSlots;
        for (auto& [flatSlot, roomMap] : map) {
            auto [day, slot] = fttp.slotsBackConversion.at(flatSlot);
            sortedSlots.insert(slot);
            ttPerDay[day][flatSlot] = "";
            for (auto& [room, tcc] : roomMap) {
                auto& [t, cl, course] = tcc;
                if (cl != cla) continue;
                auto str = course.subject.label + " (" + t.name + ") - " + room.label;
                ttPerDay[day][flatSlot] = str;
                break;
            }
        }
        return formatTimetable(ttPerDay, sortedSlots);
    }
    std::vector<std::vector<std::string>> formatTimetableFor(const Teacher& teacher) const {

        std::map<Day, std::map<FlatSlot, std::string>> ttPerDay;
        std::set<Slot> sortedSlots;
        for (auto& [flatSlot, roomMap] : map) {
            auto [day, slot] = fttp.slotsBackConversion.at(flatSlot);
            sortedSlots.insert(slot);
            ttPerDay[day][flatSlot] = "";
            for (auto& [room, tcc] : roomMap) {
                auto& [t, cl, course] = tcc;
                if (t != teacher) continue;
                auto str = course.subject.label + " (" + cl.label + ") - " + room.label;
                ttPerDay[day][flatSlot] = str;
                break;
            }
        }
        return formatTimetable(ttPerDay, sortedSlots);
    }
};
