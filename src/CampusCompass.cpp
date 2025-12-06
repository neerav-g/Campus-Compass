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

// --- DSU Structure for Kruskal's (Disjoint Set Union) ---
// I need this to check for cycles in Kruskal's
struct DSU {
    unordered_map<int,int> p;
    int find(int x) {
        if (!p.count(x)) p[x] = x;
        if (p[x] == x) return x;
        // path compression, its good for speed (O(a(n)))
        return p[x] = find(p[x]); 
    }
    void unite(int a, int b) { 
        a = find(a); 
        b = find(b); 
        if (a != b) p[b] = a; 
        // no union by rank/size, being lazy, its fine
    }
};

// --- FIX: Define the constructor or the linker will yell ---
CampusCompass::CampusCompass() {
    // just clearing maps is prob good enough
    graph.clear();
    edges_metadata.clear();
    classes_map.clear();
    students.clear();
}

// --- Static Helper Definitions ---

// My custom CSV splitter. Does not handle quoted commas, oh well.
vector<string> CampusCompass::split_csv_line(const string &line) {
    vector<string> out;
    string cur;
    // a simple loop is easier to read
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ',' || c == '\t') {
            string t = cur;
            // trim leading/trailing whitespace, this part is gross
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
    // dont forget the last field
    string t = cur;
    size_t a = t.find_first_not_of(" \r\n");
    size_t b = t.find_last_not_of(" \r\n");
    if (a == string::npos) t = "";
    else t = t.substr(a, b - a + 1);
    out.push_back(t);
    return out;
}

bool CampusCompass::ValidateName(const string &name) {
    if (name.empty()) return false;
    // shud be alpha or space, nothing else
    for (char c : name) {
        if (!(isalpha((unsigned char)c) || c == ' ')) return false;
    }
    return true;
}

bool CampusCompass::ValidateUFID(const string &ufid) {
    // must be 8 digits long
    if (ufid.size() != 8) return false;
    for (char c : ufid) if (!isdigit((unsigned char)c)) return false;
    return true;
}

bool CampusCompass::ValidateClassCode(const string &code) {
    // 3 caps, 4 digits, 7 total
    if (code.size() != 7) return false;
    for (int i = 0; i < 3; ++i) if (!isupper((unsigned char)code[i])) return false;
    for (int i = 3; i < 7; ++i) if (!isdigit((unsigned char)code[i])) return false;
    return true;
}

int CampusCompass::TimeToMinutes(const string &hhmm) {
    if (hhmm.size() < 4) return -1;
    int h = 0, m = 0;
    try {
        string sh = hhmm.substr(0, hhmm.find(':'));
        string sm = hhmm.substr(hhmm.find(':')+1);
        h = stoi(sh);
        m = stoi(sm);
    } catch (...) {
        return -1; // error if parsing fails
    }
    return h * 60 + m;
}

// --- Loading Methods ---

bool CampusCompass::LoadEdges(const string &filepath) {
    ifstream fin(filepath);
    if (!fin.is_open()) return false;
    string line;

    // not skipping the first line header here, will skip it later when i check parts[0]
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        
        auto parts = split_csv_line(line);
        if (parts.size() < 5) continue; // skip if too short
        
        int id1 = 0, id2 = 0, time = 0;
        try {
            id1 = stoi(parts[0]);
            id2 = stoi(parts[1]);
            time = stoi(parts.back()); // Time is the last column
        } catch (...) {
            continue;
        }
        
        // Skip the header line and invalid IDs
        if (parts[0] == "LocationID_1" || id1 == 0 || id2 == 0) continue;

        // Add edge in both directions
        graph[id1].push_back(Edge(id2, time, false));
        graph[id2].push_back(Edge(id1, time, false));

        // Store edge metadata using canonical form (min, max) for easy lookup
        int a = min(id1, id2);
        int b = max(id1, id2);
        edges_metadata[{a,b}] = {time, false}; // default to open
    }
    return true;
}

bool CampusCompass::LoadClasses(const string &filepath) {
    ifstream fin(filepath);
    if (!fin.is_open()) return false;
    string line;
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        
        auto parts = split_csv_line(line);
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
        ci.location_id = loc;
        // Times are there but maybe empty
        if (parts.size() >= 4) {
            ci.start_time = parts[2];
            ci.end_time = parts[3];
        } else {
            ci.start_time = "";
            ci.end_time = "";
        }
        classes_map[code] = ci;
    }
    return true;
}

bool CampusCompass::ParseCSV(const string &edges_filepath, const string &classes_filepath) {
    bool a = LoadEdges(edges_filepath);
    bool b = LoadClasses(classes_filepath);
    return a && b; // gotta load both
}

// --- Graph Algorithms (Dijkstra/Kruskal) ---

