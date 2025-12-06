#include "CampusCompass.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <queue>
#include <set>

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
        if (a != b) p[a] = b; 
    }
};

// --- Constructor ---
CampusCompass::CampusCompass() {}

// --- Static Helper Definitions (Validation & Utilities) ---

bool CampusCompass::validateName(const string &name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!isalpha(c) && c != ' ') return false;
    }
    return true;
}

bool CampusCompass::validateUfid(const string &ufid) {
    if (ufid.length() != 8) return false;
    for (char c : ufid) {
        if (!isdigit(c)) return false;
    }
    return true;
}

bool CampusCompass::validateClassCode(const string &code) {
    if (code.length() != 7) return false;
    for (int i = 0; i < 3; ++i) {
        if (!isupper(code[i])) return false;
    }
    for (int i = 3; i < 7; ++i) {
        if (!isdigit(code[i])) return false;
    }
    return true;
}

// Custom function to read quoted string (for Name) or single token
string CampusCompass::readQuotedString(stringstream& ss) {
    string token;
    ss >> ws; // Consume leading whitespace
    if (ss.peek() == '"') {
        ss.get(); // Consume opening quote
        // Use a temporary string for extraction to handle potential quotes inside token
        getline(ss, token, '"'); 
        if (ss.fail()) return ""; // Failed to read up to closing quote
        return token;
    }
    // For non-quoted tokens (like commands or IDs), read the next single word
    ss >> token;
    return token;
}

// Time conversion: "HH:MM" to total minutes from midnight
int CampusCompass::timeToMinutes(const string &hhmm) {
    if (hhmm.length() != 5 || hhmm[2] != ':') return -1;
    try {
        int hour = stoi(hhmm.substr(0, 2));
        int minute = stoi(hhmm.substr(3, 2));
        if (hour < 0 || hour > 23 || minute < 0 || minute > 59) return -1;
        return hour * 60 + minute;
    } catch (...) {
        return -1;
    }
}

// --- Data Loading Methods ---

bool CampusCompass::loadEdges(const string &filePath) {
    ifstream file(filePath);
    if (!file.is_open()) return false;

    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        int id1, id2, time;
        string name1, name2, temp;
        
        // Read ID1, ID2, and attempt to read the rest
        if (!(ss >> id1 >> id2)) continue;

        // Skip Name1, Name2, and read Time
        // The file format is irregular, so we skip the strings and grab the last int (Time)
        string remaining_tokens;
        getline(ss, remaining_tokens);
        
        stringstream ss_time(remaining_tokens);
        vector<string> tokens;
        string token;
        while (ss_time >> token) {
            tokens.push_back(token);
        }
        
        if (tokens.empty()) continue;
        
        // Assume Time is the last token and is an integer
        try {
            time = stoi(tokens.back());
        } catch (...) {
            continue;
        }

        if (time <= 0) continue; 

        // Store edge (undirected)
        graph[id1].emplace_back(id2, time, false);
        graph[id2].emplace_back(id1, time, false);

        // Store canonical edge key (min_id, max_id) for metadata
        int u = min(id1, id2);
        int v = max(id1, id2);
        edgesMetadata[{u, v}] = {time, false};
    }
    return true;
}

bool CampusCompass::loadClasses(const string &filePath) {
    ifstream file(filePath);
    if (!file.is_open()) return false;

    string line;
    getline(file, line); // Skip header

    while (getline(file, line)) {
        if (line.empty()) continue;

        stringstream ss(line);
        string code, start, end;
        int locId;
        
        // Assuming CSV structure: ClassCode, LocationID, Start Time, End Time
        if (!getline(ss, code, ',') || !(ss >> locId)) continue;
        ss.ignore(1); // Consume space after ID
        if (!getline(ss, start, ',') || !getline(ss, end)) continue;
        
        // Clean up any trailing whitespace from tokens
        start.erase(remove_if(start.begin(), start.end(), ::isspace), start.end());
        end.erase(remove_if(end.begin(), end.end(), ::isspace), end.end());

        if (!validateClassCode(code)) continue;

        classesMap[code] = {locId, start, end};
    }
    return true;
}

bool CampusCompass::parseCSV(const string &edgesPath, const string &classesPath) {
    bool edgesSuccess = loadEdges(edgesPath);
    bool classesSuccess = loadClasses(classesPath);
    return edgesSuccess && classesSuccess;
}

