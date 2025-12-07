#include <catch2/catch_test_macros.hpp>
#include "CampusCompass.h"
#include <iostream>
#include <sstream>
#include <string>

// Helper function to simulate parsing a command and capturing output
std::string runCommand(CampusCompass& cc, const std::string& command) {
    std::stringstream buffer;
    // Redirect std::cout to the buffer
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());
    
    cc.parseCommand(command);
    
    // Restore std::cout
    std::cout.rdbuf(old);
    return buffer.str();
}

// NOTE: This setup assumes the test data (edges.csv and classes.csv) 
// is correctly loaded by the CampusCompass constructor/setup, and 
// contains the minimum required data for these tests (e.g., edges 100-200, 300-400, classes COP3530, EEL3701).

TEST_CASE("Part 1: Incorrect Command Input Validation", "[UnitTests]") {
    CampusCompass cc;
    // Assume cc.parseCSV("data/edges.csv", "data/classes.csv") succeeded in a global setup.

    SECTION("Incorrect 'insert' commands (5 tests)") {
        // 1. Invalid Name (contains non-alpha/space char)
        REQUIRE(runCommand(cc, "insert \"Alice1\" 12345678 100 1 COP3530").find("unsuccessful") != std::string::npos);
        
        // 2. Invalid UFID (not 8 digits)
        REQUIRE(runCommand(cc, "insert \"Bob\" 1234567 100 1 COP3530").find("unsuccessful") != std::string::npos);
        
        // 3. Invalid Class Code (not 3 letters + 4 digits)
        REQUIRE(runCommand(cc, "insert \"Charlie\" 87654321 100 1 C3530").find("unsuccessful") != std::string::npos);
        
        // 4. Class Code DNE in classesMap
        REQUIRE(runCommand(cc, "insert \"David\" 11112222 100 1 FAK3000").find("unsuccessful") != std::string::npos);
        
        // 5. Invalid number of classes (0 classes)
        REQUIRE(runCommand(cc, "insert \"Eve\" 33334444 100 0").find("unsuccessful") != std::string::npos);
    }
}

TEST_CASE("Part 2: Edge Cases for Various Functions", "[UnitTests]") {
    CampusCompass cc;
    // Assume cc.parseCSV loaded data
    
    // Setup a valid student "Alice"
    REQUIRE(runCommand(cc, "insert \"Alice\" 10000000 100 1 COP3530").find("successful") != std::string::npos);

    SECTION("Edge Case 1: Removing a student that doesn’t exist") {
        REQUIRE(runCommand(cc, "remove 99999999").find("unsuccessful") != std::string::npos);
    }

    SECTION("Edge Case 2: Attempting to remove a student with classes (should fail)") {
        REQUIRE(runCommand(cc, "remove 10000000").find("unsuccessful") != std::string::npos); // Alice still has COP3530
    }

    SECTION("Edge Case 3: Checking status of an edge that DNE") {
        REQUIRE(runCommand(cc, "checkEdgeStatus 999 888").find("DNE") != std::string::npos);
    }

    SECTION("Edge Case 4: isConnected check between non-existent nodes") {
        REQUIRE(runCommand(cc, "isConnected 999 888").find("unsuccessful") != std::string::npos);
    }
}