// returns the full path, which is more work than just the distance, but oh well
vector<int> CampusCompass::DijkstraPathNodes(int source, int target, int &out_dist) const {
    
    // INF is huge, but not too huge to overflow when adding
    // int INF = numeric_limits<int>::max() / 4; // unnecessary, but safe
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
        // this checks if we found a better path already, inefficient to keep old ones, but needed
        if (dist.count(u) && d > dist.at(u)) continue; 
        if (u == target) break;
        
        auto it = graph.find(u);
        if (it == graph.end()) continue;
        
        for (const Edge &e : it->second) {
            if (e.closed) continue; // cant use closed roads, duh
            int v = e.to;
            int nd = d + e.weight;
            if (!dist.count(v) || nd < dist.at(v)) {
                dist[v] = nd;
                prev[v] = u;
                pq.push({v, nd});
            }
        }
    }

    if (!dist.count(target)) {
        out_dist = -1;
        return {}; // no path
    }

    out_dist = dist.at(target);
    vector<int> path;
    int cur = target;
    // Reconstruct the path backwards from the target
    while (true) {
        path.push_back(cur);
        if (cur == source) break;
        if (!prev.count(cur)) { 
            out_dist = -1; 
            return {}; // should not happen if dist.count(target) is true, but safer
        }
        cur = prev[cur];
    }
    reverse(path.begin(), path.end());
    return path;
}

// I need this one just for distances for printShortestEdges, no path tracking, slightly faster
unordered_map<int,int> CampusCompass::DijkstraDistances(int source) const {
    unordered_map<int,int> dist;
    // same setup as before...
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
            int nd = d + e.weight;
            if (!dist.count(v) || nd < dist.at(v)) {
                dist[v] = nd;
                pq.push({v, nd});
            }
        }
    }
    return dist;
}

// Helper to pull out only the edges connecting the nodes in the zone
vector<pair<int,pair<int,int>>> CampusCompass::BuildSubgraphEdgesForNodes(const set<int>& nodes) const {
    vector<pair<int,pair<int,int>>> out;
    // Iterate over all metadata, which is O(E), instead of just iterating over the nodes and checking adjacencies, which is sometimes faster
    for (auto &kv : edges_metadata) {
        int u = kv.first.first;
        int v = kv.first.second;
        int w = kv.second.first;
        bool closed = kv.second.second;
        if (closed) continue; 
        // only include edges where BOTH endpoints are in the path nodes set
        if (nodes.count(u) && nodes.count(v)) {
            out.push_back({w, {u, v}});
        }
    }
    return out;
}

// Kruskal's for MST cost. I used a custom DSU struct for this.
int CampusCompass::KruskalMSTCost(int n_nodes, const vector<pair<int,pair<int,int>>>& edges, const vector<int>& node_list) const {
    vector<pair<int,pair<int,int>>> es = edges;
    // Sort edges by weight
    sort(es.begin(), es.end(),
        [](const pair<int, pair<int, int>>& a,
           const pair<int, pair<int, int>>& b) {
            return a.first < b.first;
        }
    );
    DSU dsu;
    // Init DSU for all nodes in the list
    for (int node : node_list) dsu.p[node] = node;
    
    int comps = node_list.size();
    if (comps <= 1) return 0; // trivial case
    
    int cost = 0;
    for (auto &e : es) {
        int w = e.first;
        int u = e.second.first;
        int v = e.second.second;
        // Check if adding edge creates a cycle (already in same set)
        if (dsu.find(u) != dsu.find(v)) {
            dsu.unite(u, v);
            cost += w;
            comps--;
            if (comps == 1) break; // MST complete when all nodes are connected
        }
    }
    return cost;
}

// --- COMMAND PARSING SECTION ---

