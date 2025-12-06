#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

// Must include your CampusCompass header
#include "CampusCompass.h" 

using namespace std;

// Define file paths for data loading
#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

// --- Helper Function to Capture Output ---
string captureOutput(const string& input) {
    CampusCompass cc;
    
    // 1. Load data
    cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH);

    // 2. Setup output capture
    stringstream ss_input(input);
    stringstream ss_output;
    
    // Redirect stdout to the stringstream
    streambuf* old_cout = cout.rdbuf();
    cout.rdbuf(ss_output.rdbuf());

    // 3. Process commands
    string line;
    // Skip the first line (command count) as implemented in main.cpp
    if (getline(ss_input, line)) {
        // Read and discard line count.
    }

    // Process the rest of the commands
    while (getline(ss_input, line)) {
        if (!line.empty()) {
            cc.parseCommand(line);
        }
    }

    // 4. Restore stdout
    cout.rdbuf(old_cout);

    // Clean up trailing newlines for comparison
    string output = ss_output.str();
    while (!output.empty() && output.back() == '\n') {
        output.pop_back();
    }
    
    return output;
}

// --- REQUIRED TEST CASES ---

// [1 point] Test at least five incorrect commands.
TEST_CASE("Test 1: Incorrect/Invalid Commands (Five Cases)", "[unit-tests][incorrect]") {
    string input = R"(6
insert "Initial Student" 10000001 1 1 COP3530  // Must succeed to make next one a duplicate
insert "Invalid Name!" 10000002 1 1 COP3530  // 1. Invalid character in name
insert "Valid Name" 10000001 1 1 COP3530     // 2. Duplicate UFID (FAIL)
insert "Valid Name" 10000005 1 0 COP3530     // 3. Invalid class count (0)
dropClass 99999999 COP3530                   // 4. Student UFID (99999999) does not exist 
checkEdgeStatus 1 999                        // 5. Location ID 999 does not exist 
)";

    // Output reflects one success followed by five failures (as required by prompt, 5 incorrect commands)
    string expectedOutput = R"(successful
unsuccessful
unsuccessful
unsuccessful
unsuccessful
DNE)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test at least three edge cases for various functions.
TEST_CASE("Test 2: Edge Cases (Deletion Logic, DNE)", "[unit-tests][edge-cases]") {
    string input = R"(7
insert "Edge Student" 10000006 1 1 MAC2311  // S1: MAC2311
insert "Only Class" 10000007 1 1 CDA3101   // S2: CDA3101
dropClass 10000007 CDA3101                 // 1. Drop last class: Removes S2.
remove 10000008                             // 2. Remove DNE.
dropClass 10000006 COP3502                 // Drop class S1 doesn't have.
checkEdgeStatus 1 999
remove 10000006                             // 3. Remove S1 who still has MAC2311.
)";

    string expectedOutput = R"(successful
successful
successful
unsuccessful
unsuccessful
DNE
unsuccessful)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test dropClass, removeClass, remove, and replaceClass commands
TEST_CASE("Test 3: Core Management Command Integration", "[unit-tests][management]") {
    string input = R"(7
insert "Test Drop" 20000001 1 2 COP3502 MAC2311
insert "Test RemoveClass" 20000002 1 1 CDA3101
insert "Test RemoveClass 2" 20000003 1 1 CDA3101
replaceClass 20000001 MAC2311 CDA3101
dropClass 20000001 COP3502
removeClass CDA3101
remove 20000001
)";

    // Logic Trace confirmed: removeClass removes students 20000001, 20000002, 20000003.
    string expectedOutput = R"(successful
successful
successful
successful
successful
3
unsuccessful)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [2 points] Test your printShortestEdges command in a case where reachability changes.
TEST_CASE("Test 4: ShortestEdges Reachability and Toggle", "[unit-tests][closure]") {
    
    string input = R"(5
insert "Closure Test" 30000001 1 1 COP3530
printShortestEdges 30000001
toggleEdgesClosure 1 1 4 
printShortestEdges 30000001
toggleEdgesClosure 1 1 4
)";

    // Path 1->10 (COP3530 loc). Time: 23 (Open) -> 27 (Closed and rerouted)
    string expectedOutput = R"(successful
Name: Closure Test
COP3530 | Total Time: 23
successful
Name: Closure Test
COP3530 | Total Time: 27
successful)";

    REQUIRE(captureOutput(input) == expectedOutput);
}