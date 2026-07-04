// Simple Hierarchical Navigable Small World (HNSW) Search & Insertion (M=16)

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <unordered_set>
#include <fstream>
#include <sstream>

using namespace std;

struct Node {
    vector<float> data;
    // 2D Array: neighbors[level] holds connections for that specific layer
    vector<vector<int>> neighbors; 
    int level;

    Node(const vector<float>& vec, int l) : data(vec), level(l) {
        neighbors.resize(level + 1); // A node in higher level stays in lower levels too
    }
};

class VectorDatabase {
    private:
        vector<Node> nodes;
        int M = 16; // Maximum connections per node per layer
        
        // Determines how quickly layers thin out.
        double level_mult = 1.0 / log(16.0); // From Reserch Paper (level >= l) = e^(-l / level_multiplier)
        
        int max_level = -1;
        int enter_point = -1; // The entry node at the absolute highest layer

        // Probabilistic exponential decay for layer assignment
        int get_random_level() {
            double r = ((double)rand() / (RAND_MAX)); // 0.0 <= r < 1.0 
            if (r == 0.0) r = 0.000001; // Prevent log(0)
            return (int)(-log(r) * level_mult);
        }

        // Greedy search restricted to ONE specific layer
        int greedy_search_level(const vector<float>& query, int start_node, int level) {
            int curr_node = start_node;
            float min_dist = squared_euclidean_distance(query, nodes[curr_node].data);
            
            bool changed = true;
            while(changed) {
                changed = false;
                for (int neighbor : nodes[curr_node].neighbors[level]) {
                    float dist = squared_euclidean_distance(query, nodes[neighbor].data);
                    if (dist < min_dist) {
                        min_dist = dist;
                        curr_node = neighbor;
                        changed = true;
                    }
                }
            }
            return curr_node;
        }

    public:
        // Euclidean Distance
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        // Simple HNSW SEARCH: 
        pair<int, float> search_hnsw(const vector<float>& query) {
            if (nodes.empty()) return {-1, 0.0f};

            int curr_node = enter_point;

            // Phase 1: Zoom through the upper sparse layers 
            for (int l = max_level; l > 0; --l) {
                curr_node = greedy_search_level(query, curr_node, l);
            }

            // Phase 2: Perform the final dense search at the bottom layer (Layer 0)
            curr_node = greedy_search_level(query, curr_node, 0);
            
            return {curr_node, squared_euclidean_distance(query, nodes[curr_node].data)};
        }

        // Connect up to M neighbors per layer to the new inserted node
        void insert(const vector<float>& vec) {
            int level = get_random_level();
            int new_idx = nodes.size();
            nodes.emplace_back(vec, level);

            // First node ever inserted becomes the top entry point
            if (enter_point == -1) {
                enter_point = new_idx;
                max_level = level;
                return;
            }

            int curr_node = enter_point;

            // Phase 1: Fast traverse down the upper layers without making connections
            for (int l = max_level; l > level; --l) {
                curr_node = greedy_search_level(vec, curr_node, l);
            }

            // Phase 2: Once we reach the target level, connect up to M neighbors and drop down to Layer 0
            for (int l = min(max_level, level); l >= 0; --l) {
                // Find nearest neighbor at this specific level
                curr_node = greedy_search_level(vec, curr_node, l);

                // Collect local candidates at this level (curr_node + its neighbors)
                unordered_set<int> candidate_set;
                candidate_set.insert(curr_node);
                for (int neighbor : nodes[curr_node].neighbors[l]) {
                    candidate_set.insert(neighbor);
                }

                // If candidate pool is too small, expand to neighbors' neighbors
                if (candidate_set.size() < M * 2) {
                    vector<int> current_candidates(candidate_set.begin(), candidate_set.end());
                    for (int c : current_candidates) {
                        for (int nn : nodes[c].neighbors[l]) {
                            candidate_set.insert(nn);
                        }
                    }
                }

                // Calculate distances for all candidates
                vector<pair<float, int>> candidate_distances;
                for (int candidate : candidate_set) {
                    if (candidate != new_idx) {
                        float dist = squared_euclidean_distance(vec, nodes[candidate].data);
                        candidate_distances.push_back({dist, candidate});
                    }
                }

                // Sort candidates by distance (closest first)
                sort(candidate_distances.begin(), candidate_distances.end());

                // Connect bidirectionally to up to M closest neighbors at this level
                int connections = min((int)candidate_distances.size(), M);
                for (int i = 0; i < connections; ++i) {
                    int neighbor_idx = candidate_distances[i].second;
                    nodes[new_idx].neighbors[l].push_back(neighbor_idx);
                    nodes[neighbor_idx].neighbors[l].push_back(new_idx);
                }

                // Start from the closest neighbor for the next layer down
                if (!candidate_distances.empty()) {
                    curr_node = candidate_distances[0].second;
                }
            }

            // If this node rolled higher than anything else, it is the new King node
            if (level > max_level) {
                max_level = level;
                enter_point = new_idx;
            }
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

    cout << "Building Simple HNSW Graph (M=16) with " << dataset.size() << " nodes.." << endl;

    // Use a fixed seed for building the graph too to ensure identical random levels across runs
    srand(42);
    auto start_insert = chrono::high_resolution_clock::now();
    for (const auto& vec : dataset) {
        db.insert(vec);
    }
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;

    cout << "HNSW Graph construction complete!" << endl;
    cout << "Construction Latency: " << elapsed_insert.count() << " ms" << endl;

    cout << "Loading queries.txt..." << endl;
    vector<vector<float>> queries = load_dataset("queries.txt");
    if (queries.empty()) {
        cerr << "Failed to load queries." << endl;
        return 1;
    }

    cout << "\nStarting Simple HNSW Greedy Search..." << endl;

    double total_search_time = 0.0;
    volatile long long dummy_sum = 0; // Prevent compiler optimization
    for (const auto& query : queries) {
        auto start_search = chrono::high_resolution_clock::now();
        pair<int, float> result = db.search_hnsw(query);
        auto end_search = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed_search = end_search - start_search;
        total_search_time += elapsed_search.count();
        dummy_sum += result.first;
    }

    cout << "Total search time for " << queries.size() << " queries: " << total_search_time << " ms" << endl;
    cout << "Average search latency: " << (total_search_time / queries.size()) << " ms" << endl;
    cout << "QPS (Queries per second): " << (1000.0 / (total_search_time / queries.size())) << endl;

    return 0;
}
