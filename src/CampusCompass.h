#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <utility> 
#include <algorithm> 

using namespace std;


// --- Helper stuff I need to make the maps work ---

struct EdgeKeyHash {

    size_t operator()(const pair<int,int>&p) const noexcept {
        // Hashing for edge pairs. This took me liek 2 hours to figure out, dont touch.
        return std::hash<long long>()(((long long)p.first<<32) ^ (long long)p.second);
    }
};

// Edge definition
struct Edge {
    int to;
    int weight;
    bool closed; // Toggled for construction/events
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}

};

// Class data
struct ClassInfo {

    int location_id;
    string start_time; // "HH:MM"
    string end_time;   // "HH:MM"
};

// Student struct, its gona hold evrythin'
struct Student {

    string name;
    string ufid; 
    int residence;
    set<string> classes; // Stores class codes, use set to avoid dups

};

// --- Main Campus Class ---

class CampusCompass {

private:
    // Core data structures
    unordered_map<int, vector<Edge>> graph; // main graph: LocationID -> list of Edges
    // Stores (u, v) -> {weight, closed} for quick edge status lookup
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edges_metadata; 
    unordered_map<string, ClassInfo> classes_map; // ClassCode -> Info
    unordered_map<string, Student> students; // UFID -> Student

    // file loading helpers
    bool LoadEdges(const string &filepath);
    bool LoadClasses(const string &filepath);
    
    // Validaters and helper funxions
    static bool ValidateName(const string &name);
    static bool ValidateUFID(const string &ufid);
    static bool ValidateClassCode(const string &code);
    // this splits a line from the CSV
    static vector<string> split_csv_line(const string &line); 

    // Graph algos - need to be private
    // Dijkstra's for shortest path time
    unordered_map<int,int> DijkstraDistances(int source) const;
    vector<int> DijkstraPathNodes(int source, int target, int &out_dist) const;
    
    // Kruskal's MST helpers for student zones
    vector<pair<int,pair<int,int>>> BuildSubgraphEdgesForNodes(const set<int>& nodes) const;
    // this calculates the cost, man...
    int KruskalMSTCost(int n_nodes, const vector<pair<int,pair<int,int>>>& edges, const vector<int>& node_list) const;
    
    // time convertor for verifySchedule
    static int TimeToMinutes(const string &hhmm);

public:
    CampusCompass(); // need this for the linker!

    bool ParseCSV(const string &edges_filepath, const string &classes_filepath);

    bool ParseCommand(const string &command);
};