bool CampusCompass::ParseCommand(const string &command) {
    if (command.empty()) {
        cout << "unsuccessful\n";
        return false;
    }
    
    // This is for the first line command count, I should probably remove this part from here
    try {
        size_t first_non_space = command.find_first_not_of(" \t\r\n");
        if (first_non_space != string::npos && 
            isdigit((unsigned char)command[first_non_space]) && 
            command.find_first_not_of("0123456789 \t\r\n", first_non_space) == string::npos) {
            // Check if it's purely a number
            stoi(command);
            cout << "successful\n"; 
            return true;
        }
    } catch (...) {} // ignore parsing errors
    
    istringstream iss(command);
    string cmd;
    iss >> cmd;

    if (cmd == "insert") {
        // parsing the quoted name is annoying
        string rest = command.substr(6);
        size_t q1 = rest.find('"');
        size_t q2 = string::npos;
        if (q1 != string::npos) q2 = rest.find('"', q1+1);
        
        if (q1 == string::npos || q2 == string::npos) { cout << "unsuccessful\n"; return false; }

        string name = rest.substr(q1+1, q2 - q1 - 1);
        string after = rest.substr(q2+1);
        
        istringstream aiss(after);
        string ufid; int residence; int nclasses;

        if (!(aiss >> ufid >> residence >> nclasses)) { cout << "unsuccessful\n"; return false; }

        // Validation checks
        if (!ValidateName(name) || !ValidateUFID(ufid) || nclasses < 1 || nclasses > 6) { 
            cout << "unsuccessful\n"; 
            return false; 
        }

        vector<string> classlist;
        string code;
        for (int i = 0; i < nclasses; ++i) {
            if (!(aiss >> code)) { 
                cout << "unsuccessful\n"; 
                return false; // not enough classes provided
            }
            if (!ValidateClassCode(code) || !classes_map.count(code)) { 
                cout << "unsuccessful\n"; 
                return false; // bad class code or class DNE
            }
            classlist.push_back(code);
        }
        
        // Final check
        if (students.count(ufid)) { cout << "unsuccessful\n"; return false; }

        Student s;
        s.name = name;
        s.ufid = ufid;
        s.residence = residence;
        for (auto &c : classlist) s.classes.insert(c);
        students[ufid] = s;
        cout << "successful\n";
        return true;

    } else if (cmd == "remove") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        
        // Must exist AND have zero classes to remove
        if (students.count(id)) { 
            if (students.at(id).classes.empty()) {
                students.erase(id);
                cout << "successful\n";
                return true;
            } else {
                cout << "unsuccessful\n"; // has classes
                return false;
            }
        } else {
            cout << "unsuccessful\n"; // DNE
            return false;
        }

    } else if (cmd == "dropClass") {
        string id, code;
        if (!(iss >> id >> code)) { cout << "unsuccessful\n"; return false; }

        // Validate existence of everything
        if (!students.count(id) || !ValidateClassCode(code) || !classes_map.count(code)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        Student &s = students.at(id);
        if (!s.classes.count(code)) { cout << "unsuccessful\n"; return false; }
        
        s.classes.erase(code);
        
        // Remove student if classes list is now empty (as per requirement)
        if (s.classes.empty()) {
            students.erase(id);
        }
        
        cout << "successful\n";
        return true;

    } else if (cmd == "replaceClass") {
        string id, a, b;
        if (!(iss >> id >> a >> b)) { cout << "unsuccessful\n"; return false; }

        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        if (!ValidateClassCode(a) || !ValidateClassCode(b)) { cout << "unsuccessful\n"; return false; }
        if (!classes_map.count(b)) { cout << "unsuccessful\n"; return false; }

        Student &s = students.at(id);
        // Student must have class A, and NOT have class B
        if (!s.classes.count(a)) { cout << "unsuccessful\n"; return false; }
        if (s.classes.count(b)) { cout << "unsuccessful\n"; return false; }

        s.classes.erase(a);
        s.classes.insert(b);
        cout << "successful\n";
        return true;

    } else if (cmd == "removeClass") {
        string code;
        if (!(iss >> code)) { cout << "unsuccessful\n"; return false; }
        
        // Print 0 if the class code is bad or doesnt exist
        if (!ValidateClassCode(code) || !classes_map.count(code)) { 
            cout << "0\n"; 
            return true; 
        } 
        
        int removed_count = 0;
        
        // Iterate and modify/erase. Using iterator is safest
        for (auto it = students.begin(); it != students.end(); ) {
            Student &s = it->second;

            if (s.classes.count(code)) {
                removed_count++;
                s.classes.erase(code);
                
                // Remove student if they have 0 classes now
                if (s.classes.empty()) {
                    it = students.erase(it); // erase returns iterator to next element
                } else {
                    ++it;
                }
            } else {
                ++it;
            }
        }
        
        cout << removed_count << "\n";
        return true;

    } else if (cmd == "toggleEdgesClosure") {
        
        int count; // N, the number of edges to follow
        if (!(iss >> count)) { 
            cout << "unsuccessful\n"; 
            return false; 
        }
        
        // Loop N times to process N edges
        for (int i = 0; i < count; ++i) {
            int a, b;
            if (!(iss >> a >> b)) { 
                cout << "unsuccessful\n";
                return false; // input ended too soon
            }
            
            int u = min(a,b), v = max(a,b);
            auto it = edges_metadata.find({u,v});
            
            // Problem says "You will always be given valid edges", so we dont check DNE here,
            // but if we did, we'd fail the whole command. For safety I'll assume DNE makes it fail.
            if (it == edges_metadata.end()) {
                cout << "unsuccessful\n";
                return false;
            }
            
            // TOGGLE the state: open -> closed, closed -> open
            bool current_closed = it->second.second;
            bool new_closed = !current_closed;
            
            // Update the state in metadata
            it->second.second = new_closed;

            // Update graph adjacency lists (bidirectional update)
            for (auto &e : graph[a]) { if (e.to == b) { e.closed = new_closed; } }
            for (auto &e : graph[b]) { if (e.to == a) { e.closed = new_closed; } }
        }
        
        cout << "successful\n";
        return true;

    } else if (cmd == "checkEdgeStatus") {
        int a,b;
        if (!(iss >> a >> b)) { cout << "unsuccessful\n"; return false; }
        
        int u = min(a,b), v = max(a,b);
        auto it = edges_metadata.find({u,v});
        
        if (it == edges_metadata.end()) {
            cout << "DNE\n";
        } else {
            if (it->second.second) cout << "closed\n"; else cout << "open\n";
        }
        return true;

    } else if (cmd == "isConnected") {
        int a,b;
        if (!(iss >> a >> b)) { cout << "unsuccessful\n"; return false; }
        
        // check if nodes exist
        if (!graph.count(a) || !graph.count(b)) {
            cout << "unsuccessful\n";
            return false;
        }

        // Just use a simple BFS for connectivity
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
                if (e.closed) continue; // skip closed
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
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }

        Student &s = students.at(id);
        cout << "Name: " << s.name << "\n";
        
        vector<string> codes(s.classes.begin(), s.classes.end());
        sort(codes.begin(), codes.end()); // sort classes lexicographically, as required

        auto dists = DijkstraDistances(s.residence); 
        
        for (auto &c : codes) {
            int total = -1;
            if (classes_map.count(c)) {
                int loc = classes_map.at(c).location_id;
                // If Dijkstra ran and found a distance
                if (dists.count(loc)) total = dists.at(loc); 
            }
            cout << c << " | Total Time: " << total << "\n";
        }
        return true;

    } else if (cmd == "printStudentZone") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }

        Student &s = students.at(id);
        set<int> nodes; // nodes for the sub-graph

        // Step 1: Find all shortest path nodes
        for (auto &c : s.classes) {
            if (!classes_map.count(c)) continue;
            int loc = classes_map.at(c).location_id;
            int out_dist;
            // Use Dijkstra to get the path
            auto path = DijkstraPathNodes(s.residence, loc, out_dist); 
            if (out_dist < 0) continue; // skip unreachable classes
            for (int v : path) nodes.insert(v); // add all nodes from the path
        }
        nodes.insert(s.residence);

        // Step 2: Build sub-graph edges
        auto subedges = BuildSubgraphEdgesForNodes(nodes);
        vector<int> node_list(nodes.begin(), nodes.end());
        
        // Step 3: Kruskal's for MST cost
        int cost = 0;
        if (node_list.size() >= 1) { 
            cost = KruskalMSTCost((int)node_list.size(), subedges, node_list);
        }
        
        cout << "Student Zone Cost For " << s.name << ": " << cost << "\n"; 
        return true;

    } else if (cmd == "verifySchedule") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }

        Student &s = students.at(id);
        
        // EC: If student has only 1 class, print "unsuccessful"
        if (s.classes.size() <= 1) { 
            cout << "unsuccessful\n"; 
            return true; 
        }

        struct CRec { string code; int start; int end; int loc; };
        vector<CRec> recs;

        for (auto &c : s.classes) {
            if (!classes_map.count(c)) continue;
            ClassInfo ci = classes_map.at(c);
            int st = TimeToMinutes(ci.start_time);
            int en = TimeToMinutes(ci.end_time);
            
            // Skip classes if time is bad
            if (st < 0 || en < 0) continue; 
            recs.push_back({c, st, en, ci.location_id});
        }
        
        // Sort by earliest start time, required for consecutive checking
        sort(recs.begin(), recs.end(), [](const CRec&a,const CRec&b){
            return a.start < b.start;
        });
        
        if (recs.size() <= 1) { 
            cout << "unsuccessful\n"; 
            return true; 
        }

        cout << "Schedule Check for " << s.name << ":\n";
        
        for (size_t i = 0; i + 1 < recs.size(); ++i) {
            int from = recs[i].loc;
            int to = recs[i+1].loc;
            int shortest = -1;
            int outd;
            
            // Find shortest time between classes
            auto path = DijkstraPathNodes(from, to, outd); 
            if (outd >= 0) shortest = outd;
            
            int gap = recs[i+1].start - recs[i].end;
            
            // Time Gap >= Shortest Path Time
            if (shortest <= gap && shortest >= 0) {
                cout << recs[i].code << " - " << recs[i+1].code << " \"Can make it!\"\n";
            } else {
                cout << recs[i].code << " - " << recs[i+1].code << " \"Cannot make it!\"\n";
            }
        }
        return true;
    } else {
        // Fallback for misspelled commands
        cout << "unsuccessful\n";
        return false;
    }
    return false;
}