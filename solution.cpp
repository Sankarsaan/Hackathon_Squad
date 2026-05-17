#include <iostream>
#include <vector>
#include <queue>
#include <algorithm>
#include <chrono>
#include <random>

using namespace std;

// Two-pointer subset check for Domination Reduction
bool is_subset(const vector<int>& super_vec, const vector<int>& sub_vec, const vector<bool>& active) {
    int i = 0, j = 0;
    while (i < super_vec.size() && j < sub_vec.size()) {
        if (!active[sub_vec[j]]) { j++; continue; }
        if (!active[super_vec[i]]) { i++; continue; }
        if (super_vec[i] < sub_vec[j]) i++;
        else if (super_vec[i] == sub_vec[j]) { i++; j++; }
        else return false;
    }
    while (j < sub_vec.size() && !active[sub_vec[j]]) j++;
    return j == sub_vec.size();
}

// ---------------------------------------------------------
// EXACT SOLVER: Branch and Bound with Clique Cover Pruning
// ---------------------------------------------------------
long long best_exact_score = 0;
vector<bool> best_exact_state;

// Upper Bound via Greedy Weighted Clique Cover
long long get_clique_cover_bound(const vector<int>& available, const vector<vector<int>>& adj, const vector<long long>& S) {
    long long bound = 0;
    vector<bool> local_covered(S.size(), false);
    
    // Sort descending by weight for greedy clique packing
    vector<pair<long long, int>> sorted_nodes;
    for (int u : available) sorted_nodes.push_back({S[u], u});
    sort(sorted_nodes.rbegin(), sorted_nodes.rend());

    for (auto p : sorted_nodes) {
        int u = p.second;
        if (local_covered[u]) continue;
        
        long long max_clique_weight = S[u];
        local_covered[u] = true;
        
        for (int v : adj[u]) {
            if (!local_covered[v] && find(available.begin(), available.end(), v) != available.end()) {
                max_clique_weight = max(max_clique_weight, S[v]);
                local_covered[v] = true;
            }
        }
        bound += max_clique_weight;
    }
    return bound;
}

void exact_mwis(vector<int> available, long long current_score, const vector<vector<int>>& adj, vector<bool>& current_state, const vector<long long>& S) {
    if (available.empty()) {
        if (current_score > best_exact_score) {
            best_exact_score = current_score;
            best_exact_state = current_state;
        }
        return;
    }

    // PRUNING: Upper Bound Check
    if (current_score + get_clique_cover_bound(available, adj, S) <= best_exact_score) {
        return; 
    }

    // SMART BRANCHING: Pick highest dynamic degree, tie-break by weight
    int best_idx = 0;
    int max_deg = -1;
    for (int i = 0; i < available.size(); ++i) {
        int u = available[i];
        int deg = 0;
        for (int v : adj[u]) if (find(available.begin(), available.end(), v) != available.end()) deg++;
        
        if (deg > max_deg || (deg == max_deg && S[u] > S[available[best_idx]])) {
            max_deg = deg;
            best_idx = i;
        }
    }

    int u = available[best_idx];
    available.erase(available.begin() + best_idx);

    // Branch 1: INCLUDE u
    vector<int> next_available;
    for (int v : available) {
        bool is_neighbor = false;
        for (int neighbor : adj[u]) if (v == neighbor) { is_neighbor = true; break; }
        if (!is_neighbor) next_available.push_back(v);
    }

    current_state[u] = true;
    exact_mwis(next_available, current_score + S[u], adj, current_state, S);
    current_state[u] = false;

    // Branch 2: EXCLUDE u
    exact_mwis(available, current_score, adj, current_state, S);
}