// --- Graph Algorithms ---

// Dijkstra's: Computes shortest distance from source to all other nodes
unordered_map<int,int> CampusCompass::dijkstraDistances(int source) const {
    // Min-heap: {distance, node}
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    unordered_map<int, int> dist;
    
    // Initialize dist map with infinity for all nodes in the graph
    for (auto const& [id, edges] : graph) dist[id] = numeric_limits<int>::max();
    if (dist.find(source) == dist.end()) return {}; 
    
    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (d > dist[u]) continue;

        if (graph.count(u)) {
            for (const auto& edge : graph.at(u)) {
                if (edge.closed) continue; // Skip closed edges

                int v = edge.to;
                int weight = edge.weight;

                if (dist.count(v) && dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    pq.push({dist[v], v});
                }
            }
        }
    }
    
    // Filter out unreachable nodes
    unordered_map<int, int> result;
    for (auto const& [id, d] : dist) {
        if (d != numeric_limits<int>::max()) {
            result[id] = d;
        }
    }
    return result;
}

// Dijkstra's: Computes shortest path nodes and total distance between source and target
vector<int> CampusCompass::dijkstraPathNodes(int source, int target, int &outDist) const {
    outDist = -1;
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    unordered_map<int, int> dist;
    unordered_map<int, int> prev;
    
    for (auto const& [id, edges] : graph) dist[id] = numeric_limits<int>::max();

    if (dist.find(source) == dist.end() || dist.find(target) == dist.end()) return {};
    
    dist[source] = 0;
    pq.push({0, source});

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();

        if (d > dist[u]) continue;
        if (u == target) {
            outDist = d;
            break; 
        }

        if (graph.count(u)) {
            for (const auto& edge : graph.at(u)) {
                if (edge.closed) continue; 

                int v = edge.to;
                int weight = edge.weight;

                if (dist.count(v) && dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    prev[v] = u;
                    pq.push({dist[v], v});
                }
            }
        }
    }
    
    vector<int> path;
    if (outDist != -1) {
        int curr = target;
        while (curr != source) {
            path.push_back(curr);
            if (!prev.count(curr)) { 
                path.clear(); outDist = -1; return path; 
            }
            curr = prev.at(curr);
        }
        path.push_back(source);
        reverse(path.begin(), path.end());
    }

    return path;
}

// Kruskal's: Computes MST cost
int CampusCompass::kruskalMSTCost(const vector<pair<int,pair<int,int>>>& edges) const {
    if (edges.empty()) return 0;
    
    DSU dsu;
    int mstCost = 0;
    
    vector<pair<int,pair<int,int>>> sortedEdges = edges;
    sort(sortedEdges.begin(), sortedEdges.end());
    
    for (const auto& edge : sortedEdges) {
        int weight = edge.first;
        int u = edge.second.first;
        int v = edge.second.second;
        
        if (dsu.find(u) != dsu.find(v)) {
            dsu.unite(u, v);
            mstCost += weight;
        }
    }
    
    return mstCost;
}

// --- Command Parsing and Handlers ---

bool CampusCompass::parseCommand(const string &commandLine) {
    stringstream ss(commandLine);
    string command;
    if (!(ss >> command)) return false; 

    if (command == "insert") return handleInsert(ss);
    if (command == "remove") return handleRemove(ss);
    if (command == "dropClass") return handleDropClass(ss);
    if (command == "replaceClass") return handleReplaceClass(ss);
    if (command == "removeClass") return handleRemoveClass(ss);
    if (command == "toggleEdgesClosure") return handleToggleEdgesClosure(ss);
    if (command == "checkEdgeStatus") return handleCheckEdgeStatus(ss);
    if (command == "isConnected") return handleIsConnected(ss);
    if (command == "printShortestEdges") return handlePrintShortestEdges(ss);
    if (command == "printStudentZone") return handlePrintStudentZone(ss);
    if (command == "verifySchedule") return handleVerifySchedule(ss);
    
    cout << "unsuccessful\n";
    return false;
}

