// #define CATCH_CONFIG_MAIN
// #include <catch2/catch_test_macros.hpp>
// #include "CampusCompass.h"
// #include <sstream>
// #include <fstream>
// #include <cstdio>

// using namespace std;


// TEST_CASE("Test 1: Invalid Commands", "[CampusCompass]") {
//     CampusCompass cc;
//     cc.parseCSV("edges.csv", "classes.csv");

//     // Five invalid commands
//     REQUIRE(cc.parseCommand(R"(insert "A11y" 45679999 1 1 COP3530)") == false);
//     REQUIRE(cc.parseCommand(R"(insert "Bob!" 12345678 2 1 COP3502)") == false);
//     REQUIRE(cc.parseCommand(R"(insert "Alice" 1234ABCD 1 1 COP3502)") == false);
//     REQUIRE(cc.parseCommand(R"(replaceClass 12345678 COP3502 COP3505)") == false);
//     REQUIRE(cc.parseCommand(R"(dropClass 00000000 COP3502)") == false);
// }

// TEST_CASE("Test 2: Edge Cases", "[CampusCompass]") {
//     CampusCompass cc;
//     cc.parseCSV("edges.csv", "classes.csv");

//     // Remove student that doesn't exist
//     REQUIRE(cc.parseCommand("remove 99999999") == false);
//     // Toggle edge that doesn't exist
//     REQUIRE(cc.parseCommand("toggleEdgesClosure 1 999 1000") == false);
//     // Check edge status for non-existent edge
//     REQUIRE(cc.parseCommand("checkEdgeStatus 1 999") == true); // prints DNE
// }

// TEST_CASE("Test 3: Student Class Commands", "[CampusCompass]") {
//     CampusCompass cc;
//     cc.parseCSV("edges.csv", "classes.csv");

//     // Insert student with two classes
//     REQUIRE(cc.parseCommand(R"(insert "Bob" 12345678 1 2 COP3530 COP3502)") == true);

//     // Drop class COP3502, student should still exist
//     REQUIRE(cc.parseCommand("dropClass 12345678 COP3502") == true);

//     // Drop class COP3530, student should be removed automatically
//     REQUIRE(cc.parseCommand("dropClass 12345678 COP3530") == true);

//     // Re-insert for replaceClass test
//     REQUIRE(cc.parseCommand(R"(insert "Bob" 12345678 1 2 COP3530 COP3502)") == true);
//     REQUIRE(cc.parseCommand("replaceClass 12345678 COP3502 COP3503") == true);

//     // Attempt to remove student with classes should fail
//     REQUIRE(cc.parseCommand("remove 12345678") == false);

//     // RemoveClass COP3530 should remove COP3530 from Bob
//     REQUIRE(cc.parseCommand("removeClass COP3530") == true);

//     // Now Bob should only have COP3503
//     REQUIRE(cc.parseCommand("remove 12345678") == false); // still has 1 class
// }

// TEST_CASE("Test 4: printShortestEdges with Edge Closure", "[CampusCompass]") {
//     CampusCompass cc;
//     cc.parseCSV("edges.csv", "classes.csv");

//     // Insert student located at Hume Hall (1) with class COP3530 at location 14
//     REQUIRE(cc.parseCommand(R"(insert "Bob" 12345678 1 1 COP3530)") == true);

//     // Initial shortest paths should be valid
//     REQUIRE(cc.parseCommand("printShortestEdges 12345678") == true);

//     // Close edge 1-4 (Hume Hall <-> Rawlings Hall)
//     REQUIRE(cc.parseCommand("toggleEdgesClosure 1 1 4") == true);

//     // After edge closure, printShortestEdges should reflect longer path or unreachable
//     REQUIRE(cc.parseCommand("printShortestEdges 12345678") == true);

//     // Reopen the edge
//     REQUIRE(cc.parseCommand("toggleEdgesClosure 1 1 4") == true);

//     // Should be reachable again
//     REQUIRE(cc.parseCommand("printShortestEdges 12345678") == true);
// }