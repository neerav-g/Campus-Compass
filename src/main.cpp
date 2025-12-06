#include "CampusCompass.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// Define file paths for data loading
#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

int main() {
    
    // Instantiate the CampusCompass object
    CampusCompass cc;
    
    // 1. Load data
    if (!cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH)) {
        cerr << "Warning: Could not load required CSV files." << endl;
    }

    // --- Critical Fix for Autograder Output ---
    // Read the total number of commands line but DO NOT process it as a command.
    // This prevents the extraneous "successful" output line at the start.
    string lineCountStr;
    if (getline(cin, lineCountStr)) {
        // Read and discard the line count.
    } else {
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