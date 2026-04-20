
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "../random.hpp"

const std::vector<std::string> teacherNames {"Mueller","Schmidt","Schneider","Fischer","Weber","Meyer","Wagner",
    "Schulz","Bauer","Becker","Meier","Hoffmann","Koch","Klein","Richter","Wolf","Schwarz",
    "Schaefer","Huber","Neumann","Maier","Mayer","Schroeder","Braun","Berger","Schmid",
    "Zimmermann","Schmitz","Schmitt","Hofmann","Krueger","Hartmann","Werner","Fuchs","Lange",
    "Lehmann","Koenig","Kaiser","Krause","Walter","Schulze","Lang","Keller","Peters","Herrmann",
    "Winkler","Frank","Sommer","Steiner","Jung","Koehler","Baumann","Winter","Roth","Vogel",
    "Scholz","Moeller","Gruber","Graf","Schubert","Engel","Schuster","Stein","Friedrich","Beck",
    "Hahn","Lorenz","Martin","Peter","Simon","Moser","Albrecht","Franke","Guenther","Haas",
    "Ludwig","Schumacher","Kraus","Thomas","Vogt","Jaeger","Kaufmann","Heinrich","Mueller","Weiss",
    "Schreiber","Brunner","Hansen","Maus","Otto","Boehm","Bergmann","Paul","Brandt","Seidel",
    "Kern","Hofer","Kraemer","Dietrich","Herrmann","Arnold","Jansen","Kramer","Franz","Ernst",
    "Kuhn","Lindner","Horn","Ziegler","Maurer","Ritter","Schulte","Pohl","Sauer","Pfeiffer",
    "Busch","Voigt","Seifert","Bachmann","Wolff"};

const std::string teacherInitials = "WERTUIOPASDFGHJKLCBNM";

const std::vector<std::string> subjects = {"ASW","BildendeKunst","Biologie",
    "Chemie","Deutsch","Englisch","Franzoesisch","Gemeinschaftskunde","Geographie","Geschichte",
    "Informatik","Mathematik","Musik","Physik","Religion/Ethik","Seminarkurs","Sport","Wirtschaft"};

const std::vector<std::vector<std::string>> subjectCategories = {
    {"Deutsch"},
    {"Englisch"},
    {"Geographie","Wirtschaft","Gemeinschaftskunde","ASW"},
    {"BildendeKunst"},
    {"Biologie"},
    {"Chemie"},
    {"Sport"},
    {"Franzoesisch"},
    {"Geschichte"},
    {"Informatik","Mathematik"},
    {"Musik"},
    {"Physik"},
    {"Religion/Ethik"},
    {"Seminarkurs"}
};

enum RoomType {SPORT, SCIENCE, MUSIC, ARTS, IT, GENERIC};
const std::vector<RoomType> roomTypes = {SPORT, SCIENCE, MUSIC, ARTS, IT, GENERIC};
const std::vector<char> roomPrefixes = {'H', 'L', 'M', 'K', 'C', 'R'};
const std::vector<int> roomOccurrences = {2, 6, 3, 3, 4, 50};
const std::map<std::string, RoomType> subjectToRoomType = {
    {"ASW",GENERIC},
    {"BildendeKunst",ARTS},
    {"Biologie",SCIENCE},
    {"Chemie",SCIENCE},
    {"Deutsch",GENERIC},
    {"Englisch",GENERIC},
    {"Franzoesisch",GENERIC},
    {"Gemeinschaftskunde",GENERIC},
    {"Geographie",GENERIC},
    {"Geschichte",GENERIC},
    {"Informatik",IT},
    {"Mathematik",GENERIC},
    {"Musik",MUSIC},
    {"Physik",SCIENCE},
    {"Religion/Ethik",GENERIC},
    {"Seminarkurs",GENERIC},
    {"Sport",SPORT},
    {"Wirtschaft",GENERIC}
};

std::map<std::string, int> getSubjectsOfGrade(int grade) {
    std::map<std::string, int> out;

    std::string filename = "generate/subjects-" + std::to_string(grade) + ".txt";
    std::ifstream ifs(filename);
    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string subject, nbHoursStr;
        std::getline(iss, subject, ' ');
        std::getline(iss, nbHoursStr, ' ');
        out[subject] = std::atoi(nbHoursStr.c_str());
    }

    return out;
}

float getRatioOfSubject(const std::string& subject, int maxGrade) {
    int nbSubject = 0;
    int nbAll = 0;
    for (int grade = 5; grade <= maxGrade; grade++) {
        for (auto& [s, nbUnits] : getSubjectsOfGrade(grade))
            (s == subject ? nbSubject : nbAll) += nbUnits;
    }
    return nbSubject / (float) nbAll;
}

