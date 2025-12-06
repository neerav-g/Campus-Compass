#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <set>

#include "CampusCompass.h" 

using namespace std;

#define EDGES_FILE_PATH "data/edges.csv"
#define CLASSES_FILE_PATH "data/classes.csv"

// --- Helper Function to Capture Output ---
string captureOutput(const string& input) {
    CampusCompass cc;
    
    cc.parseCSV(EDGES_FILE_PATH, CLASSES_FILE_PATH);

    stringstream ss_input(input);
    stringstream ss_output;
    
    streambuf* old_cout = cout.rdbuf();
    cout.rdbuf(ss_output.rdbuf());

    // Skip the command count line
    string line;
    if (getline(ss_input, line)) {
        // Discard line count.
    }

    while (getline(ss_input, line)) {
        if (!line.empty()) {
            cc.parseCommand(line);
        }
    }

    cout.rdbuf(old_cout);
    
    return ss_output.str();
}

// [1 point] Test at least five incorrect commands.
TEST_CASE("Test 1: Incorrect/Invalid Commands and Data Validation", "[validation]") {
    string input = R"(5
insert "Invalid Name123" 11111111 1 1 COP3502
insert "Valid Name" 11111111 1 1 COP3502 
insert "Valid Name" 11111 1 1 COP3502
insert "Valid Name" 22222222 1 7 COP3502 MAC2311 CDA3101 EEL3701 PHY2054 CHM2045 ADD5555
insert "Valid Name" 33333333 1 1 INVALID
)";

    string expectedOutput = R"(unsuccessful
unsuccessful
unsuccessful
unsuccessful
unsuccessful
)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test at least three edge cases for various functions (e.g., removing a student that doesn’t exist).
TEST_CASE("Test 2: Edge Cases (Deletion Logic, DNE)", "[edge-cases]") {
    string input = R"(4
insert "Edge Case Student" 40000001 1 1 COP3530
dropClass 40000001 COP3530
remove 99999999
dropClass 40000001 MAC2311
)";

    // Logic Trace:
    // 1. insert 40000001: successful (Classes: COP3530)
    // 2. dropClass (last class): successful (student is removed)
    // 3. remove DNE (99999999): unsuccessful
    // 4. dropClass DNE (40000001 already removed): unsuccessful
    string expectedOutput = R"(successful
successful
unsuccessful
unsuccessful
)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [1 point] Test dropClass, removeClass, remove, and replaceClass commands
TEST_CASE("Test 3: Core Management Command Integration", "[management]") {
    string input = R"(7
insert "Test Drop" 20000001 1 2 COP3530 MAC2311
insert "Test RemoveClass 1" 20000002 1 1 CDA3101
insert "Test RemoveClass 2" 20000003 1 2 CDA3101 EEL3701
replaceClass 20000001 MAC2311 CDA3101
dropClass 20000001 COP3530
removeClass CDA3101
remove 20000003
)";

    // Logic Trace:
    // 4. replaceClass 20000001: successful (Classes: COP3530, CDA3101)
    // 5. dropClass 20000001 COP3530: successful (Classes: CDA3101)
    // 6. removeClass CDA3101: Affects 3 students. 20000001 and 20000002 removed. 20000003 remains with EEL3701. Output: 3
    // 7. remove 20000003: successful

    string expectedOutput = R"(successful
successful
successful
successful
successful
3
successful
)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// [2 points] Test printShortestEdges reachability change.
TEST_CASE("Test 4: ShortestEdges Reachability and Toggle", "[closure]") {
    
    string input = R"(8
insert "Traveler A" 1 2 CDA3101 MAC2311
printShortestEdges 1
toggleEdgesClosure 1 1 2
printShortestEdges 1
toggleEdgesClosure 1 1 2
printShortestEdges 1
toggleEdgesClosure 1 1 2
printShortestEdges 1
)";
    // Assuming Loc 1 (Res) to Loc 2 (CDA3101) is shortest path 12.
    // Assuming Loc 1 to Loc 4 (MAC2311) is shortest path 50.
    // Assuming no alternate path from 1 to 2 exists once 1-2 is closed.

    string expectedOutput = R"(successful
Name: Traveler A
CDA3101 | Total Time: 12
MAC2311 | Total Time: 50
successful
Name: Traveler A
CDA3101 | Total Time: -1
MAC2311 | Total Time: 50
successful
Name: Traveler A
CDA3101 | Total Time: 12
MAC2311 | Total Time: 50
successful
Name: Traveler A
CDA3101 | Total Time: -1
MAC2311 | Total Time: 50
)";

    REQUIRE(captureOutput(input) == expectedOutput);
}

// Extra Credit Test Case
TEST_CASE("Test 5: Verify Schedule (Extra Credit)", "[extra-credit]") {
    string input = R"(2
insert "Scheduler A" 60000001 1 3 CDA3101 MAC2311 EEL3701
verifySchedule 60000001
)";
    
    // Schedule (Sorted):
    // 1. MAC2311 (Loc 4): 10:40 - 11:30
    // 2. CDA3101 (Loc 2): 11:30 - 12:40 (Modified from sample time for testing)
    // 3. EEL3701 (Loc 3): 12:50 - 13:40 (Modified from sample time for testing)
    
    // Check 1: 4 -> 2. Time Gap = 11:30 - 11:30 = 0 mins. Shortest Time (4->2) > 0.
    // Check 2: 2 -> 3. Time Gap = 12:50 - 12:40 = 10 mins. Shortest Time (2->3) (e.g., 5 mins) < 10.

    string expectedOutput = R"(successful
Schedule Check for Scheduler A:
MAC2311 - CDA3101 "Cannot make it!"
CDA3101 - EEL3701 "Can make it!"
)";

    REQUIRE(captureOutput(input) == expectedOutput);
}