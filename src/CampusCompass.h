#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>

using namespace std;

struct Edge {
    int to;
    int weight;
    bool closed;
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}
};

struct ClassInfo {
    int location_id;
    string start_time; // "HH:MM"
    string end_time;   // "HH:MM"
};

struct Student {
    string name;
    string ufid; // 8 digits as string
    int residence;
    set<string> classes; // class codes
};

class CampusCompass {
private:
    // graph adjacency list: node id -> list of edges
    unordered_map<int, vector<Edge>> graph;
    // helper to find edge index pairs when toggling or checking status
    // store undirected edges in a map keyed by ordered pair (min,max) to weight and closed
    struct EdgeKeyHash {
        size_t operator()(const pair<int,int>&p) const noexcept {
            return std::hash<long long>()(((long long)p.first<<32) ^ (long long)p.second);
        }
    };
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edges_metadata;
    // map class code to ClassInfo
    unordered_map<string, ClassInfo> classes_map;
    // map UFID to Student
    unordered_map<string, Student> students;

    // helpers
    bool LoadEdges(const string &filepath);
    bool LoadClasses(const string &filepath);
    static bool ValidateName(const string &name);
    static bool ValidateUFID(const string &ufid);
    static bool ValidateClassCode(const string &code);
    vector<int> DijkstraPathNodes(int source, int target, int &out_dist) const;
    unordered_map<int,int> DijkstraDistances(int source) const;
    vector<pair<int,pair<int,int>>> BuildSubgraphEdgesForNodes(const set<int>& nodes) const;
    int KruskalMSTCost(int n_nodes, const vector<pair<int,pair<int,int>>>& edges, const vector<int>& node_list) const;
    static int TimeToMinutes(const string &hhmm);
public:
    CampusCompass();
    bool ParseCSV(const string &edges_filepath, const string &classes_filepath);
    bool ParseCommand(const string &command);
};
