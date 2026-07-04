// HNSW with efSearch and efConstruction with candidate priority queue search
// Approach taken from HNSW paper

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <queue>
#include <unordered_set>
#include <fstream>
#include <sstream>

using namespace std;


struct Node {
    vector<float> data;
    vector<vector<int>> neighbors; 
    int level;
    Node(const vector<float>& vec, int l) : data(vec), level(l) {
        neighbors.resize(level + 1);
    }
};

class VectorDatabase {
    private:
        vector<Node> nodes;
        int M = 16; 
        
        // HNSW PARAMETERS 
        int efConstruction = 32; // VIP List size during insertion
        int efSearch = 32;       // VIP List size during queries
        
        double level_mult = 1.0 / log(16.0); 
        int max_level = -1;
        int enter_point = -1;

        // Random Level Generation
        int get_random_level() {
            double r = ((double)rand() / (RAND_MAX));
            if (r == 0.0) r = 0.000001; 
            return (int)(-log(r) * level_mult);
        }

        
        // PRIORITY QUEUE CANDIDATE SEARCH
        
        priority_queue<pair<float, int>> search_layer(const vector<float>& query, int ep, int ef, int layer) {
            unordered_set<int> visited;
            visited.insert(ep);

            // Min-Heap: Keeps track of nodes we NEED to explore (closest at top)
            priority_queue<pair<float, int>, vector<pair<float, int>>, greater<pair<float, int>>> candidates;
            
            // Max-Heap: The VIP List. Keeps track of the best 'ef' nodes we have FOUND (furthest at top to easily kick them out)
            priority_queue<pair<float, int>> top_ef_nodes;

            float initial_dist = squared_euclidean_distance(query, nodes[ep].data);
            candidates.push({initial_dist, ep});
            top_ef_nodes.push({initial_dist, ep});

            while (!candidates.empty()) {
                auto curr = candidates.top();
                candidates.pop();
                
                /* OPTIMIZATION: If the closest node left to explore is further away than 
                 the WORST node on our VIP list, we can safely stop exploring this branch */
                if (curr.first > top_ef_nodes.top().first) {
                    break; 
                }

                for (int neighbor : nodes[curr.second].neighbors[layer]) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        float dist = squared_euclidean_distance(query, nodes[neighbor].data);
                        
                        // If we haven't filled our VIP list, or this neighbor is closer than the worst VIP
                        if (top_ef_nodes.size() < ef || dist < top_ef_nodes.top().first) {
                            candidates.push({dist, neighbor});
                            top_ef_nodes.push({dist, neighbor});
                            
                            // Kick the worst node out of the VIP list if we exceed 'ef'
                            if (top_ef_nodes.size() > ef) {
                                top_ef_nodes.pop();
                            }
                        }
                    }
                }
            }
            return top_ef_nodes; // Returns the Max-Heap VIP list
        }

    public:
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        // HNSW Insertion (efConstruction)
        void insert(const vector<float>& vec) {
            int level = get_random_level();
            int new_idx = nodes.size();
            nodes.emplace_back(vec, level);

            if (enter_point == -1) {
                enter_point = new_idx;
                max_level = level;
                return;
            }

            int curr_node = enter_point;

            // Phase 1: Zoom down the top layers.
            for (int l = max_level; l > level; --l) {
                // search_layer returns a heap. We just grab the single best node from it.
                auto W = search_layer(vec, curr_node, 1, l);
                
                pair<float, int> best_node = {FLT_MAX, -1};
                while(!W.empty()) {
                    if(W.top().first < best_node.first) best_node = W.top();
                    W.pop();
                }
                curr_node = best_node.second;
            }

            // Phase 2: We hit our target layer. Connect and drop down to layer 0.
            for (int l = min(max_level, level); l >= 0; --l) {
                
                // efConstruction: Find a pool of candidates to make connections to
                auto W = search_layer(vec, curr_node, efConstruction, l);

                // Transfer candidates to a vector so we can easily pick the top M
                vector<pair<float, int>> candidates_list;
                while(!W.empty()){
                    candidates_list.push_back(W.top());
                    W.pop();
                }
                // Sort to get smallest distances
                sort(candidates_list.begin(), candidates_list.end());

                // Connect bidirectional edges to the top M candidates
                int num_connections = min((int)candidates_list.size(), M);
                for (int i = 0; i < num_connections; ++i) {
                    int n = candidates_list[i].second;
                    if (n != new_idx) { 
                        nodes[new_idx].neighbors[l].push_back(n);
                        nodes[n].neighbors[l].push_back(new_idx);
                    }
                }
                curr_node = candidates_list[0].second; // The closest node isentry point for the next layer down
            }

            if (level > max_level) {
                max_level = level;
                enter_point = new_idx;
            }
        }

        // HNSW Query Search (efSearch)
        pair<int, float> search_hnsw(const vector<float>& query) {
            if (nodes.empty()) return {-1, 0.0f};

            int curr_node = enter_point;

            // Phase 1: search down to Layer 0
            for (int l = max_level; l > 0; --l) {
                auto W = search_layer(query, curr_node, 1, l);
                pair<float, int> best_node = {FLT_MAX, -1};
                while(!W.empty()) {
                    if(W.top().first < best_node.first) best_node = W.top();
                    W.pop();
                }
                curr_node = best_node.second;
            }

            // Phase 2: Layer 0 search using efSearch
            auto W = search_layer(query, curr_node, efSearch, 0);
            
            // Find the best among the 'ef' VIP candidates
            pair<float, int> best_node = {FLT_MAX, -1};
            while(!W.empty()) {
                if(W.top().first < best_node.first) best_node = W.top();
                W.pop();
            }
            
            return {best_node.second, best_node.first};
        }
};

