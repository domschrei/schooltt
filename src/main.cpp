
#include "model.hpp"
#include "parse.hpp"
#include "encoding.hpp"
#include <iostream>
#include <filesystem>

std::vector<int> parseModel(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<int> solution(1, 0);
    bool inSolution = false;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        if (line[0] == 's') continue; // Ignore "s SATISFIABLE"
        if (line[0] == 'v') {
            inSolution = true;
            std::istringstream iss(line.substr(1)); // Skip 'v'
            int val;
            while (iss >> val) {
                int k = abs(val);
                if (k >= (int)solution.size()) {
                    solution.resize(k+1, 0); // Resize and fill with 0 (or -1, -2, ...)
                }
                solution[k] = val; // Store k or -k
            }
        }
    }
    return solution;
}

void printTable(const std::vector<std::vector<std::string>>& table) {

    std::vector<size_t> cellWidths(table.front().size(), 0);
    for (auto& row : table)
        for (size_t i = 0; i < row.size(); i++)
            cellWidths[i] = std::max(cellWidths[i], row[i].size());

    for (size_t i = 0; i < table.size(); i++) {
        auto& row = table[i];
        for (size_t j = 0; j < table[i].size(); j++) {
            int cellWidth = cellWidths[j];
            printf("%s%s  ", row[j].c_str(), std::string(cellWidth - row[j].size(), ' ').c_str());
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <school-tt-file> <cnf-output> [<solver-bin-path> | <solver-output>]\n";
        return 1;
    }

    const std::string inputPath(argv[1]);
    const std::string cnfPath(argv[2]);
    const std::string solverPath = (argc >= 4 ? argv[3] : "");
    bool solverOutputProvided = solverPath.size() >= 4 && solverPath.compare(solverPath.size() - 4, 4, ".txt") == 0;
    bool runSolver = !solverPath.empty() && !solverOutputProvided;
    const std::string solverOutput = runSolver ? ".solver-output.txt" : solverPath;

    FlatTimetablingProblem fttp(ttp::parseTimetablingProblem(inputPath));
    Encoding enc(fttp);

    if (!std::filesystem::is_regular_file(cnfPath)) {
        printf("Encoding ...\n");
        enc.encode(cnfPath);
        printf("Encoding done\n");
    } else {
        printf("%s already present\n", cnfPath.c_str());
    }

    if (runSolver) {
        ::remove(solverOutput.c_str());
        std::string cmd = solverPath + " " + cnfPath + " -q > " + solverOutput;
        printf("Executing SAT solver: %s\n", cmd.c_str());
        system(cmd.c_str());
        printf("SAT solving done\n");
    }

    if (!solverOutput.empty()) {
        printf("Parsing solver output ...\n");
        auto sol = parseModel(solverOutput);

        auto tt = enc.decode(fttp, sol);
        std::cout << tt.toStr();
        for (auto& cla : fttp.classes) {
            std::cout << "\nClass " << cla.label << ":\n";
            printTable(tt.formatTimetableFor(cla));
        }
        for (auto& t : fttp.teachers) {
            std::cout << "\nTeacher " << t.name << ":\n";
            printTable(tt.formatTimetableFor(t));
        }
        fflush(stdout);
    }
}
