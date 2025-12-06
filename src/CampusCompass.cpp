#include "CampusCompass.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <queue>
#include <limits>
#include <algorithm>

using namespace std;

CampusCompass::CampusCompass() {
    graph.clear();
    edges_metadata.clear();
    classes_map.clear();
    students.clear();
}

static vector<string> split_csv_line(const string &line) {
    // naive split on commas or tabs, trim whitespace
    vector<string> out;
    string cur;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == ',' || c == '\t') {
            // push
            string t = cur;
            // trim
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
    string t = cur;
    size_t a = t.find_first_not_of(" \r\n");
    size_t b = t.find_last_not_of(" \r\n");
    if (a == string::npos) t = "";
    else t = t.substr(a, b - a + 1);
    out.push_back(t);
    return out;
}

bool CampusCompass::LoadEdges(const string &filepath) {
    ifstream fin(filepath);
    if (!fin.is_open()) return false;
    string line;
    // skip header if present by checking whether the first nonempty line contains non-numeric tokens
    bool first = true;
    while (getline(fin, line)) {
        if (line.find_first_not_of(" \t\r\n") == string::npos) continue;
        auto parts = split_csv_line(line);
        // Expect at least 5 columns like: LocationID_1,LocationID_2,Name_1,Name_2,Time
        if (parts.size() < 5) {
            // could be whitespace or header - try to parse as numbers if possible
            bool numeric_line = true;
            for (auto &p : parts) {
                if (p.empty()) continue;
                for (char c : p) if (!isdigit(c) && c != '-' && c != '+') numeric_line = false;
            }
            if (!numeric_line) continue;
        }
        // try to parse numeric columns
        int id1 = 0, id2 = 0, time = 0;
        try {
            id1 = stoi(parts[0]);
            id2 = stoi(parts[1]);
            time = stoi(parts.back());
        } catch (...) {
            continue;
        }
        // store undirected edge
        graph[id1].push_back(Edge(id2, time, false));
        graph[id2].push_back(Edge(id1, time, false));
        int a = min(id1, id2);
        int b = max(id1, id2);
        edges_metadata[{a,b}] = {time, false};
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
        // Expect at least 2 columns: ClassCode, LocationID, optional start/end
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
    return a && b;
}

bool CampusCompass::ValidateName(const string &name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!(isalpha((unsigned char)c) || c == ' ')) return false;
    }
    return true;
}

bool CampusCompass::ValidateUFID(const string &ufid) {
    if (ufid.size() != 8) return false;
    for (char c : ufid) if (!isdigit((unsigned char)c)) return false;
    return true;
}

bool CampusCompass::ValidateClassCode(const string &code) {
    if (code.size() != 7) return false;
    for (int i = 0; i < 3; ++i) if (!isupper((unsigned char)code[i])) return false;
    for (int i = 3; i < 7; ++i) if (!isdigit((unsigned char)code[i])) return false;
    return true;
}

