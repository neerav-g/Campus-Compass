#include "CampusCompass.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <algorithm>
#include <cctype>
#include <limits>

using namespace std;

CampusCompass::CampusCompass() {
    // Constructor initializes all members to an empty state (though this is redundant for map/vector).
    graph.clear();
    edgesMetadata.clear();
    classesMap.clear();
    students.clear();
}

/**
 * @brief Splits a CSV line, handling quotes and trimming whitespace.
 * @param line The single line of CSV/TSV data.
 * @return A vector of strings representing the fields.
 */
vector<string> CampusCompass::splitCsvLine(const string &line) {
    vector<string> out;
    string cur;
    bool inQuote = false;
    
    // Lambda function for trimming whitespace and carriage returns from fields
    auto trimAndPush = [&](const string& s) {
        size_t a = s.find_first_not_of(" \t\r\n");
        size_t b = s.find_last_not_of(" \t\r\n");
        string t;
        if (a == string::npos) t = "";
        else t = s.substr(a, b - a + 1);
        if (!t.empty() && t.back() == '\r') t.pop_back();
        out.push_back(t);
    };

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"' ) {
            // Toggle inQuote state
            inQuote = !inQuote;
            continue;
        }
        // Delimiters are comma or tab, but only when not inside quotes
        if (!inQuote && (c == ',' || c == '\t')) {
            trimAndPush(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    // Handle the last field after the loop
    trimAndPush(cur);
    return out;
}

// --- Validation Functions ---

bool CampusCompass::validateName(const string &name) {
    if (name.empty()) return false;
    for (char c : name) {
        // Must only contain alphabetic characters or spaces
        if (!(isalpha((unsigned char)c) || c == ' ')) return false;
    }
    return true;
}

bool CampusCompass::validateUfid(const string &ufid) {
    // UFID must be exactly 8 digits long
    if (ufid.size() != 8) return false;
    for (char c : ufid) if (!isdigit((unsigned char)c)) return false;
    return true;
}

bool CampusCompass::validateClassCode(const string &code) {
    // Class code must be 3 capital letters followed by 4 digits (e.g., COP3530)
    if (code.size() != 7) return false;
    for (int i = 0; i < 3; ++i) if (!isupper((unsigned char)code[i])) return false;
    for (int i = 3; i < 7; ++i) if (!isdigit((unsigned char)code[i])) return false;
    return true;
}

/**
 * @brief Converts time string "HH:MM" to total minutes from midnight.
 * @param hhmm The time string.
 * @return Total minutes, or -1 on invalid format/range.
 */
int CampusCompass::timeToMinutes(const string &hhmm) {
    if (hhmm.empty()) return -1;
    size_t pos = hhmm.find(':');
    if (pos == string::npos) return -1;
    string sh = hhmm.substr(0, pos);
    string sm = hhmm.substr(pos + 1);
    
    auto trim = [](string s) {
        size_t a = s.find_first_not_of(" \r\n");
        size_t b = s.find_last_not_of(" \r\n");
        if (a == string::npos) return string("");
        return s.substr(a, b - a + 1);
    };
    sh = trim(sh);
    sm = trim(sm);
    
    // Check if parts are numeric
    if (sh.empty() || sm.empty()) return -1;
    for (char c : sh) if (!isdigit((unsigned char)c)) return -1;
    for (char c : sm) if (!isdigit((unsigned char)c)) return -1;
    
    int h = 0, m = 0;
    try {
        h = stoi(sh);
        m = stoi(sm);
    } catch (...) {
        return -1;
    }
    
    // Check range validity
    if (h < 0 || h > 23) return -1;
    if (m < 0 || m > 59) return -1;
    
    return h * 60 + m;
}

// --- Data Loading Functions ---

bool CampusCompass::loadEdges(const string &filePath) {
    ifstream fin(filePath);
    if (!fin.is_open()) return false;
    string line;
    
    // Read and discard header line
    if (!getline(fin, line)) return true;

    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        auto parts = splitCsvLine(line);
        // We only care about LocationID_1, LocationID_2, and Time (5th field)
        if (parts.size() < 5) continue; 

        // Ensure Location IDs (parts[0] and parts[1]) and Time (parts.back()) are numeric
        bool ok0 = !parts[0].empty() && all_of(parts[0].begin(), parts[0].end(), [](char c){ return isdigit((unsigned char)c); });
        bool ok1 = !parts[1].empty() && all_of(parts[1].begin(), parts[1].end(), [](char c){ return isdigit((unsigned char)c); });
        bool okTime = !parts.back().empty() && all_of(parts.back().begin(), parts.back().end(), [](char c){ return isdigit((unsigned char)c); });
        
        if (!ok0 || !ok1 || !okTime) continue;

        int id1 = 0, id2 = 0, time = 0;
        try {
            id1 = stoi(parts[0]);
            id2 = stoi(parts[1]);
            time = stoi(parts.back());
        } catch (...) {
            continue;
        }
        if (id1 == 0 || id2 == 0 || id1 == id2) continue; // Skip invalid IDs or self-loops

        // Add to adjacency list (undirected graph)
        graph[id1].push_back(Edge(id2, time, false));
        graph[id2].push_back(Edge(id1, time, false));

        // Store metadata using canonical key (min, max) for easy lookups/toggling
        int u = min(id1, id2);
        int v = max(id1, id2);
        edgesMetadata[{u,v}] = {time, false}; // Time (weight) and initial status (open=false)
    }
    return true;
}