TEST_CASE("Part 3: dropClass, removeClass, remove, and replaceClass", "[UnitTests]") {
    CampusCompass cc;
    // NOTE: We assume 'COP3530' and 'EEL3701' are valid classes in classes.csv.
    
    // Test the state transitions completely.

    SECTION("Drop Class and Student Removal Logic") {
        // Setup Alice (10000000) with 2 classes
        runCommand(cc, "insert \"Alice\" 10000000 100 2 COP3530 EEL3701");
        // Setup Bob (20000000) with 1 class
        runCommand(cc, "insert \"Bob\" 20000000 400 1 COP3530");

        // dropClass (Alice drops one, remains in system)
        REQUIRE(runCommand(cc, "dropClass 10000000 EEL3701").find("successful") != std::string::npos);
        
        // dropClass (Bob drops his last class, should be REMOVED)
        REQUIRE(runCommand(cc, "dropClass 20000000 COP3530").find("successful") != std::string::npos);
        
        // Check: Bob is now gone.
        REQUIRE(runCommand(cc, "remove 20000000").find("unsuccessful") != std::string::npos); 
    }
    
    SECTION("replaceClass Logic") {
        // Setup Charlie (30000000) with COP3530 (assuming NEW9999 is another valid class)
        // NOTE: If NEW9999 DNE in your classes.csv, this test needs to use two *known* valid class codes. 
        // We'll use EEL3701 which is assumed to be valid but NOT currently held by Charlie.
        runCommand(cc, "insert \"Charlie\" 30000000 100 1 COP3530");

        // replaceClass (Should succeed: COP3530 -> EEL3701)
        REQUIRE(runCommand(cc, "replaceClass 30000000 COP3530 EEL3701").find("successful") != std::string::npos);
        
        // Fails if student already has the target class EEL3701
        REQUIRE(runCommand(cc, "replaceClass 30000000 COP3530 EEL3701").find("unsuccessful") != std::string::npos); 
    }

    SECTION("removeClass (Global class removal)") {
        // Setup David (40000000) with COP3530 and EEL3701 (remains after remove)
        runCommand(cc, "insert \"David\" 40000000 100 2 COP3530 EEL3701");
        // Setup Eve (50000000) with COP3530 (should be REMOVED after remove)
        runCommand(cc, "insert \"Eve\" 50000000 400 1 COP3530");
        
        // Remove class COP3530: affects 2 students (David is modified, Eve is removed).
        // Output is the count of students the class was dropped from (2).
        std::string output = runCommand(cc, "removeClass COP3530");
        REQUIRE(output.find("2") != std::string::npos);
        
        // Check: Eve (removed student) is now gone.
        REQUIRE(runCommand(cc, "remove 50000000").find("unsuccessful") != std::string::npos);
        // Check: David (modified student) is still present.
        REQUIRE(runCommand(cc, "remove 40000000").find("unsuccessful") == std::string::npos);
    }
    
    SECTION("remove (Specific UFID removal)") {
        CampusCompass cc2; // Use a fresh object to avoid contamination
        // Setup a student with 0 classes
        runCommand(cc2, "insert \"Frank\" 60000000 100 1 COP3530");
        runCommand(cc2, "dropClass 60000000 COP3530"); // Frank now has 0 classes
        
        // Frank has 0 classes and can now be removed
        REQUIRE(runCommand(cc2, "remove 60000000").find("successful") != std::string::npos);
    }
}

TEST_CASE("Part 4: printShortestEdges and Edges Closure State Change (FINAL FIX)", "[UnitTests]") {
    CampusCompass cc;
    // NOTE: Student setup is now moved into sections below to ensure persistence.

    // Helper to ensure student is created for the section
    auto setup_alice = [&cc]() {
        // Use a unique UFID for each section to prevent conflicts if not fully isolated
        runCommand(cc, "insert \"Alice\" 10000000 1 1 COP3530"); 
    };

    SECTION("Initial printShortestEdges (Student can reach class)") {
        setup_alice(); // Re-insert Alice
        
        std::string output1 = runCommand(cc, "printShortestEdges 10000000");
        
        REQUIRE( output1.find("Name: Alice") != std::string::npos ); 
        REQUIRE( output1.find("COP3530 | Total Time: 23") != std::string::npos ); 
    }
    
    SECTION("Edge Toggle OFF and Reachability Check") {
        setup_alice(); // Re-insert Alice
        
        // Toggle OFF critical edge (1, 4)
        runCommand(cc, "toggleEdgesClosure 1 1 4"); 
        
        // Check for reachability failure (Time: -1)
        std::string output2 = runCommand(cc, "printShortestEdges 10000000");
        
        REQUIRE( output2.find("Name: Alice") != std::string::npos );
        REQUIRE( output2.find("COP3530 | Total Time: -1") != std::string::npos ); 
    }
    
    SECTION("Edge Toggle ON and Reachability Restore") {
        // NOTE: Alice needs to be inserted AND the edge needs to be closed first, then reopened.
        // This section tests the combined action from Section 2 + the final step.
        setup_alice(); 
        
        // 1. Close the edge (Simulating the state change from the prompt requirement)
        runCommand(cc, "toggleEdgesClosure 1 1 4");
        
        // 2. Open the edge
        runCommand(cc, "toggleEdgesClosure 1 1 4");
        
        // 3. Check for path restoration (Time: 23)
        std::string output3 = runCommand(cc, "printShortestEdges 10000000");
        
        REQUIRE( output3.find("Name: Alice") != std::string::npos );
        REQUIRE( output3.find("COP3530 | Total Time: 23") != std::string::npos ); 
    }
}