vector<int> CampusCompass::DijkstraPathNodes(int source, int target, int &out_dist) const {
    // returns the nodes on shortest path from source to target inclusive; if unreachable returns empty and out_dist = -1
    const int INF = numeric_limits<int>::max() / 4;
    unordered_map<int,int> dist;
    unordered_map<int,int> prev;
    struct Node {
        int id;
        int d;
    };
    struct Cmp {
        bool operator()(const Node&a, const Node&b) const { return a.d > b.d; }
    };
    priority_queue<Node, vector<Node>, Cmp> pq;
    dist[source] = 0;
    pq.push({source, 0});
    while (!pq.empty()) {
        auto cur = pq.top(); pq.pop();
        int u = cur.id;
        int d = cur.d;
        if (d != dist.at(u)) continue;
        if (u == target) break;
        auto it = graph.find(u);
        if (it == graph.end()) continue;
        for (const Edge &e : it->second) {
            if (e.closed) continue;
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
        return {};
    }
    out_dist = dist.at(target);
    vector<int> path;
    int cur = target;
    while (true) {
        path.push_back(cur);
        if (cur == source) break;
        cur = prev[cur];
    }
    reverse(path.begin(), path.end());
    return path;
}

unordered_map<int,int> CampusCompass::DijkstraDistances(int source) const {
    unordered_map<int,int> dist;
    const int INF = numeric_limits<int>::max() / 4;
    struct Node { int id; int d; };
    struct Cmp { bool operator()(const Node&a, const Node&b) const { return a.d > b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;
    dist[source] = 0;
    pq.push({source,0});
    while (!pq.empty()) {
        auto cur = pq.top(); pq.pop();
        int u = cur.id;
        int d = cur.d;
        if (d != dist.at(u)) continue;
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

vector<pair<int,pair<int,int>>> CampusCompass::BuildSubgraphEdgesForNodes(const set<int>& nodes) const {
    // returns vector of {weight, {u,v}} with u<v for nodes both in nodes set and edge open
    vector<pair<int,pair<int,int>>> out;
    for (auto &kv : edges_metadata) {
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

struct DSU {
    unordered_map<int,int> p;
    int find(int x) {
        if (!p.count(x)) p[x] = x;
        if (p[x] == x) return x;
        return p[x] = find(p[x]);
    }
    void unite(int a, int b) { a = find(a); b = find(b); if (a != b) p[b] = a; }
};

int CampusCompass::KruskalMSTCost(int n_nodes, const vector<pair<int,pair<int,int>>>& edges, const vector<int>& node_list) const {
    // edges: {weight, {u,v}}
    vector<pair<int,pair<int,int>>> es = edges;
    sort(es.begin(), es.end(),
    [](const pair<int, pair<int, int>>& a,
       const pair<int, pair<int, int>>& b) {
        return a.first < b.first;
    }
);
    DSU dsu;
    for (int node : node_list) dsu.p[node] = node;
    int comps = node_list.size();
    int cost = 0;
    for (auto &e : es) {
        int w = e.first;
        int u = e.second.first;
        int v = e.second.second;
        if (dsu.find(u) != dsu.find(v)) {
            dsu.unite(u, v);
            cost += w;
            comps--;
            if (comps == 1) break;
        }
    }
    return cost;
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
        return -1;
    }
    return h * 60 + m;
}

bool CampusCompass::ParseCommand(const string &command) {
    if (command.empty()) {
        cout << "unsuccessful\n";
        return false;
    }
    // parse quoted name if present
    istringstream iss(command);
    string cmd;
    iss >> cmd;
    if (cmd == "insert") {
        // insert "Name" UFID residence N CLASS1 CLASS2 ...
        string rest = command.substr(6);
        // find first quote
        size_t q1 = rest.find('"');
        size_t q2 = string::npos;
        if (q1 != string::npos) q2 = rest.find('"', q1+1);
        if (q1 == string::npos || q2 == string::npos) {
            cout << "unsuccessful\n";
            return false;
        }
        string name = rest.substr(q1+1, q2 - q1 - 1);
        // remainder tokens after q2
        string after = rest.substr(q2+1);
        istringstream aiss(after);
        string ufid; int residence; int nclasses;
        if (!(aiss >> ufid >> residence >> nclasses)) {
            cout << "unsuccessful\n";
            return false;
        }
        if (!ValidateName(name) || !ValidateUFID(ufid) || nclasses < 1 || nclasses > 6) {
            cout << "unsuccessful\n";
            return false;
        }
        vector<string> classlist;
        string code;
        for (int i = 0; i < nclasses; ++i) {
            if (!(aiss >> code)) {
                cout << "unsuccessful\n";
                return false;
            }
            if (!ValidateClassCode(code) || !classes_map.count(code)) {
                cout << "unsuccessful\n";
                return false;
            }
            classlist.push_back(code);
        }
        if (students.count(ufid)) {
            cout << "unsuccessful\n";
            return false;
        }
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
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        students.erase(id);
        cout << "successful\n";
        return true;
    } else if (cmd == "dropClass") {
        string id, code;
        if (!(iss >> id >> code)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        if (!ValidateClassCode(code) || !classes_map.count(code)) { cout << "unsuccessful\n"; return false; }
        Student &s = students[id];
        if (!s.classes.count(code)) { cout << "unsuccessful\n"; return false; }
        s.classes.erase(code);
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
        Student &s = students[id];
        if (!s.classes.count(a)) { cout << "unsuccessful\n"; return false; }
        if (s.classes.count(b)) { cout << "unsuccessful\n"; return false; }
        s.classes.erase(a);
        s.classes.insert(b);
        cout << "successful\n";
        return true;
    } else if (cmd == "removeClass") {
        string code;
        if (!(iss >> code)) { cout << "unsuccessful\n"; return false; }
        if (!ValidateClassCode(code)) { cout << "0\n"; return true; }
        int removed_count = 0;
        vector<string> to_remove;
        for (auto &kv : students) {
            const string &id = kv.first;
            const Student &s = kv.second;
            if (s.classes.count(code)) {
                removed_count++;
            }
        }
        // actually remove
        vector<string> delete_ids;
        for (auto &kv : students) {
            string id = kv.first;
            Student &s = students[id];
            if (s.classes.count(code)) {
                s.classes.erase(code);
                if (s.classes.empty()) delete_ids.push_back(id);
            }
        }
        for (auto &id : delete_ids) students.erase(id);
        cout << removed_count << "\n";
        return true;
    } else if (cmd == "toggleEdgesClosure") {
        int N;
        if (!(iss >> N)) { cout << "unsuccessful\n"; return false; }
        vector<int> tokens;
        int x;
        while (iss >> x) tokens.push_back(x);
        if ((int)tokens.size() != 2*N) { cout << "unsuccessful\n"; return false; }
        for (int i = 0; i < N; ++i) {
            int a = tokens[2*i];
            int b = tokens[2*i+1];
            int u = min(a,b), v = max(a,b);
            auto it = edges_metadata.find({u,v});
            if (it == edges_metadata.end()) {
                // they said we'll always be given valid edges; but handle gracefully
                continue;
            }
            it->second.second = !it->second.second;
            // update graph adjacency lists
            for (auto &e : graph[a]) {
                if (e.to == b) { e.closed = it->second.second; }
            }
            for (auto &e : graph[b]) {
                if (e.to == a) { e.closed = it->second.second; }
            }
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
        // bfs or dijkstra ignoring weights
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
            for (auto &e : graph.at(u)) {
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
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        Student &s = students[id];
        cout << "Name: " << s.name << "\n";
        // collect class codes sorted lexicographically
        vector<string> codes(s.classes.begin(), s.classes.end());
        sort(codes.begin(), codes.end());
        // compute distances from residence once
        auto dists = DijkstraDistances(s.residence);
        for (auto &c : codes) {
            int total = -1;
            if (classes_map.count(c)) {
                int loc = classes_map[c].location_id;
                if (dists.count(loc)) total = dists[loc];
            }
            cout << c << " | Total Time: " << total << "\n";
        }
        return true;
    } else if (cmd == "printStudentZone") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        Student &s = students[id];
        // for each class, get path nodes from residence to class location
        set<int> nodes;
        vector<int> unreachable_classes;
        for (auto &c : s.classes) {
            if (!classes_map.count(c)) {
                unreachable_classes.push_back(-1);
                continue;
            }
            int loc = classes_map[c].location_id;
            int out_dist;
            auto path = DijkstraPathNodes(s.residence, loc, out_dist);
            if (out_dist < 0) {
                // if unreachable skip, but spec: will not test when student has no reachable classes
                continue;
            }
            for (int v : path) nodes.insert(v);
        }
        // build edges of subgraph
        auto subedges = BuildSubgraphEdgesForNodes(nodes);
        vector<int> node_list(nodes.begin(), nodes.end());
        int cost = 0;
        if (!node_list.empty()) {
            cost = KruskalMSTCost((int)node_list.size(), subedges, node_list);
        }
        cout << "Student Zone Cost For " << s.name << ": " << cost << "\n";
        return true;
    } else if (cmd == "verifySchedule") {
        string id;
        if (!(iss >> id)) { cout << "unsuccessful\n"; return false; }
        if (!students.count(id)) { cout << "unsuccessful\n"; return false; }
        Student &s = students[id];
        if (s.classes.size() <= 1) { cout << "unsuccessful\n"; return true; }
        // build vector of class codes with times
        struct CRec { string code; int start; int end; int loc; };
        vector<CRec> recs;
        for (auto &c : s.classes) {
            if (!classes_map.count(c)) continue;
            ClassInfo ci = classes_map[c];
            int st = TimeToMinutes(ci.start_time);
            int en = TimeToMinutes(ci.end_time);
            recs.push_back({c, st, en, ci.location_id});
        }
        sort(recs.begin(), recs.end(), [](const CRec&a,const CRec&b){
            return a.start < b.start;
        });
        cout << "Schedule Check for " << s.name << ":\n";
        bool any = false;
        for (size_t i = 0; i + 1 < recs.size(); ++i) {
            any = true;
            int from = recs[i].loc;
            int to = recs[i+1].loc;
            int shortest = -1;
            int outd;
            auto path = DijkstraPathNodes(from, to, outd);
            if (outd >= 0) shortest = outd;
            int gap = recs[i+1].start - recs[i].end;
            if (shortest <= gap && shortest >= 0) {
                cout << recs[i].code << " - " << recs[i+1].code << " \"Can make it!\"\n";
            } else {
                cout << recs[i].code << " - " << recs[i+1].code << " \"Cannot make it!\"\n";
            }
        }
        if (!any) cout << "unsuccessful\n";
        return true;
    } else {
        cout << "unsuccessful\n";
        return false;
    }
    return false;
}
