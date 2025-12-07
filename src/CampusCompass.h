#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <utility>
#include <functional> // Required for std::hash

using namespace std;

// Custom hash for pair<int,int> to be used as a key in unordered_map.
// This is essential for edgesMetadata.
struct EdgeKeyHash {
    size_t operator()(const pair<int,int>& p) const noexcept {
        // Use bit shifting to combine the two integers into a single 64-bit value for hashing.
        unsigned long long a = static_cast<unsigned long long>(p.first);
        unsigned long long b = static_cast<unsigned long long>(p.second);
        unsigned long long combined = (a << 32) ^ b;
        return std::hash<unsigned long long>()(combined);
    }
};

// Represents an edge in the adjacency list.
struct Edge {
    int to;
    int weight;
    bool closed; // True if the road is closed/inaccessible.
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}
};

// Stores metadata for a specific class.
struct ClassInfo {
    int locationId;
    string startTime; // e.g., "10:40"
    string endTime;   // e.g., "11:30"
};

// Stores individual student data.
struct Student {
    string name;
    string ufid;
    int residence;
    set<string> classes; // Using a set for efficient O(logN) lookups and insertions.
};

// Disjoint Set Union (DSU) structure for Kruskal's algorithm (MST).
struct DSU {
    unordered_map<int,int> p; // Parent map: p[child] = parent
    
    // Finds the representative (root) of the set containing x (with path compression).
    int find(int x) {
        if (!p.count(x)) p[x] = x;
        if (p[x] == x) return x;
        return p[x] = find(p[x]);
    }
    
    // Unites the sets containing a and b.
    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a != b) p[b] = a; // Union by rank/size could be added but simple union is sufficient.
    }
};

class CampusCompass {
private:
    // Graph representation: Adjacency list.
    unordered_map<int, vector<Edge>> graph;
    
    // Canonical edge storage: key={min(ID1), max(ID2)}, value={weight, closed status}.
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edgesMetadata;
    
    // Map of class codes to their location and time info.
    unordered_map<string, ClassInfo> classesMap;
    
    // Map of UFIDs to Student objects.
    unordered_map<string, Student> students;

    // --- Data Loading ---
    bool loadEdges(const string &filePath);
    bool loadClasses(const string &filePath);

    // --- Validation Helpers ---
    static bool validateName(const string &name);
    static bool validateUfid(const string &ufid);
    static bool validateClassCode(const string &code);
    static vector<string> splitCsvLine(const string &line);
    
    // Converts time string "HH:MM" to total minutes from midnight (for verifySchedule).
    static int timeToMinutes(const string &hhmm);

    // --- Graph Algorithms ---
    // Dijkstra's: Finds shortest distances from a source to all other reachable nodes.
    unordered_map<int,int> dijkstraDistances(int source) const;
    
    // Dijkstra's: Finds the shortest path node sequence and total distance (time).
    vector<int> dijkstraPathNodes(int source, int target, int &outDist) const;

    // Kruskal's Helpers:
    // Builds a list of edges that only exist between nodes in the given set.
    vector<pair<int,pair<int,int>>> buildSubgraphEdgesForNodes(const set<int>& nodes) const;
    
    // Computes the total cost of the MST using Kruskal's algorithm.
    int kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges, const vector<int>& nodeList) const;

public:
    CampusCompass();
    bool parseCSV(const string &edgesFilePath, const string &classesFilePath);
    bool parseCommand(const string &command);
};