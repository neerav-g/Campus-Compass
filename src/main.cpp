#include "CampusCompass.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

// Define file paths relative to the execution directory
// NOTE: These paths assume your file structure is correctly set up (e.g., your_root_dir/data/edges.csv)
#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

int main() {
    // Instantiate the CampusCompass object
    CampusCompass cc;
    
    // 1. Load data
    if (!cc.ParseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH)) {
        cerr << "Warning: Could not load required CSV files. Proceeding with potentially incomplete data." << endl;
    }

    // Line 1: Read the total number of commands.
    string line_count_str;
    if (getline(cin, line_count_str)) {
        cc.ParseCommand(line_count_str);
    } else {
        return 0;
    }

    // 2. Command Parsing Loop
    string command;
    while (getline(cin, command)) {
        if (!command.empty()) {
            cc.ParseCommand(command);
        }
    }

    return 0;
}