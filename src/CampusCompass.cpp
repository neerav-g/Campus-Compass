#include "CampusCompass.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <queue>
#include <limits>
#include <algorithm>
#include <set>
#include <iterator>

using namespace std;

// --- Disjoint Set Union (DSU) Structure ---
struct DSU {
    unordered_map<int,int> p;

    int find(int x) {
        if (!p.count(x)) p[x] = x;
        if (p[x] == x) return x;
        return p[x] = find(p[x]); // Path compression
    }

    void unite(int a, int b) { 
        a = find(a); 
        b = find(b); 
        if (a != b) p[b] = a; 
    }
};

// --- Constructor ---
CampusCompass::CampusCompass() {
    graph.clear();
    edgesMetadata.clear();
    classesMap.clear();
    students.clear();
}

// --- Static Helper Definitions ---

vector<string> CampusCompass::splitCsvLine(const string &line) {
    // Custom logic to handle CSV/TSV lines and whitespace trimming
    vector<string> out;
    string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ',' || c == '\t') {
            string t = cur;
            size_t a = t.find_first_not_of(" \r\n");
            size_t b = t.find_last_not_of(" \r\n");
            if (a == string::npos) t = "";
            else t = t.substr(a, b - a + 1);
            out.push_back(t);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    // Handle the last field
    string t = cur;
    size_t a = t.find_first_not_of(" \r\n");
    size_t b = t.find_last_not_of(" \r\n");
    if (a == string::npos) t = "";
    else t = t.substr(a, b - a + 1);
    out.push_back(t);
    return out;
}

// FIX: Enhanced Name Validation (Test 1 fix)
bool CampusCompass::validateName(const string &name) {
    if (name.empty()) return false;
    for (char c : name) {
        // Strictly allow only letters and spaces.
        if (!(isalpha((unsigned char)c) || c == ' ')) return false;
    }
    return true;
}

bool CampusCompass::validateUfid(const string &ufid) {
    if (ufid.size() != 8) return false;
    for (char c : ufid) if (!isdigit((unsigned char)c)) return false;
    return true;
}

bool CampusCompass::validateClassCode(const string &code) {
    if (code.size() != 7) return false;
    for (int i = 0; i < 3; ++i) if (!isupper((unsigned char)code[i])) return false;
    for (int i = 3; i < 7; ++i) if (!isdigit((unsigned char)code[i])) return false;
    return true;
}

int CampusCompass::timeToMinutes(const string &hhmm) {
    if (hhmm.size() < 4) return -1;
    int h = 0, m = 0;
    try {
        string sh = hhmm.substr(0, hhmm.find(':'));
        string sm = hhmm.substr(hhmm.find(':')+1);
        h = stoi(sh);
        m = stoi(sm);
    } catch (...) {
        return -1;
    }
    return h * 60 + m;
}

// --- Loading Methods ---

bool CampusCompass::loadEdges(const string &filePath) {
    ifstream fin(filePath);
    if (!fin.is_open()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        
        auto parts = splitCsvLine(line);
        if (parts.size() < 5) continue;
        
        int id1 = 0, id2 = 0, time = 0;
        try {
            id1 = stoi(parts[0]);
            id2 = stoi(parts[1]);
            time = stoi(parts.back());
        } catch (...) {
            continue;
        }
        
        if (parts[0] == "LocationID_1" || id1 == 0 || id2 == 0) continue;

        graph[id1].push_back(Edge(id2, time, false));
        graph[id2].push_back(Edge(id1, time, false));
        
        int a = min(id1, id2);
        int b = max(id1, id2);
        edgesMetadata[{a,b}] = {time, false};
    }
    return true;
}

bool CampusCompass::loadClasses(const string &filePath) {
    ifstream fin(filePath);
    if (!fin.is_open()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        
        auto parts = splitCsvLine(line);
        if (parts.size() < 2) continue;
        
        string code = parts[0];
        if (code == "ClassCode" || code.empty()) continue;
        
        int loc = 0;
        try {
            loc = stoi(parts[1]);
        } catch (...) {
            continue;
        }
        
        ClassInfo ci;
        ci.locationId = loc;
        if (parts.size() >= 4) {
            ci.startTime = parts[2];
            ci.endTime = parts[3];
        } else {
            ci.startTime = "";
            ci.endTime = "";
        }
        classesMap[code] = ci;
    }
    return true;
}

bool CampusCompass::parseCSV(const string &edgesFilePath, const string &classesFilePath) {
    bool a = loadEdges(edgesFilePath);
    bool b = loadClasses(classesFilePath);
    return a && b;
}

