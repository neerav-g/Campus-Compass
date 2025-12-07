#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <utility>

using namespace std;

struct EdgeKeyHash {
    size_t operator()(const pair<int,int>& p) const noexcept {
        unsigned long long a = static_cast<unsigned long long>(p.first);
        unsigned long long b = static_cast<unsigned long long>(p.second);
        unsigned long long combined = (a << 32) ^ b;
        return std::hash<unsigned long long>()(combined);
    }
};

struct Edge {
    int to;
    int weight;
    bool closed;
    Edge(int t = 0, int w = 0, bool c = false) : to(t), weight(w), closed(c) {}
};

struct ClassInfo {
    int locationId;
    string startTime;
    string endTime;
};

struct Student {
    string name;
    string ufid;
    int residence;
    set<string> classes;
};

struct DSU {
    unordered_map<int,int> p;
    int find(int x) {
        if (!p.count(x)) p[x] = x;
        if (p[x] == x) return x;
        return p[x] = find(p[x]);
    }
    void unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a != b) p[b] = a;
    }
};

class CampusCompass {
private:
    unordered_map<int, vector<Edge>> graph;
    unordered_map<pair<int,int>, pair<int,bool>, EdgeKeyHash> edgesMetadata;
    unordered_map<string, ClassInfo> classesMap;
    unordered_map<string, Student> students;

    bool loadEdges(const string &filePath);
    bool loadClasses(const string &filePath);

    static bool validateName(const string &name);
    static bool validateUfid(const string &ufid);
    static bool validateClassCode(const string &code);
    static vector<string> splitCsvLine(const string &line);
    static int timeToMinutes(const string &hhmm);

    unordered_map<int,int> dijkstraDistances(int source) const;
    vector<int> dijkstraPathNodes(int source, int target, int &outDist) const;

    vector<pair<int,pair<int,int>>> buildSubgraphEdgesForNodes(const set<int>& nodes) const;
    int kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges, const vector<int>& nodeList) const;

public:
    CampusCompass();
    bool parseCSV(const string &edgesFilePath, const string &classesFilePath);
    bool parseCommand(const string &command);
};