bool CampusCompass::loadClasses(const string &filePath) {
    ifstream fin(filePath);
    if (!fin.is_open()) return false;
    string line;
    
    // Read and discard header line
    if (!getline(fin, line)) return true;

    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        auto parts = splitCsvLine(line);
        
        // Need at least ClassCode (0) and LocationID (1)
        if (parts.size() < 2) continue; 
        
        string code = parts[0];
        if (code.empty() || !validateClassCode(code)) continue;

        int loc = 0;
        bool ok = !parts[1].empty() && all_of(parts[1].begin(), parts[1].end(), [](char c){ return isdigit((unsigned char)c); });
        if (!ok) continue;
        
        try {
            loc = stoi(parts[1]);
        } catch (...) {
            continue;
        }
        
        ClassInfo ci;
        ci.locationId = loc;
        
        // Load optional start/end times for Extra Credit
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

// Core logic for Dijkstra's to find the shortest path nodes and total time.
vector<int> CampusCompass::dijkstraPathNodes(int source, int target, int &outDist) const {
    // 
    
    // Stores shortest distance from source to each node found so far.
    unordered_map<int,int> dist;
    // Stores the predecessor of each node on the shortest path.
    unordered_map<int,int> prev; 
    
    // Node structure for the Priority Queue: {ID, Distance}
    struct Node { int id; int d; };
    // Comparator for the Min-Heap: orders by shortest distance (d).
    struct Cmp { bool operator()(const Node &a, const Node &b) const { return a.d > b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;

    dist[source] = 0;
    pq.push({source, 0});

    while (!pq.empty()) {
        Node cur = pq.top(); pq.pop();
        int u = cur.id;
        int d = cur.d;
        
        // Skip if we found a shorter path to u already (stale entry)
        if (dist.count(u) && d > dist.at(u)) continue; 
        
        if (u == target) break; // Found the target, stop early
        
        auto it = graph.find(u);
        if (it == graph.end()) continue; // Node u has no outgoing edges

        // Relax all outgoing edges from u
        for (const Edge &e : it->second) {
            if (e.closed) continue; // Skip closed edges
            
            int v = e.to;
            int newDist = d + e.weight;
            
            // Relaxation step: If a shorter path to v is found
            if (!dist.count(v) || newDist < dist.at(v)) {
                dist[v] = newDist;
                prev[v] = u;
                pq.push({v, newDist});
            }
        }
    }

    // Path reconstruction
    if (!dist.count(target)) {
        outDist = -1; // Target unreachable
        return {};
    }

    outDist = dist.at(target);
    vector<int> path;
    int cur = target;
    while (true) {
        path.push_back(cur);
        if (cur == source) break;
        if (!prev.count(cur)) { outDist = -1; return {}; } // Should not happen if dist.count(target) is true
        cur = prev[cur];
    }
    reverse(path.begin(), path.end());
    return path;
}

// Overload of Dijkstra's for use in printShortestEdges (doesn't reconstruct path).
unordered_map<int,int> CampusCompass::dijkstraDistances(int source) const {
    unordered_map<int,int> dist;
    struct Node { int id; int d; };
    struct Cmp { bool operator()(const Node &a, const Node &b) const { return a.d > b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;

    dist[source] = 0;
    pq.push({source, 0});

    while (!pq.empty()) {
        Node cur = pq.top(); pq.pop();
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

/**
 * @brief Filters the campus edges to create a subgraph containing only edges 
 * connecting two nodes present in the given set.
 * @param nodes The set of nodes forming the subgraph (from shortest paths).
 * @return A vector of edges {weight, {node1, node2}}.
 */
vector<pair<int,pair<int,int>>> CampusCompass::buildSubgraphEdgesForNodes(const set<int>& nodes) const {
    vector<pair<int,pair<int,int>>> out;
    // Iterate over the canonical list of all edges
    for (const auto &kv : edgesMetadata) {
        int u = kv.first.first;
        int v = kv.first.second;
        int w = kv.second.first;
        bool closed = kv.second.second;
        
        if (closed) continue; // Only consider open edges
        
        // An edge is part of the subgraph only if BOTH endpoints are in the nodes set
        if (nodes.count(u) && nodes.count(v)) {
            out.push_back({w, {u, v}});
        }
    }
    return out;
}

// Comparator for sorting edges by weight (used in Kruskal's).
static bool edgeCompare(const pair<int, pair<int,int>> &a, const pair<int, pair<int,int>> &b) {
    return a.first < b.first;
}

/**
 * @brief Computes the Minimum Spanning Tree (MST) cost using Kruskal's algorithm.
 * @param edges The list of edges in the subgraph.
 * @param nodeList The list of nodes in the subgraph.
 * @return The total cost of the MST.
 */
int CampusCompass::kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges, const vector<int>& nodeList) const {
    vector<pair<int,pair<int,int>>> sortedEdges = edges;
    // 1. Sort all edges by weight in ascending order (greedy choice).
    sort(sortedEdges.begin(), sortedEdges.end(), edgeCompare);

    DSU dsu;
    // 2. Initialize DSU: each node is its own component.
    for (int node : nodeList) dsu.p[node] = node;
    int components = static_cast<int>(nodeList.size());
    if (components <= 1) return 0; // Already connected or empty

    int cost = 0;
    // 3. Iterate through sorted edges and use DSU for cycle detection.
    for (const auto &e : sortedEdges) {
        int w = e.first;
        int u = e.second.first;
        int v = e.second.second;
        
        // If u and v are not already connected (adding the edge won't form a cycle)
        if (dsu.find(u) != dsu.find(v)) {
            dsu.unite(u, v); // Merge the two components
            cost += w;       // Add edge weight to MST cost
            components--;    // Decrease component count
            if (components == 1) break; // Stop when all nodes are connected
        }
    }
    return cost;
}

// --- Command Parsing ---

bool CampusCompass::parseCommand(const string &command) {
    if (command.empty()) {
        cout << "unsuccessful\n";
        return false;
    }

    istringstream iss(command);
    string cmd;
    iss >> cmd;

    // --- Command: insert ---
    if (cmd == "insert") {
        // Find quoted name string
        string rest = command.substr(6);
        size_t q1 = rest.find('"');
        size_t q2 = string::npos;
        if (q1 != string::npos) q2 = rest.find('"', q1 + 1);
        if (q1 == string::npos || q2 == string::npos) { cout << "unsuccessful\n"; return false; }
        
        string name = rest.substr(q1 + 1, q2 - q1 - 1);
        string after = rest.substr(q2 + 1);
        istringstream aiss(after);
        
        string ufid; 
        int residence; 
        int numClasses;
        
        // Read UFID, Residence ID, and N
        if (!(aiss >> ufid >> residence >> numClasses)) { cout << "unsuccessful\n"; return false; }
        
        // Validate student constraints
        if (!validateName(name) || !validateUfid(ufid) || numClasses < 1 || numClasses > 6) {
            cout << "unsuccessful\n"; return false;
        }
        
        // Read N class codes
        vector<string> classList;
        string code;
        for (int i = 0; i < numClasses; ++i) {
            if (!(aiss >> code)) { cout << "unsuccessful\n"; return false; }
            // Validate code format and existence in classesMap
            if (!validateClassCode(code) || !classesMap.count(code)) { cout << "unsuccessful\n"; return false; }
            classList.push_back(code);
        }
        
        // Check for UFID uniqueness
        if (students.count(ufid)) { cout << "unsuccessful\n"; return false; }
        
        // Final insertion
        Student s;
        s.name = name;
        s.ufid = ufid;
        s.residence = residence;
        for (const auto &c : classList) s.classes.insert(c);
        students[ufid] = s;
        cout << "successful\n";
        return true;

    // --- Command: remove ---
    } else if (cmd == "remove") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        
        if (!students.count(id)) { 
            cout << "unsuccessful\n"; return false; 
        } 
        
        // Only allow removal if the student has 0 classes
        if (students.at(id).classes.empty()) {
            students.erase(id);
            cout << "successful\n";
            return true;
        } else {
            cout << "unsuccessful\n"; return false;
        }

    // --- Command: dropClass ---
    } else if (cmd == "dropClass") {
        string id, code;
        if (!(iss >> id >> code)) { cout << "unsuccessful\n"; return false; }
        
        // Validate student existence, class code format, and class code existence
        if (!students.count(id) || !validateClassCode(code) || !classesMap.count(code)) { 
            cout << "unsuccessful\n"; return false; 
        }
        
        Student &s = students.at(id);
        // Check if student is enrolled in the class
        if (!s.classes.count(code)) { 
            cout << "unsuccessful\n"; return false; 
        }
        
        s.classes.erase(code);
        
        // If the student now has 0 classes, remove the student record
        if (s.classes.empty()) students.erase(id); 
        
        cout << "successful\n";
        return true;

    // --- Command: replaceClass ---
    } else if (cmd == "replaceClass") {
        string id, codeA, codeB;
        if (!(iss >> id >> codeA >> codeB)) { cout << "unsuccessful\n"; return false; }
        
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        
        // Validate class codes and ensure codeB exists
        if (!validateClassCode(codeA) || !validateClassCode(codeB) || !classesMap.count(codeB)) { 
            cout << "unsuccessful\n"; return false; 
        }
        
        Student &s = students.at(id);
        // Fail if student doesn't have A or already has B
        if (!s.classes.count(codeA) || s.classes.count(codeB)) { 
            cout << "unsuccessful\n"; return false; 
        }
        
        s.classes.erase(codeA);
        s.classes.insert(codeB);
        cout << "successful\n";
        return true;

    // --- Command: removeClass ---
    } else if (cmd == "removeClass") {
        string code;
        if (!(iss >> code)) { cout << "unsuccessful\n"; return false; }
        
        // Check if the class code is valid and exists in the classes map
        if (!validateClassCode(code) || !classesMap.count(code)) {
            cout << "0\n"; return true; // Prints 0 if class does not exist
        }
        
        int removedCount = 0;
        // Iterate through all students to remove the class
        for (auto it = students.begin(); it != students.end(); ) {
            Student &s = it->second;
            if (s.classes.count(code)) {
                removedCount++;
                s.classes.erase(code);
                // If student is left with 0 classes, remove the student
                if (s.classes.empty()) it = students.erase(it);
                else ++it;
            } else ++it;
        }
        
        cout << removedCount << "\n";
        return true;

    // --- Command: toggleEdgesClosure ---
    } else if (cmd == "toggleEdgesClosure") {
        int count;
        if (!(iss >> count)) { cout << "unsuccessful\n"; return false; }
        
        for (int i = 0; i < count; ++i) {
            int a, b;
            if (!(iss >> a >> b)) { cout << "unsuccessful\n"; return false; }
            
            int u = min(a, b);
            int v = max(a, b);
            
            // Find canonical edge in metadata
            auto it = edgesMetadata.find({u, v});
            if (it == edgesMetadata.end()) {
                cout << "unsuccessful\n"; return false;
            }
            
            // Toggle status in metadata
            bool newClosed = !it->second.second;
            it->second.second = newClosed;
            
            // Update adjacency lists in graph for fast access during Dijkstra's
            for (auto &e : graph[u]) if (e.to == v) e.closed = newClosed;
            for (auto &e : graph[v]) if (e.to == u) e.closed = newClosed;
        }
        cout << "successful\n";
        return true;

    // --- Command: checkEdgeStatus ---
    } else if (cmd == "checkEdgeStatus") {
        int a, b;
        if (!(iss >> a >> b)) { cout << "unsuccessful\n"; return false; }
        
        int u = min(a, b);
        int v = max(a, b);
        
        auto it = edgesMetadata.find({u, v});
        if (it == edgesMetadata.end()) {
            cout << "DNE\n"; // Does Not Exist
        } else {
            if (it->second.second) cout << "closed\n"; else cout << "open\n";
        }
        return true;

    // --- Command: isConnected ---
    } else if (cmd == "isConnected") {
        int a, b;
        if (!(iss >> a >> b)) { cout << "unsuccessful\n"; return false; }
        
        if (!graph.count(a) || !graph.count(b)) {
            cout << "unsuccessful\n"; return false;
        }
        
        // Use Breadth-First Search (BFS) to find connectivity
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
            
            for (const auto &e : it->second) {
                if (e.closed) continue; // Skip closed edges
                if (!visited.count(e.to)) {
                    visited.insert(e.to);
                    q.push(e.to);
                }
            }
        }
        
        if (found) cout << "successful\n"; else cout << "unsuccessful\n";
        return true;

    // --- Command: printShortestEdges ---
    } else if (cmd == "printShortestEdges") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        
        Student &s = students.at(id);
        cout << "Name: " << s.name << "\n";
        
        // Copy class codes and sort them lexicographically for output order
        vector<string> codes(s.classes.begin(), s.classes.end());
        sort(codes.begin(), codes.end());
        
        // Run Dijkstra's once from the student's residence
        auto dists = dijkstraDistances(s.residence);
        
        for (const auto &c : codes) {
            int totalTime = -1;
            
            if (classesMap.count(c)) {
                int loc = classesMap.at(c).locationId;
                // Check if the class location was reached by Dijkstra's
                if (dists.count(loc)) totalTime = dists.at(loc);
            }
            
            cout << c << " | Total Time: " << totalTime << "\n";
        }
        return true;

    // --- Command: printStudentZone ---
    } else if (cmd == "printStudentZone") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        
        Student &s = students.at(id);
        set<int> nodes;
        nodes.insert(s.residence); // Start with residence
        
        // 1. Find the shortest path from residence to every class and collect all nodes on those paths
        for (const auto &c : s.classes) {
            if (!classesMap.count(c)) continue;
            int loc = classesMap.at(c).locationId;
            int outDist;
            
            // Get the nodes on the shortest path
            auto path = dijkstraPathNodes(s.residence, loc, outDist);
            
            if (outDist < 0) continue; // Skip unreachable classes
            
            for (int v : path) nodes.insert(v);
        }
        
        // 2. Build the subgraph containing only edges connecting the collected nodes
        auto subEdges = buildSubgraphEdgesForNodes(nodes);
        vector<int> nodeList(nodes.begin(), nodes.end()); // Nodes for DSU initialization
        
        // 3. Calculate the MST cost of the subgraph using Kruskal's
        int cost = 0;
        if (!nodeList.empty()) cost = kruskalMSTCost(subEdges, nodeList);
        
        cout << "Student Zone Cost For " << s.name << ": " << cost << "\n";
        return true;

    // --- Command: verifySchedule (Extra Credit) ---
    } else if (cmd == "verifySchedule") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        
        Student &s = students.at(id);
        
        struct ClassRecord { string code; int start; int end; int loc; };
        vector<ClassRecord> records;
        
        // Populate records with validated and converted time data
        for (const auto &c : s.classes) {
            if (!classesMap.count(c)) continue;
            ClassInfo ci = classesMap.at(c);
            
            int startMinutes = timeToMinutes(ci.startTime);
            int endMinutes = timeToMinutes(ci.endTime);
            
            // Only consider classes with valid time data
            if (startMinutes < 0 || endMinutes < 0) continue; 
            
            records.push_back({c, startMinutes, endMinutes, ci.locationId});
        }
        
        // Sort records by earliest start time
        sort(records.begin(), records.end(), [](const ClassRecord &a, const ClassRecord &b){
            return a.start < b.start;
        });
        
        if (records.size() <= 1) { 
            cout << "unsuccessful\n"; return true; // Need at least two classes to check a gap
        }
        
        cout << "Schedule Check for " << s.name << ":\n";
        
        for (size_t i = 0; i + 1 < records.size(); ++i) {
            int from = records[i].loc;
            int to = records[i+1].loc;
            
            int shortestTime = -1;
            int outDist;
            
            // Find shortest travel time between the two class locations
            dijkstraPathNodes(from, to, outDist);
            if (outDist >= 0) shortestTime = outDist;
            
            // Calculate time gap (time available for travel)
            int timeGap = records[i+1].start - records[i].end;
            
            // Check feasibility
            if (shortestTime >= 0 && shortestTime <= timeGap) {
                cout << records[i].code << " - " << records[i+1].code << " \"Can make it!\"\n";
            } else {
                cout << records[i].code << " - " << records[i+1].code << " \"Cannot make it!\"\n";
            }
        }
        return true;
        
    } else {
        // Unrecognized command
        cout << "unsuccessful\n";
        return false;
    }
}