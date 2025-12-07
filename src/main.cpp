#include "CampusCompass.h"
#include <iostream>
#include <string>

using namespace std;

#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

int main() {
    CampusCompass cc;
    // load CSV files. If this fails, do not print to stdout
    cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH);

    // read and discard the first line that indicates number of commands
    string lineCountStr;
    if (!getline(cin, lineCountStr)) return 0;

    string command;
    while (getline(cin, command)) {
        if (!command.empty()) {
            cc.parseCommand(command);
        }
    }
    return 0;
}