bool CampusCompass::handleInsert(stringstream& ss) {
    string name = readQuotedString(ss);
    string ufid;
    int residence, n_classes;
    
    if (!(ss >> ufid >> residence >> n_classes)) {
        cout << "unsuccessful\n"; return false;
    }

    if (!validateName(name) || !validateUfid(ufid) || students.count(ufid)) {
        cout << "unsuccessful\n"; return false;
    }
    if (n_classes < 1 || n_classes > 6) {
        cout << "unsuccessful\n"; return false;
    }
    
    set<string> class_codes;
    for (int i = 0; i < n_classes; ++i) {
        string code;
        if (!(ss >> code) || !validateClassCode(code) || !classesMap.count(code) || class_codes.count(code)) {
            cout << "unsuccessful\n"; return false;
        }
        class_codes.insert(code);
    }
    
    string extra;
    if (ss >> extra) {
        cout << "unsuccessful\n"; return false;
    }
    
    students[ufid] = {name, ufid, residence, class_codes};
    cout << "successful\n";
    return true;
}

bool CampusCompass::handleRemove(stringstream& ss) {
    string ufid;
    if (!(ss >> ufid)) {
        cout << "unsuccessful\n"; return false;
    }
    if (students.erase(ufid) > 0) {
        cout << "successful\n"; return true;
    } else {
        cout << "unsuccessful\n"; return false;
    }
}

bool CampusCompass::handleDropClass(stringstream& ss) {
    string ufid, code;
    if (!(ss >> ufid >> code)) {
        cout << "unsuccessful\n"; return false;
    }

    if (!students.count(ufid) || !classesMap.count(code)) {
        cout << "unsuccessful\n"; return false;
    }
    
    Student& s = students.at(ufid);
    if (!s.classes.count(code)) {
        cout << "unsuccessful\n"; return false;
    }

    s.classes.erase(code);
    
    if (s.classes.empty()) {
        students.erase(ufid);
    }
    
    cout << "successful\n";
    return true;
}

bool CampusCompass::handleReplaceClass(stringstream& ss) {
    string ufid, code1, code2;
    if (!(ss >> ufid >> code1 >> code2)) {
        cout << "unsuccessful\n"; return false;
    }

    if (!students.count(ufid) || !classesMap.count(code2)) {
        cout << "unsuccessful\n"; return false;
    }

    Student& s = students.at(ufid);
    if (!s.classes.count(code1) || s.classes.count(code2)) {
        cout << "unsuccessful\n"; return false;
    }

    s.classes.erase(code1);
    s.classes.insert(code2);
    
    cout << "successful\n";
    return true;
}

bool CampusCompass::handleRemoveClass(stringstream& ss) {
    string code;
    if (!(ss >> code) || !classesMap.count(code)) {
        cout << "0\n"; return false;
    }
    
    int dropped_count = 0;
    vector<string> ufids_to_remove;

    for (auto& pair : students) {
        Student& s = pair.second;
        if (s.classes.count(code)) {
            s.classes.erase(code);
            dropped_count++;
            if (s.classes.empty()) {
                ufids_to_remove.push_back(s.ufid);
            }
        }
    }
    
    for (const string& ufid : ufids_to_remove) {
        students.erase(ufid);
    }

    cout << dropped_count << "\n";
    return true;
}

bool CampusCompass::handleToggleEdgesClosure(stringstream& ss) {
    int n;
    if (!(ss >> n)) {
        cout << "successful\n"; // Input error handling simplified
        return false; 
    }
    
    for (int i = 0; i < n; ++i) {
        int u, v;
        if (!(ss >> u >> v)) break;

        int id1 = min(u, v);
        int id2 = max(u, v);
        
        if (edgesMetadata.count({id1, id2})) {
            bool new_status = !edgesMetadata.at({id1, id2}).second;
            edgesMetadata.at({id1, id2}).second = new_status;
        
            for (auto& edge : graph[u]) {
                if (edge.to == v) { edge.closed = new_status; break; }
            }
            for (auto& edge : graph[v]) {
                if (edge.to == u) { edge.closed = new_status; break; }
            }
        }
    }
    
    cout << "successful\n";
    return true;
}

bool CampusCompass::handleCheckEdgeStatus(stringstream& ss) {
    int u, v;
    if (!(ss >> u >> v)) {
        cout << "unsuccessful\n"; return false;
    }

    int id1 = min(u, v);
    int id2 = max(u, v);
    
    if (!edgesMetadata.count({id1, id2})) {
        cout << "DNE\n"; return true;
    }
    
    if (edgesMetadata.at({id1, id2}).second) {
        cout << "closed\n";
    } else {
        cout << "open\n";
    }
    return true;
}

