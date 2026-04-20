
#pragma once

#include "model.hpp"
#include <cassert>
#include <fstream>
#include <initializer_list>

#define EXPLAIN_VARS false

class Encoding {

private:
    const RoomsBySubject& _rooms_by_subject;
    const std::vector<Teacher>& _teachers;
    const std::vector<Class>& _classes;
    const std::vector<FlatSlot>& _slots;

    std::vector<int> _out_lits;
    int _nb_vars {0};
    int _nb_cls {0};

    int _running_var {1};

    struct CmpTeacherAtRoomSlot {
        bool operator()(const std::tuple<Teacher, Room, FlatSlot>& left, const std::tuple<Teacher, Room, FlatSlot>& right) const {
            auto [t, r, s] = left;
            auto [t2, r2, s2] = right;
            if (t != t2) return t < t2;
            if (r != r2) return r < r2;
            if (s < s2) return s < s2;
            return false;
        }
    };
    struct CmpClassAtRoomSlot {
        bool operator()(const std::tuple<Class, Room, FlatSlot>& left, const std::tuple<Class, Room, FlatSlot>& right) const {
            auto [t, r, s] = left;
            auto [t2, r2, s2] = right;
            if (t != t2) return t < t2;
            if (r != r2) return r < r2;
            if (s < s2) return s < s2;
            return false;
        }
    };
    struct CmpTeacherAtClassCourse {
        bool operator()(const std::tuple<Teacher, Class, Course>& left, const std::tuple<Teacher, Class, Course>& right) const {
            auto [t, cla, cour] = left;
            auto [t2, cla2, cour2] = right;
            if (t != t2) return t < t2;
            if (cla != cla2) return cla < cla2;
            if (cour != cour2) return cour < cour2;
            return false;
        }
    };
    struct CmpClassCoursePartAtSlot {
        bool operator()(const std::tuple<Class, Course, int, FlatSlot, int>& left, const std::tuple<Class, Course, int, FlatSlot, int>& right) const {
            auto [cla, cour, ic, s, is] = left;
            auto [cla2, cour2, ic2, s2, is2] = right;
            if (cla != cla2) return cla < cla2;
            if (cour != cour2) return cour < cour2;
            if (ic != ic2) return ic < ic2;
            if (s != s2) return s < s2;
            if (is != is2) return is < is2;
            return false;
        }
    };

    // Variable expressing: Teacher A is assigned to teach class B's course C.
    std::map<std::tuple<Teacher, Class, Course>, int, CmpTeacherAtClassCourse> _vars_teacher_at_class_course;
    // Variable expressing: Teacher A teaches in room B at slot C, part D.
    std::map<std::tuple<Teacher, Room, FlatSlot>, int, CmpTeacherAtRoomSlot> _vars_teacher_at_room_slot;
    std::map<std::tuple<Class, Room, FlatSlot>, int, CmpClassAtRoomSlot> _vars_class_at_room_slot;
    // Variable expressing: Class A's course B, part C takes place in room D at slot E, part F.
    std::map<std::tuple<Class, Course, int, FlatSlot, int>, int, CmpClassCoursePartAtSlot> _vars_class_course_part_at_room_slot;

#if EXPLAIN_VARS
    std::map<int, std::string> _var_explainer;
#endif

public:
    Encoding(const FlatTimetablingProblem& ttProblem) :
        _rooms_by_subject((ttProblem.roomsBySubject)),
        _teachers((ttProblem.teachers)),
        _classes((ttProblem.classes)),
        _slots((ttProblem.slots)) {}

