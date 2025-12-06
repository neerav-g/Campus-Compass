#include "CampusCompass.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string> // Ensure string is included

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

    // --- CRITICAL FIX START ---
    // Read the total number of commands line but DO NOT process it as a command
    // This prevents the extraneous "successful" output in the test environment.
    string line_count_str;
    if (getline(cin, line_count_str)) {
        // The line is read (e.g., "6" is in line_count_str), but we do nothing with it.
    } else {
        // No input detected
        return 0;
    }
    // --- CRITICAL FIX END ---

    // 2. Command Parsing Loop
    // The loop begins processing the *first actual command* (e.g., "insert...")
    string command;
    while (getline(cin, command)) {
        if (!command.empty()) {
            cc.ParseCommand(command);
        }
    }

    return 0;
}