int main(int argc, char* argv[]) {
    if (argc < 11) {
        std::cerr << "Usage: " << argv[0]
                  << " <nbDays> <nbSlots> <maxGrade> <minNumClasses> <maxNumClasses>"
                  << " <minTaughtSubjects> <maxTaughtSubjects> <roomMultiplier>"
                  << " <teacherMultiplier> <seed>\n"
                  << "Example: " << argv[0] << " 5 12 2 4 1.4 2.6 1 1 1337\n";
        return 1;
    }

    const int    nbDays                = std::atoi(argv[1]);
    const int    nbSlots               = std::atoi(argv[2]);
    const int    maxGrade              = std::atoi(argv[3]);
    const float  minNumClasses         = std::atof(argv[4]);
    const float  maxNumClasses         = std::atof(argv[5]);
    const float  minTaughtSubjects     = std::atof(argv[6]);
    const float  maxTaughtSubjects     = std::atof(argv[7]);
    const float  roomMultiplierBase    = std::atof(argv[8]);
    const float  teacherMultiplierBase = std::atof(argv[9]);
    const int    seed                  = std::atoi(argv[10]);

    const int totalEstNbClasses = 0.5f * (maxNumClasses + minNumClasses) * (maxGrade - 5 + 1);
    const float nbTeachers      = totalEstNbClasses * teacherMultiplierBase;
    const float roomMultiplier  = totalEstNbClasses * roomMultiplierBase;

    SplitMix64Rng rng(seed);
    auto rngFunc = [&]() {return rng.randomInRange(0, 1);};

    printf("#RoomsBySubject\n");
    for (int c = 0; c < subjectCategories.size(); c++) {
        auto& cat = subjectCategories[c];
        float ratio = 0;
        for (auto& subj : cat) ratio += getRatioOfSubject(subj, maxGrade);
        int nbRooms = std::max((int) std::ceil(roomMultiplier * ratio), 2);
        for (auto& subject : cat) {
            std::string roomString;
            for (int r = 0; r < nbRooms; r++) {
                roomString += "," + subject + "_R" + std::to_string(r+1);
            }
            printf("%s%s\n", subject.c_str(), roomString.c_str());
        }
    }

    printf("#Teachers\n");
    std::set<std::string> subjectsWithFullyCapableTeacher;
    std::vector<std::vector<std::string>> categoriesToChooseFrom = subjectCategories;
    for (int i = 0; i < nbTeachers; i++) {
        std::string surname = random_choice_k_from_n(teacherNames.data(), teacherNames.size(), 1, rngFunc)[0];
        char initial = random_choice_k_from_n(teacherInitials.data(), teacherInitials.size(), 1, rngFunc)[0];
        std::string name = std::string(1, initial) + "." + surname;

        // Decide on a set of taught subject categories
        int nbSubjectsToTeach = std::round(rng.randomInRange(minTaughtSubjects, maxTaughtSubjects));
        int nbTaught = 0;
        std::vector<std::vector<std::string>> categories;
        while (nbTaught < nbSubjectsToTeach && !categoriesToChooseFrom.empty()) {
            int idx = rng.randomInRange(0, categoriesToChooseFrom.size());
            categories.push_back(categoriesToChooseFrom[idx]);
            nbTaught += categories.back().size();
            categoriesToChooseFrom.erase(categoriesToChooseFrom.begin() + idx);
        }
        auto randCats = random_choice_k_from_n(subjectCategories.data(), subjectCategories.size(),
            nbSubjectsToTeach - nbTaught, rngFunc);
        categories.insert(categories.end(), randCats.begin(), randCats.end());

        std::set<std::string> subjects;
        for (auto cat : categories) subjects.insert(cat.begin(), cat.end());
        for (auto subj : subjects) {
            int maxLevel = subjectsWithFullyCapableTeacher.count(subj) ? std::min(12, (int)std::round(rng.randomInRange(6.6, 18))) : 12;
            printf("%s,%s", name.c_str(), subj.c_str());
            for (int l = 5; l <= maxLevel; l++) printf(",%i", l);
            printf("\n");
            subjectsWithFullyCapableTeacher.insert(subj);
        }
    }
    if (!categoriesToChooseFrom.empty()) abort();

    printf("#Classes\n");
    for (int grade = 5; grade <= maxGrade; grade++) {
        int nbClasses = std::round(rng.randomInRange(minNumClasses, maxNumClasses));
        std::vector<std::string> suffixes = {"a", "b", "c", "d", "e", "f"};
        for (int c = 0; c < nbClasses; c++) {
            std::string classLabel = std::to_string(grade) + suffixes[c];
            for (auto& [subject, nbUnits] : getSubjectsOfGrade(grade)) {
                printf("%s,%s,%i,%i,%i\n", classLabel.c_str(), subject.c_str(), grade, nbUnits, nbUnits);
            }
        }
    }

    printf("#Days\n");
    std::string slotsStr = "";
    if (nbSlots >= 1) slotsStr += "8,9.5,";
    if (nbSlots >= 2) slotsStr += "9.75,11.25,";
    if (nbSlots >= 3) slotsStr += "11.5,12.25,";
    if (nbSlots >= 4) slotsStr += "12.25,13,";
    if (nbSlots >= 5) slotsStr += "14,15.5,";
    if (nbSlots >= 6) slotsStr += "15.75,17.25,";
    if (nbSlots >= 7) slotsStr += "17.5,18.25,";
    slotsStr = slotsStr.substr(0, slotsStr.size()-1);
    
    if (nbDays >= 1) printf("Montag,%s\n", slotsStr.c_str());
    if (nbDays >= 2) printf("Dienstag,%s\n", slotsStr.c_str());
    if (nbDays >= 3) printf("Mittwoch,%s\n", slotsStr.c_str());
    if (nbDays >= 4) printf("Donnerstag,%s\n", slotsStr.c_str());
    if (nbDays >= 5) printf("Freitag,%s\n", slotsStr.c_str());
    printf("#UnitDuration\n0.75\n");
}