    void encode(const std::string& cnfPath) {

        // Each classcourse has exactly one teacher
        printf("=1 teacher per classcourse\n");
        for (auto& cla : _classes) for (auto& course : cla.courses) {
            bool someTeacherEligible = false;
            for (auto& t : _teachers) if (t.isEligible(course)) {
                someTeacherEligible = true;
                appendToClause(varTeacherAtClassCourse(t, cla, course));
            }
            if (!someTeacherEligible) {
                printf("ERROR: No teacher eligible for class %s course %s!\n", cla.label.c_str(), course.subject.label.c_str());
                abort();
            }
            auto lits = currentClause();
            endClause(); // at-least-one
            encodeAtMostOne(lits);
        }

        // Each (mandatory) part of each classcourse takes place some time, some place
        printf("each classcourse takes place\n");
        for (auto& cla : _classes) for (auto& course : cla.courses) {
            for (int coursePart = 0; coursePart < course.minFlatSlots; coursePart++) {
                if (!_internal_clause.empty()) abort();
                for (auto& slot : _slots) for (int slotPart = 0; slotPart < slot.numParts; slotPart++) {
                    appendToClause(varClassCoursePartAtSlot(cla, course, coursePart, slot, slotPart));
                }
                endClause(); // at-least-one
                // we don't do at-most-one here since it could be expensive
            }
        }

        // If there's a class course at some slot, it must take place in some room
        for (auto& cla : _classes) for (auto& course : cla.courses) {
            for (int coursePart = 0; coursePart < course.minFlatSlots; coursePart++) {
                for (auto& slot : _slots) for (int slotPart = 0; slotPart < slot.numParts; slotPart++) {
                    appendToClause(-varClassCoursePartAtSlot(cla, course, coursePart, slot, slotPart));
                    for (auto& room : _rooms_by_subject.getSuitableRooms({course.subject})) {
                        appendToClause(varClassAtRoomSlot(cla, room, slot));
                    }
                    endClause();
                }
            }
        }

        // Each teacher can only be at one place at any time
        printf("teachers cannot multitask\n");
        for (auto& t : _teachers) {
            for (auto& slot : _slots) {
                std::set<int> vars;
                for (auto& room : _rooms_by_subject.getSuitableRooms(t.getTaughtSubjects())) {
                    vars.insert(varTeacherAtRoomSlot(t, room, slot));
                }
                encodeAtMostOne(std::vector<int>(vars.begin(), vars.end()));
            }
        }

        // Each class can only be at one place at any time
        printf("classes cannot multitask\n");
        for (auto& cla : _classes) {
            for (auto& slot : _slots) {
                std::set<int> vars;
                for (auto& [subj, rooms] : _rooms_by_subject.map) {
                    for (auto& room : rooms) {
                        vars.insert(varClassAtRoomSlot(cla, room, slot));
                    }
                }
                //printf("AMO ");
                //for (int lit : vars) printf("%i ", lit);
                //printf("\n");
                encodeAtMostOne(std::vector<int>(vars.begin(), vars.end()));
            }
        }

        std::map<std::pair<Room, FlatSlot>, std::vector<Teacher>> roomslotToTeachers;
        for (auto& slot : _slots) {
            for (auto& t : _teachers) for (auto& room : _rooms_by_subject.getSuitableRooms(t.getTaughtSubjects())) {
                roomslotToTeachers[{room, slot}].push_back(t);
            }
        }

        // Each roomslot has at most one teacher
        printf("<= 1 teacher per roomslot\n");
        for (auto& [roomslot, teachers] : roomslotToTeachers) {
            auto& [room, slot] = roomslot;
            for (auto& t : teachers)
                appendToClause(varTeacherAtRoomSlot(t, room, slot));
            auto lits = currentClause();
            clearClause();
            encodeAtMostOne(lits);
        }

        // Each roomslot has at most one class
        printf("<= 1 class per roomslot\n");
        for (auto& [roomslot, _] : roomslotToTeachers) {
            auto& [room, slot] = roomslot;
            for (auto& cla : _classes)
                appendToClause(varClassAtRoomSlot(cla, room, slot));
            auto lits = currentClause();
            clearClause();
            encodeAtMostOne(lits);
        }

        // If a classcourse takes place, then the teacher teaches the classcourse iff the teacher is at that place
        printf("connections\n");
        std::map<Room, std::map<Class, std::vector<Course>>> roomMap;
        for (auto& cla : _classes) for (auto& course : cla.courses) {
            for (auto& room : _rooms_by_subject.getSuitableRooms({course.subject})) {
                roomMap[room][cla].push_back(course);
                printf("%s %s\n", cla.label.c_str(), room.label.c_str());
                for (auto& slot : _slots) for (int slotPart = 0; slotPart < slot.numParts; slotPart++) {
                    for (int coursePart = 0; coursePart < course.minFlatSlots; coursePart++) {

                        for (auto& t : _teachers) if (t.isEligible(course)) {
                            addClause({
                                -varClassCoursePartAtSlot(cla, course, coursePart, slot, slotPart),
                                -varClassAtRoomSlot(cla, room, slot),
                                -varTeacherAtClassCourse(t, cla, course),
                                varTeacherAtRoomSlot(t, room, slot)
                            });
                            addClause({
                                -varClassCoursePartAtSlot(cla, course, coursePart, slot, slotPart),
                                -varClassAtRoomSlot(cla, room, slot),
                                varTeacherAtClassCourse(t, cla, course),
                                -varTeacherAtRoomSlot(t, room, slot)
                            });
                        }
                    }
                }
            }
        }

        // for each class at each roomslot, there can be at most one course (part) for that class
        printf("classes cannot multitask (refined)\n");
        for (auto& [room, cla2course] : roomMap) {
            for (auto& slot : _slots) for (int slotPart = 0; slotPart < slot.numParts; slotPart++) {
                for (auto& [cla, courses] : cla2course) {
                    for (auto& course : courses) {
                        for (int p = 0; p < course.minFlatSlots; p++) {
                            appendToClause(varClassCoursePartAtSlot(cla, course, p, slot, slotPart));
                        }
                    }
                    auto lits = currentClause();
                    clearClause();
                    encodeAtMostOne(lits);
                }
            }
        }

        // At each location and time, there can be at most one class-course
        // for (auto& slot : _slots) {
        //     const int slotPart = 0;
        //     for (auto& [room, classcourses] : roomMap) {
        //         for (auto& [cla, course] : classcourses) {
        //             for (int coursePart = 0; coursePart < course.minFlatSlots; coursePart++) {
        //                 appendToClause(varClassCoursePartAtRoomSlot(cla, course, coursePart, room, slot, slotPart));
        //             }
        //         }
        //         auto lits = currentClause();
        //         clearClause();
        //         encodeAtMostOne(lits);
        //     }
        // }

        outputFormula(cnfPath);
    }

