#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <utility> 
#include <algorithm> // for min/max

using namespace std;

// --- Helper Structures ---

// Structure for hashing a pair of ints for edges_metadata map key
struct EdgeKeyHash {
    size_t operator()(const pair<int,int>&p) const noexcept {
        return std::hash<long long>()(((long long)p.first<<32) ^ (long long)p.second);
    }
};

// Structure for Adjacency List (Edge)
struct Edge {
    int to;
    int weight;
    bool closed; // True if the edge is closed
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}
};

// Structure for Class Data
struct ClassInfo {
    int location_id;
    string start_time; // "HH:MM"
    string end_time;   // "HH:MM"
};

// Structure for Student Data
struct Student {
    string name;
    string ufid; 
    int residence;
    set<string> classes; // Stores class codes
};

// --- Main Class ---

class CampusCompass {
private:
    // Core Data Structures
    unordered_map<int, vector<Edge>> graph; // Graph: LocationID -> list of Edges
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edges_metadata; // (min_id, max_id) -> {weight, closed}
    unordered_map<string, ClassInfo> classes_map; // ClassCode -> ClassInfo
    unordered_map<string, Student> students; // UFID -> Student

    // Data Loading
    bool LoadEdges(const string &filepath);
    bool LoadClasses(const string &filepath);
    
    // Input/Data Validation & Helpers
    static bool ValidateName(const string &name);
    static bool ValidateUFID(const string &ufid);
    static bool ValidateClassCode(const string &code);
    static vector<string> split_csv_line(const string &line); 

    // Graph Algorithms and Helpers
    unordered_map<int,int> DijkstraDistances(int source) const;
    vector<int> DijkstraPathNodes(int source, int target, int &out_dist) const;
    
    // Kruskal's MST Helpers for printStudentZone
    vector<pair<int,pair<int,int>>> BuildSubgraphEdgesForNodes(const set<int>& nodes) const;
    int KruskalMSTCost(int n_nodes, const vector<pair<int,pair<int,int>>>& edges, const vector<int>& node_list) const;
    
    // Time Helper
    static int TimeToMinutes(const string &hhmm);

public:
    CampusCompass(); // Constructor (Crucial for linking!)
    bool ParseCSV(const string &edges_filepath, const string &classes_filepath);
    bool ParseCommand(const string &command);
};