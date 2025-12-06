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
// This function initializes the CampusCompass, loads data, redirects std::cout, 
// processes the input string, and returns the captured output string for comparison.
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
    string input = R"(5
insert "Invalid Name!" 10000001 1 1 COP3530
insert "Valid Name" 10000001 1 1 COP3530
insert "Valid Name" 10000005 1 0 COP3530
dropClass 99999999 COP3530
checkEdgeStatus 1 999
)";

    // Five Failure Cases:
    // 1. Invalid character in name.
    // 2. Duplicate UFID.
    // 3. Invalid class count (0).
    // 4. Student UFID (99999999) does not exist.
    // 5. Location ID 999 does not exist (DNE).
    string expectedOutput = R"(unsuccessful
unsuccessful
unsuccessful
unsuccessful
DNE)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test at least three edge cases for various functions.
TEST_CASE("Test 2: Edge Cases (Deletion Logic, DNE)", "[unit-tests][edge-cases]") {
    string input = R"(7
insert "Edge Student" 10000006 1 1 MAC2311
insert "Only Class" 10000007 1 1 CDA3101
dropClass 10000007 CDA3101
remove 10000008
dropClass 10000006 COP3502
checkEdgeStatus 1 999
remove 10000006
)";

    // Edge Cases Tested:
    // 1. **dropClass**: Dropping the *last* class (10000007 CDA3101) triggers full student removal.
    // 2. **remove**: Removing a student that **DNE** (10000008).
    // 3. **remove**: Removing a student that **exists but has classes** (10000006).
    string expectedOutput = R"(successful
successful
successful
unsuccessful
unsuccessful
DNE
unsuccessful)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test your dropClass, removeClass, remove, and replaceClass commands
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

    // Logic Trace:
    // 4. replaceClass: (20000001: {COP3502, CDA3101}) -> successful
    // 5. dropClass: (20000001: {CDA3101}) -> successful
    // 6. removeClass CDA3101: Affects 3 students; all 3 are removed. Output: 3
    // 7. remove 20000001: Fails because the student was already removed in step 6.
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
    // This test ensures Dijkstra correctly ignores closed edges and finds the next shortest route.
    // It assumes a direct path (Time 13) and an alternate path (Time 17) exist between Loc 1 and Loc 4 (COP3530).
    
    string input = R"(5
insert "Closure Test" 30000001 1 1 COP3530
printShortestEdges 30000001
toggleEdgesClosure 1 1 4 
printShortestEdges 30000001
toggleEdgesClosure 1 1 4
)";

    // Expected Logic:
    // 2. print: Path is direct (13).
    // 3. toggleEdgesClosure: Closes the 1-4 edge.
    // 4. print: Path must now use alternate route (17).
    string expectedOutput = R"(successful
Name: Closure Test
COP3530 | Total Time: 13
successful
Name: Closure Test
COP3530 | Total Time: 17
successful)";

    REQUIRE(captureOutput(input) == expectedOutput);
}