    Timetable decode(const FlatTimetablingProblem& fttp, const std::vector<int>& model) {

#if EXPLAIN_VARS
        for (auto [v, str] : _var_explainer) {
            if (model[v] == v) printf("%s\n", str.c_str());
        }
#endif

        Timetable tt {fttp};
        for (auto& cla : _classes) for (auto& course : cla.courses) {
            for (auto& t : _teachers) if (t.isEligible(course)) {
                if (model[varTeacherAtClassCourse(t, cla, course)] <= 0) continue;
                // Teacher t is mapped to cla + course
                for (auto& slot : _slots) for (int slotPart = 0; slotPart < slot.numParts; slotPart++) {
                    for (auto& room : _rooms_by_subject.getSuitableRooms({course.subject})) {
                        if (model[varTeacherAtRoomSlot(t, room, slot)] <= 0) continue;
                        if (model[varClassAtRoomSlot(cla, room, slot)] <= 0) continue;
                        for (int coursePart = 0; coursePart < course.minFlatSlots; coursePart++) {
                            if (model[varClassCoursePartAtSlot(cla, course, coursePart, slot, slotPart)] > 0) {
                                tt.map[slot][room] = {t, cla, course};
                            }
                        }
                    }
                }
            }
        }

        return tt;
    }

private:
    void encodeAtMostOne(const std::vector<int>& lits) {
        // naive pairwise
        for (int l1 : lits) for (int l2 : lits) if (l1 < l2) {
            addClause({-l1, -l2});
        }
    }

