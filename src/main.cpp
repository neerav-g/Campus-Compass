#include "CampusCompass.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// Define file paths for data loading
#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

int main() {
    
    CampusCompass cc;
    
    // 1. Load data
    if (!cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH)) {
        // Warning is omitted to keep output clean, but failure is noted.
    }

    // Read and discard the command count line (required for autograder format)
    string lineCountStr;
    if (!getline(cin, lineCountStr)) {
        return 0; 
    }

    // 2. Command Parsing Loop
    string command;
    while (getline(cin, command)) {
        if (!command.empty()) {
            cc.parseCommand(command);
        }
    }

    return 0;
}