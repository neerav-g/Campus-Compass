#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <utility> 
#include <algorithm>

using namespace std;

// --- Helper Data Structures for Hashing and Edges ---

// Structure for hashing a pair of ints, used as a key for edges_metadata map
struct EdgeKeyHash {
    size_t operator()(const pair<int,int>& p) const noexcept {
        // Simple hash combination technique
        return std::hash<long long>()(((long long)p.first << 32) ^ (long long)p.second);
    }
};

// Represents an edge in the graph
struct Edge {
    int to;
    int weight;
    bool closed; // True if the edge is closed (unavailable)
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}
};

// Stores class information
struct ClassInfo {
    int locationId;
    string startTime; // "HH:MM"
    string endTime;   // "HH:MM"
};

// Stores student information
struct Student {
    string name;
    string ufid; 
    int residence;
    set<string> classes; // Stores class codes
};

// --- Main Class Definition ---

class CampusCompass {
private:
    // Core Data Structures
    unordered_map<int, vector<Edge>> graph; // Adjacency List: LocationID -> Edges
    // Canonical edge key (min_id, max_id) -> {weight, closed_status}
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edgesMetadata; 
    unordered_map<string, ClassInfo> classesMap; // ClassCode -> ClassInfo
    unordered_map<string, Student> students; // UFID -> Student

    // --- Private Helper Method Declarations ---

    // Data Loading
    bool loadEdges(const string &filePath);
    bool loadClasses(const string &filePath);
    
    // Input/Data Validation & Utilities
    static bool validateName(const string &name);
    static bool validateUfid(const string &ufid);
    static bool validateClassCode(const string &code);
    static vector<string> splitCsvLine(const string &line); 
    static int timeToMinutes(const string &hhmm);

    // Graph Algorithms
    unordered_map<int,int> dijkstraDistances(int source) const;
    vector<int> dijkstraPathNodes(int source, int target, int &outDist) const;
    
    // Kruskal's MST Helpers
    vector<pair<int,pair<int,int>>> buildSubgraphEdgesForNodes(const set<int>& nodes) const;
    int kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges, const vector<int>& nodeList) const;
    
public:
    CampusCompass();
    bool parseCSV(const string &edgesFilePath, const string &classesFilePath);
    bool parseCommand(const string &command);
};