    void addLiteral(int lit) {
        _out_lits.push_back(lit);
        _nb_cls += lit==0;
        _nb_vars = std::max(_nb_vars, std::abs(lit));
    }
    void addClause(std::initializer_list<int> lits) {
        for (int l : lits) addLiteral(l);
        addLiteral(0);
    }
    void outputFormula(const std::string& cnfPath) {
        std::ofstream ofs(cnfPath);
        ofs << "p cnf " << _nb_vars << " " << _nb_cls << std::endl;
        for (int lit : _out_lits) {
            ofs << lit;
            if (lit == 0) ofs << std::endl;
            else ofs << " ";
        }
    }

    std::vector<int> _internal_clause;
    void appendToClause(int lit) {_internal_clause.push_back(lit);}
    void appendToClause(std::initializer_list<int> lits) {
        for (int l : lits) appendToClause(l);
    }
    // copy explicitly so it can be used for other methods using appendToClause etc.
    std::vector<int> currentClause() {return _internal_clause;}
    void endClause() {
        for (int l : _internal_clause) addLiteral(l);
        addLiteral(0);
        clearClause();
    }
    void clearClause() {
        _internal_clause.clear();
    }

    int varTeacherAtRoomSlot(const Teacher& t, const Room& room, const FlatSlot& slot) {
        auto it = _vars_teacher_at_room_slot.find({t, room, slot});
        if (it != _vars_teacher_at_room_slot.end()) return it->second;
        int v = freshVariable();
        _vars_teacher_at_room_slot[{t, room, slot}] = v;
#if EXPLAIN_VARS
        _var_explainer[v] = "teacher " + t.name + " is at room " + room.label + " at slot " + std::to_string(slot.id);
#endif
        return v;
    }
    int varClassAtRoomSlot(const Class& cla, const Room& room, const FlatSlot& slot) {
        auto it = _vars_class_at_room_slot.find({cla, room, slot});
        if (it != _vars_class_at_room_slot.end()) return it->second;
        int v = freshVariable();
        _vars_class_at_room_slot[{cla, room, slot}] = v;
#if EXPLAIN_VARS
        _var_explainer[v] = "class " + cla.label + " is at room " + room.label + " at slot " + std::to_string(slot.id);
#endif
        return v;
    }
    int varTeacherAtClassCourse(const Teacher& t, const Class& cla, const Course& course) {
        auto it = _vars_teacher_at_class_course.find({t, cla, course});
        if (it != _vars_teacher_at_class_course.end()) return it->second;
        int v = freshVariable();
        _vars_teacher_at_class_course[{t, cla, course}] = v;
#if EXPLAIN_VARS
        _var_explainer[v] = "teacher " + t.name + " teaches class " + cla.label + " in course " + course.subject.label;
#endif
        return v;
    }
    int varClassCoursePartAtSlot(const Class& cla, const Course& course, int coursePart, const FlatSlot& slot, int slotPart) {
        auto it = _vars_class_course_part_at_room_slot.find({cla, course, coursePart, slot, slotPart});
        if (it != _vars_class_course_part_at_room_slot.end()) return it->second;
        int v = freshVariable();
        _vars_class_course_part_at_room_slot[{cla, course, coursePart, slot, slotPart}] = v;
#if EXPLAIN_VARS
        _var_explainer[v] = "class " + cla.label + " course " + course.subject.label + " pt. " + std::to_string(coursePart)
            + " takes place at slot " + std::to_string(slot.id) + " pt. " + std::to_string(slotPart);
#endif
        return v;
    }

    int freshVariable() {int v = _running_var; _running_var++; return v;}
};