// --- Graph Algorithms ---

vector<int> CampusCompass::dijkstraPathNodes(int source, int target, int &outDist) const {
    unordered_map<int,int> dist;
    unordered_map<int,int> prev;
    struct Node { int id; int d; };
    struct Cmp { bool operator()(const Node&a, const Node&b) const { return a.d > b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;
    
    dist[source] = 0;
    pq.push({source, 0});
    
    while (!pq.empty()) {
        auto cur = pq.top(); pq.pop();
        int u = cur.id;
        int d = cur.d;
        
        if (dist.count(u) && d > dist.at(u)) continue;
        if (u == target) break;
        
        auto it = graph.find(u);
        if (it == graph.end()) continue;
        
        for (const Edge &e : it->second) {
            if (e.closed) continue; // Skip closed edges
            int v = e.to;
            int newDist = d + e.weight;
            if (!dist.count(v) || newDist < dist.at(v)) {
                dist[v] = newDist;
                prev[v] = u;
                pq.push({v, newDist});
            }
        }
    }
    
    if (!dist.count(target)) {
        outDist = -1;
        return {};
    }
    
    outDist = dist.at(target);
    vector<int> path;
    int cur = target;
    while (true) {
        path.push_back(cur);
        if (cur == source) break;
        if (!prev.count(cur)) { outDist = -1; return {}; }
        cur = prev[cur];
    }
    reverse(path.begin(), path.end());
    return path;
}

unordered_map<int,int> CampusCompass::dijkstraDistances(int source) const {
    unordered_map<int,int> dist;
    struct Node { int id; int d; };
    struct Cmp { bool operator()(const Node&a, const Node&b) const { return a.d > b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;
    
    dist[source] = 0;
    pq.push({source,0});

    while (!pq.empty()) {
        auto cur = pq.top(); pq.pop();
        int u = cur.id;
        int d = cur.d;
        
        if (dist.count(u) && d > dist.at(u)) continue;

        auto it = graph.find(u);
        if (it == graph.end()) continue;
        
        for (const Edge &e : it->second) {
            if (e.closed) continue; 
            int v = e.to;
            int newDist = d + e.weight;
            if (!dist.count(v) || newDist < dist.at(v)) {
                dist[v] = newDist;
                pq.push({v, newDist});
            }
        }
    }
    return dist;
}

vector<pair<int,pair<int,int>>> CampusCompass::buildSubgraphEdgesForNodes(const set<int>& nodes) const {
    vector<pair<int,pair<int,int>>> out;
    for (auto &kv : edgesMetadata) {
        int u = kv.first.first;
        int v = kv.first.second;
        int w = kv.second.first;
        bool closed = kv.second.second;
        
        if (closed) continue; 
        
        if (nodes.count(u) && nodes.count(v)) {
            out.push_back({w, {u, v}});
        }
    }
    return out;
}

int CampusCompass::kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges, const vector<int>& nodeList) const {
    vector<pair<int,pair<int,int>>> sortedEdges = edges;
    sort(sortedEdges.begin(), sortedEdges.end(),
        [](const pair<int, pair<int, int>>& a,
           const pair<int, pair<int, int>>& b) {
            return a.first < b.first;
        }
    );
    
    DSU dsu;
    for (int node : nodeList) dsu.p[node] = node;
    
    int components = nodeList.size();
    if (components <= 1) return 0;
    
    int cost = 0;
    for (auto &e : sortedEdges) {
        int w = e.first;
        int u = e.second.first;
        int v = e.second.second;
        
        if (dsu.find(u) != dsu.find(v)) {
            dsu.unite(u, v);
            cost += w;
            components--;
            if (components == 1) break; 
        }
    }
    return cost;
}

// --- Command Parsing Logic ---

bool CampusCompass::parseCommand(const string &command) {
    if (command.empty()) {
        cout << "unsuccessful\n";
        return false;
    }
    
    // Command Count/Numeric Line Handling
    try {
        size_t firstNonSpace = command.find_first_not_of(" \t\r\n");
        if (firstNonSpace != string::npos && 
            isdigit((unsigned char)command[firstNonSpace]) && 
            command.find_first_not_of("0123456789 \t\r\n", firstNonSpace) == string::npos) {
            stoi(command);
            cout << "successful\n"; 
            return true;
        }
    } catch (...) {}
    
    istringstream iss(command);
    string cmd;
    iss >> cmd;

    if (cmd == "insert") {
        string rest = command.substr(6);
        size_t q1 = rest.find('"');
        size_t q2 = string::npos;
        if (q1 != string::npos) q2 = rest.find('"', q1+1);
        
        if (q1 == string::npos || q2 == string::npos) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        string name = rest.substr(q1+1, q2 - q1 - 1);
        string after = rest.substr(q2+1);
        
        istringstream aiss(after);
        string ufid; 
        int residence; 
        int numClasses;

        // Reads UFID, residence, and class count
        if (!(aiss >> ufid >> residence >> numClasses)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        // Validate basic inputs (name characters, UFID length/type, class count range)
        if (!validateName(name) || !validateUfid(ufid) || numClasses < 1 || numClasses > 6) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        // --- CORRECTED LOGIC: Check if Residence ID is a valid node in the graph ---
        if (!graph.count(residence)) {
            cout << "unsuccessful\n";
            return false;
        }
        // -------------------------------------------------------------------------

        vector<string> classList;
        string code;
        for (int i = 0; i < numClasses; ++i) {
            if (!(aiss >> code)) { 
                cout << "unsuccessful\n"; 
                return false; 
            }
            // Validate class code format and check if it exists in map
            if (!validateClassCode(code) || !classesMap.count(code)) { 
                cout << "unsuccessful\n"; 
                return false; 
            }
            classList.push_back(code);
        }
        
        // Final check: Duplicate UFID
        if (students.count(ufid)) { 
            cout << "unsuccessful\n"; 
            return false; 
        } 

        Student s;
        s.name = name;
        s.ufid = ufid;
        s.residence = residence;
        for (const auto &c : classList) s.classes.insert(c);
        students[ufid] = s;
        cout << "successful\n";
        return true;

    } else if (cmd == "remove") {
        string id;
        if (!(iss >> id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        if (students.count(id)) { 
            if (students.at(id).classes.empty()) {
                students.erase(id);
                cout << "successful\n";
                return true;
            } else {
                cout << "unsuccessful\n"; 
                return false;
            }
        } else {
            cout << "unsuccessful\n"; 
            return false;
        }

    } else if (cmd == "dropClass") {
        string id, code;
        if (!(iss >> id >> code)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        if (!students.count(id) || !validateClassCode(code) || !classesMap.count(code)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        Student &s = students.at(id);
        if (!s.classes.count(code)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        s.classes.erase(code);
        
        if (s.classes.empty()) {
            students.erase(id);
        }
        
        cout << "successful\n";
        return true;

    } else if (cmd == "replaceClass") {
        string id, codeA, codeB;
        if (!(iss >> id >> codeA >> codeB)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        if (!students.count(id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        if (!validateClassCode(codeA) || !validateClassCode(codeB)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        if (!classesMap.count(codeB)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        Student &s = students.at(id);
        // Must have codeA and must NOT have codeB
        if (!s.classes.count(codeA) || s.classes.count(codeB)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        s.classes.erase(codeA);
        s.classes.insert(codeB);
        cout << "successful\n";
        return true;

    } else if (cmd == "removeClass") {
        string code;
        if (!(iss >> code)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        if (!validateClassCode(code) || !classesMap.count(code)) { 
            cout << "0\n"; 
            return true; 
        } 
        
        int removedCount = 0;
        
        // Loop safely iterates and removes students if class count hits zero.
        for (auto it = students.begin(); it != students.end(); ) {
            Student &s = it->second;

            if (s.classes.count(code)) {
                s.classes.erase(code);
                
                if (s.classes.empty()) {
                    removedCount++;
                    it = students.erase(it); // Erase student and advance iterator
                } else {
                    it++; // Only dropped the class, keep student
                }
            } else {
                ++it;
            }
        }
        
        cout << removedCount << "\n";
        return true;

    } else if (cmd == "toggleEdgesClosure") {
        int count;
        if (!(iss >> count)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        for (int i = 0; i < count; ++i) {
            int a, b;
            if (!(iss >> a >> b)) { 
                cout << "unsuccessful\n";
                return false; 
            }
            
            int u = min(a,b), v = max(a,b);
            auto it = edgesMetadata.find({u,v});
            
            if (it == edgesMetadata.end()) {
                cout << "unsuccessful\n";
                return false;
            }
            
            bool newClosed = !it->second.second;
            
            // Update metadata
            it->second.second = newClosed;

            // Update both directions in the adjacency list
            auto update_adj_list = [&](int start, int end, bool closed_status) {
                if (graph.count(start)) {
                    for (auto &e : graph.at(start)) { 
                        if (e.to == end) { 
                            e.closed = closed_status; 
                            break; 
                        } 
                    }
                }
            };
            
            update_adj_list(a, b, newClosed);
            update_adj_list(b, a, newClosed);
        }
        
        cout << "successful\n";
        return true;

    } else if (cmd == "checkEdgeStatus") {
        int a,b;
        if (!(iss >> a >> b)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        int u = min(a,b), v = max(a,b);
        auto it = edgesMetadata.find({u,v});
        
        if (it == edgesMetadata.end()) {
            cout << "DNE\n";
        } else {
            if (it->second.second) cout << "closed\n"; else cout << "open\n";
        }
        return true;

    } else if (cmd == "isConnected") {
        int a,b;
        if (!(iss >> a >> b)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        if (!graph.count(a) || !graph.count(b)) {
            cout << "unsuccessful\n";
            return false;
        }

        set<int> visited;
        queue<int> q;
        q.push(a);
        visited.insert(a);
        bool found = false;

        while (!q.empty()) {
            int u = q.front(); q.pop();
            if (u == b) { found = true; break; }

            auto it = graph.find(u);
            if (it == graph.end()) continue;

            for (auto &e : it->second) {
                if (e.closed) continue;
                if (!visited.count(e.to)) {
                    visited.insert(e.to);
                    q.push(e.to);
                }
            }
        }
        if (found) cout << "successful\n"; else cout << "unsuccessful\n";
        return true;

    } else if (cmd == "printShortestEdges") {
        string id;
        if (!(iss >> id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        if (!students.count(id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        Student &s = students.at(id);
        cout << "Name: " << s.name << "\n";
        
        vector<string> codes(s.classes.begin(), s.classes.end());
        sort(codes.begin(), codes.end());

        auto dists = dijkstraDistances(s.residence); 
        
        for (const auto &c : codes) {
            int totalTime = -1;
            if (classesMap.count(c)) {
                int loc = classesMap.at(c).locationId;
                if (dists.count(loc)) totalTime = dists.at(loc);
            }
            cout << c << " | Total Time: " << totalTime << "\n";
        }
        return true;

    } else if (cmd == "printStudentZone") {
        string id;
        if (!(iss >> id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        if (!students.count(id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        Student &s = students.at(id);
        set<int> nodes;

        for (const auto &c : s.classes) {
            if (!classesMap.count(c)) continue;
            int loc = classesMap.at(c).locationId;
            int outDist;
            auto path = dijkstraPathNodes(s.residence, loc, outDist); 
            if (outDist < 0) continue;
            for (int v : path) nodes.insert(v); 
        }
        nodes.insert(s.residence);

        auto subEdges = buildSubgraphEdgesForNodes(nodes);
        vector<int> nodeList(nodes.begin(), nodes.end());
        
        int cost = 0;
        if (nodeList.size() >= 1) { 
            cost = kruskalMSTCost(subEdges, nodeList);
        }
        
        cout << "Student Zone Cost For " << s.name << ": " << cost << "\n"; 
        return true;

    } else if (cmd == "verifySchedule") {
        string id;
        if (!(iss >> id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        if (!students.count(id)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        Student &s = students.at(id);
        
        if (s.classes.size() <= 1) { 
            cout << "unsuccessful\n"; 
            return true; 
        }

        struct ClassRecord { string code; int start; int end; int loc; };
        vector<ClassRecord> records;

        for (const auto &c : s.classes) {
            if (!classesMap.count(c)) continue;
            ClassInfo ci = classesMap.at(c);
            int startMinutes = timeToMinutes(ci.startTime);
            int endMinutes = timeToMinutes(ci.endTime);
            
            if (startMinutes < 0 || endMinutes < 0) continue; 
            records.push_back({c, startMinutes, endMinutes, ci.locationId});
        }
        
        sort(records.begin(), records.end(), [](const ClassRecord& a, const ClassRecord& b){
            return a.start < b.start;
        });
        
        if (records.size() <= 1) { 
            cout << "unsuccessful\n"; 
            return true; 
        }

        cout << "Schedule Check for " << s.name << ":\n";
        
        for (size_t i = 0; i + 1 < records.size(); ++i) {
            int from = records[i].loc;
            int to = records[i+1].loc;
            int shortestTime = -1;
            int outDist;
            
            dijkstraPathNodes(from, to, outDist); 
            if (outDist >= 0) shortestTime = outDist;
            
            int timeGap = records[i+1].start - records[i].end;
            
            if (shortestTime <= timeGap && shortestTime >= 0) {
                cout << records[i].code << " - " << records[i+1].code << " \"Can make it!\"\n";
            } else {
                cout << records[i].code << " - " << records[i+1].code << " \"Cannot make it!\"\n";
            }
        }
        return true;
    } else {
        cout << "unsuccessful\n";
        return false;
    }
}