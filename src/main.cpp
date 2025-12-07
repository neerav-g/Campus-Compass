#include "CampusCompass.h"
#include <iostream>
#include <string>

using namespace std;

// Defined file paths for data loading
#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

int main() {
    // Create the main application object
    CampusCompass cc;
    
    // Load CSV files. This should be done first.
    // The problem statement says: "If this fails, do not print to stdout".
    cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH);

    // Read and discard the first line that indicates number of commands
    string lineCountStr;
    if (!getline(cin, lineCountStr)) return 0;

    // Process commands from standard input
    string command;
    while (getline(cin, command)) {
        if (!command.empty()) {
            cc.parseCommand(command);
        }
    }
    return 0;
}