bool CampusCompass::handleIsConnected(stringstream& ss) {
    int u, v;
    if (!(ss >> u >> v)) {
        cout << "unsuccessful\n"; return false;
    }

    if (!graph.count(u) || !graph.count(v)) {
        cout << "unsuccessful\n"; return false;
    }

    queue<int> q;
    set<int> visited;
    
    q.push(u);
    visited.insert(u);

    while (!q.empty()) {
        int curr = q.front();
        q.pop();

        if (curr == v) {
            cout << "successful\n"; return true;
        }

        for (const auto& edge : graph.at(curr)) {
            if (!edge.closed && !visited.count(edge.to)) {
                visited.insert(edge.to);
                q.push(edge.to);
            }
        }
    }

    cout << "unsuccessful\n";
    return false;
}

struct ClassResult { string code; int time; };

bool CampusCompass::handlePrintShortestEdges(stringstream& ss) {
    string ufid;
    if (!(ss >> ufid) || !students.count(ufid)) {
        cout << "unsuccessful\n"; return false;
    }
    
    const Student& s = students.at(ufid);
    unordered_map<int, int> shortest_distances = dijkstraDistances(s.residence);
    vector<ClassResult> results;
    
    for (const string& code : s.classes) {
        int locId = classesMap.at(code).locationId;
        int shortestTime = -1;

        if (shortest_distances.count(locId)) {
            shortestTime = shortest_distances.at(locId);
        }
        
        results.push_back({code, shortestTime});
    }

    sort(results.begin(), results.end(), [](const ClassResult& a, const ClassResult& b) {
        return a.code < b.code;
    });

    cout << "Name: " << s.name << "\n";
    for (const auto& res : results) {
        cout << res.code << " | Total Time: " << res.time << "\n";
    }
    return true;
}

bool CampusCompass::handlePrintStudentZone(stringstream& ss) {
    string ufid;
    if (!(ss >> ufid) || !students.count(ufid)) {
        cout << "unsuccessful\n"; return false;
    }
    
    const Student& s = students.at(ufid);
    set<int> all_path_nodes;
    bool has_reachable_class = false;
    
    for (const string& code : s.classes) {
        int target = classesMap.at(code).locationId;
        int dist;
        vector<int> path = dijkstraPathNodes(s.residence, target, dist);
        
        if (dist != -1) {
            has_reachable_class = true;
            all_path_nodes.insert(path.begin(), path.end());
        }
    }
    
    if (!has_reachable_class) {
        cout << "Student Zone Cost For " << s.name << ": 0\n";
        return true; 
    }

    // Build subgraph edges only using open edges and nodes on the shortest paths
    vector<pair<int,pair<int,int>>> subgraphEdges;
    for (auto const& [key, metadata] : edgesMetadata) {
        int u = key.first;
        int v = key.second;
        int weight = metadata.first;
        bool closed = metadata.second;

        if (!closed && all_path_nodes.count(u) && all_path_nodes.count(v)) {
            subgraphEdges.push_back({weight, {u, v}});
        }
    }
    
    int mstCost = kruskalMSTCost(subgraphEdges);
    
    cout << "Student Zone Cost For " << s.name << ": " << mstCost << "\n";
    return true;
}

struct ClassRecord { string code; int start, end, loc; };

bool CampusCompass::handleVerifySchedule(stringstream& ss) {
    string ufid;
    if (!(ss >> ufid) || !students.count(ufid)) {
        cout << "unsuccessful\n"; return false;
    }

    const Student& s = students.at(ufid);
    vector<ClassRecord> records;

    for (const string& c : s.classes) {
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
        cout << "unsuccessful\n"; return true; 
    }

    cout << "Schedule Check for " << s.name << ":\n";
    
    for (size_t i = 0; i + 1 < records.size(); ++i) {
        int from = records[i].loc; 
        int to = records[i+1].loc;
        int outDist;
        
        dijkstraPathNodes(from, to, outDist); 
        int shortestTime = outDist;
        
        int timeGap = records[i+1].start - records[i].end;
        
        string status;
        if (shortestTime >= 0 && shortestTime <= timeGap) {
            status = "\"Can make it!\"";
        } else {
            status = "\"Cannot make it!\"";
        }

        cout << records[i].code << " - " << records[i+1].code << " " << status << "\n";
    }

    return true;
}