// ---------------------------------------------------------
// MAIN EXECUTABLE
// ---------------------------------------------------------
int main() {
    auto start_time = chrono::high_resolution_clock::now();
    double total_time_limit = 295.0; // 5 minute hard buffer

    ios_base::sync_with_stdio(false); cin.tie(NULL);

    int n; long long m;
    cin >> n >> m;

    vector<long long> S(n + 1);
    for (int i = 1; i <= n; ++i) cin >> S[i];

    vector<vector<int>> adj(n + 1);
    for (long long i = 0; i < m; ++i) {
        int u, v; cin >> u >> v;
        adj[u].push_back(v); 
        adj[v].push_back(u);
    }
    for (int i = 1; i <= n; ++i) sort(adj[i].begin(), adj[i].end());

    vector<bool> active(n + 1, true);
    vector<bool> in_team(n + 1, false);
    vector<int> active_degree(n + 1, 0);
    queue<int> q;

    for (int i = 1; i <= n; ++i) { 
        active_degree[i] = adj[i].size();
        q.push(i); 
    }

    // ==========================================
    // PHASE 1: Incremental Reductions
    // ==========================================
    while (!q.empty()) {
        int u = q.front(); q.pop();
        if (!active[u]) continue;

        long long rival_sum = 0; int act_neighbors = 0;
        for (int v : adj[u]) {
            if (active[v]) { rival_sum += S[v]; act_neighbors++; }
        }

        if (S[u] >= rival_sum) {
            in_team[u] = true; active[u] = false;
            for (int v : adj[u]) if (active[v]) {
                active[v] = false;
                for (int w : adj[v]) if (active[w]) { active_degree[w]--; q.push(w); }
            }
            continue;
        }

        // Domination restricted to non-massive hubs to prevent TLE
        if (act_neighbors > 0 && act_neighbors <= 50) {
            for (int v : adj[u]) {
                if (active[v] && S[v] <= S[u] && active_degree[v] >= active_degree[u]) {
                    if (is_subset(adj[v], adj[u], active)) {
                        active[v] = false; 
                        for (int w : adj[v]) if (active[w]) { active_degree[w]--; q.push(w); }
                    }
                }
            }
        }
    }

    // ==========================================
    // PHASE 2: Component Discovery (BFS)
    // ==========================================
    vector<bool> visited(n + 1, false);
    vector<vector<int>> components;
    int total_active_nodes = 0;

    for (int i = 1; i <= n; ++i) {
        if (active[i] && !visited[i]) {
            vector<int> comp; queue<int> bfs_q;
            bfs_q.push(i); visited[i] = true;
            while (!bfs_q.empty()) {
                int curr = bfs_q.front(); bfs_q.pop();
                comp.push_back(curr);
                for (int neighbor : adj[curr]) {
                    if (active[neighbor] && !visited[neighbor]) {
                        visited[neighbor] = true; bfs_q.push(neighbor);
                    }
                }
            }
            components.push_back(comp);
            total_active_nodes += comp.size();
        }
    }

    // ==========================================
    // PHASE 3: Dynamic Component Solving
    // ==========================================
    mt19937 rng(1337);
    
    for (const auto& comp : components) {
        if (comp.size() <= 26) { //2^26 is close to 10^8 so can be solved within 1 sec to get the exact solution of that component
            // --------------------------------------
            // SUB-PHASE A: Exact Solver
            // --------------------------------------
            // Inject Lower Bound
            vector<pair<double, int>> ratio;
            for (int u : comp) ratio.push_back({(double)S[u] / (active_degree[u] + 1.0), u});
            sort(ratio.rbegin(), ratio.rend()); 
            
            vector<bool> temp_state(n + 1, false);
            long long initial_lb = 0;
            for (auto p : ratio) {
                int u = p.second; bool can_add = true;
                for (int v : adj[u]) if (temp_state[v]) { can_add = false; break; }
                if (can_add) { temp_state[u] = true; initial_lb += S[u]; }
            }

            best_exact_score = initial_lb;
            best_exact_state = temp_state; 
            fill(temp_state.begin(), temp_state.end(), false);
            
            exact_mwis(comp, 0, adj, temp_state, S);
            
            for (int u : comp) {
                if (best_exact_state[u]) in_team[u] = true;
                active[u] = false; 
            }
        } else {
            // --------------------------------------
            // SUB-PHASE B: HILS Heuristic
            // --------------------------------------
            auto now = chrono::high_resolution_clock::now();
            double elapsed = chrono::duration<double>(now - start_time).count();
            double time_left = max(0.0, total_time_limit - elapsed);
            double comp_time_limit = time_left * ((double)comp.size() / total_active_nodes);
            auto comp_start_time = chrono::high_resolution_clock::now();

            vector<pair<double, int>> ratio;
            for (int u : comp) ratio.push_back({(double)S[u] / (active_degree[u] + 1.0), u});
            sort(ratio.rbegin(), ratio.rend());

            vector<bool> comp_state(n + 1, false);
            long long comp_score = 0;

            for (auto p : ratio) {
                int u = p.second; bool can_add = true;
                for (int v : adj[u]) if (comp_state[v]) { can_add = false; break; }
                if (can_add) { comp_state[u] = true; comp_score += S[u]; }
            }

            long long best_comp_score = comp_score;
            vector<bool> best_comp_state = comp_state;
            vector<int> nodes = comp;

            while (true) {
                if (chrono::duration<double>(chrono::high_resolution_clock::now() - comp_start_time).count() > comp_time_limit) break;

                bool improved = false;
                shuffle(nodes.begin(), nodes.end(), rng);

                for (int u : nodes) {
                    if (comp_state[u]) continue;
                    long long conflict_weight = 0;
                    vector<int> conflicts;
                    for (int v : adj[u]) if (comp_state[v]) { conflict_weight += S[v]; conflicts.push_back(v); }

                    // (w, 1)-swap & Plateau Search
                    if (S[u] > conflict_weight || (S[u] == conflict_weight && rng() % 100 < 10)) {
                        comp_state[u] = true; comp_score += S[u];
                        for (int v : conflicts) { comp_state[v] = false; comp_score -= S[v]; }
                        if (S[u] > conflict_weight) improved = true;
                        
                        if (comp_score > best_comp_score) {
                            best_comp_score = comp_score; best_comp_state = comp_state;
                        }
                        continue;
                    }
                    
                    // (1, 2)-swap
                    if (conflicts.size() == 1) {
                        int kicked = conflicts[0];
                        for (int w : nodes) {
                            if (w == u || comp_state[w]) continue;
                            auto it = lower_bound(adj[w].begin(), adj[w].end(), u);
                            if (it != adj[w].end() && *it == u) continue;

                            bool w_valid = true;
                            for (int x : adj[w]) if (comp_state[x] && x != kicked) { w_valid = false; break; }

                            if (w_valid && (S[u] + S[w] > S[kicked])) {
                                comp_state[u] = true; comp_state[w] = true; comp_state[kicked] = false;
                                comp_score += (S[u] + S[w] - S[kicked]);
                                improved = true;
                                if (comp_score > best_comp_score) { best_comp_score = comp_score; best_comp_state = comp_state; }
                                break; 
                            }
                        }
                    }
                }

                if (!improved && !nodes.empty()) {
                    int force_u = nodes[rng() % nodes.size()];
                    if (!comp_state[force_u]) {
                        comp_state[force_u] = true; comp_score += S[force_u];
                        for (int v : adj[force_u]) if (comp_state[v]) { comp_state[v] = false; comp_score -= S[v]; }
                    }
                }
            }
            for (int u : comp) {
                if (best_comp_state[u]) in_team[u] = true;
                active[u] = false;
            }
        }
        total_active_nodes -= comp.size(); 
    }

    // ==========================================
    // PHASE 4: Final Greedy Post-Processing
    // ==========================================
    // Just in case any nodes slipped through isolated
    vector<pair<long long, int>> final_sweep;
    for (int i = 1; i <= n; ++i) if (!in_team[i]) final_sweep.push_back({S[i], i});
    sort(final_sweep.rbegin(), final_sweep.rend());
    
    for (auto p : final_sweep) {
        int u = p.second;
        bool valid = true;
        for (int v : adj[u]) if (in_team[v]) { valid = false; break; }
        if (valid) in_team[u] = true;
    }

    // ==========================================
    // OUTPUT GENERATION
    // ==========================================
    long long final_score = 0;
    vector<int> final_team;
    for (int i = 1; i <= n; ++i) {
        if (in_team[i]) {
            final_team.push_back(i);
            final_score += S[i];
        }
    }

    cout << final_score << "\n";
    for (int i = 0; i < final_team.size(); ++i) {
        cout << final_team[i] << (i == final_team.size() - 1 ? "" : " ");
    }
    cout << "\n";

    return 0;
}