// Helper function to load dataset
vector<vector<float>> load_dataset(const string& filename) {
    vector<vector<float>> data;
    ifstream in(filename);
    if (!in.is_open()) {
        cerr << "Could not open " << filename << endl;
        return data;
    }
    string line;
    while (getline(in, line)) {
        stringstream ss(line);
        vector<float> vec;
        float val;
        while (ss >> val) {
            vec.push_back(val);
        }
        if (!vec.empty()) data.push_back(vec);
    }
    in.close();
    return data;
}

int main() {
    VectorDatabase db;

    cout << "Loading dataset.txt..." << endl;
    vector<vector<float>> dataset = load_dataset("dataset.txt");
    if (dataset.empty()) {
        cerr << "Failed to load dataset." << endl;
        return 1;
    }

    cout << "Inserting " << dataset.size() << " base vectors using Optimized HNSW (efConstruction=32, M=16):" << endl;
    
    srand(42); 
    auto start_insert = chrono::high_resolution_clock::now();
    for (const auto& vec : dataset) {
        db.insert(vec);
    }
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;

    cout << "Graph Construction Complete!" << endl;
    cout << "HNSW Insertion Time: " << elapsed_insert.count() << " ms" << endl;


    cout << "Loading queries.txt..." << endl;
    vector<vector<float>> queries = load_dataset("queries.txt");
    if (queries.empty()) {
        cerr << "Failed to load queries." << endl;
        return 1;
    }

    cout << "\nRunning Optimized HNSW Search Benchmark.." << endl;

    double total_search_time = 0.0;
    volatile long long dummy_sum = 0; // Prevent compiler optimization
    for(const auto& query : queries){
        auto start = chrono::high_resolution_clock::now();
        pair<int,float> result = db.search_hnsw(query);
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end - start;
        total_search_time += elapsed.count();
        dummy_sum += result.first;
    }

    cout << "Total search time for " << queries.size() << " queries: " << total_search_time << " ms" << endl;
    cout << "Average Search Latency: " << (total_search_time / queries.size()) << " ms" << endl;
    cout << "QPS (Queries/sec): " << (1000.0 / (total_search_time / queries.size())) << endl;

    return 0;
}
