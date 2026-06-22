// Comparison of Brute-Force kNN search vs Graph-Based ANN search

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cfloat>
#include <algorithm>

using namespace std;

struct Node {
    vector<float> data;
    vector<int> neighbors;
    Node(const vector<float>& vec) : data(vec) {}
};

class VectorDatabase {
    private:
        vector<Node> nodes;
        int M = 16; 

    public:
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        void insert(const vector<float>& vec) {
            Node new_node(vec);
            int new_node_index = nodes.size();

            if (nodes.empty()) {
                nodes.push_back(new_node);
                return;
            }

            vector<pair<float, int>> distances;
            for (size_t i = 0; i < nodes.size(); ++i) {
                float dist = squared_euclidean_distance(vec, nodes[i].data);
                distances.push_back({dist, i});
            }

            sort(distances.begin(), distances.end());

            int num_connections = min((int)distances.size(), M);
            for (int i = 0; i < num_connections; ++i) {
                int neighbor_index = distances[i].second;
                new_node.neighbors.push_back(neighbor_index);
                nodes[neighbor_index].neighbors.push_back(new_node_index);
            }

            nodes.push_back(new_node);
        }

        // Returns {Index, Distance Score}
        pair<int, float> search_linear(const vector<float>& query) {
            int best_index = -1;
            float best_score = FLT_MAX; 

            for (size_t i = 0; i < nodes.size(); ++i) {
                float score = squared_euclidean_distance(query, nodes[i].data);
                if (score < best_score) {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
            return {best_index, best_score};
        }

        // Returns {Index, Distance Score}
        pair<int, float> search_greedy(const vector<float>& query) {
            if (nodes.empty()) return {-1, 0.0f};

            int current_node = 0; 
            float current_dist = squared_euclidean_distance(query, nodes[current_node].data);

            while (true) {
                int best_neighbor = -1;
                float best_neighbor_dist = current_dist;

                for (int neighbor_index : nodes[current_node].neighbors) {
                    float dist = squared_euclidean_distance(query, nodes[neighbor_index].data);
                    
                    if (dist < best_neighbor_dist) {
                        best_neighbor_dist = dist;
                        best_neighbor = neighbor_index;
                    }
                }

                if (best_neighbor != -1) {
                    current_node = best_neighbor;
                    current_dist = best_neighbor_dist;
                } else {
                    break;
                }
            }
            return {current_node, current_dist};
        }
};

int main() {
    VectorDatabase db;
    int num_vectors = 10000; 

    cout << "Generating " << num_vectors << " random vectors..." << endl;
    
    // Use a fixed seed so I get the exact same random vectors every time I run
    srand(42); 

    // Separate data generation from insertion to get an accurate benchmark
    vector<vector<float>> dataset(num_vectors, vector<float>(128));
    for(int i = 0; i < num_vectors; ++i) {
        for(int j = 0; j < 128; ++j) {
            dataset[i][j] = static_cast<float>(rand()) / RAND_MAX;
        }
    }

    cout << "Building Database & Graph (Insertion).." << endl;
    
    //  INSERTION (WRITE PENALTY) TIMER 
    auto start_insert = chrono::high_resolution_clock::now();
    
    for(int i = 0; i < num_vectors; ++i) {
        db.insert(dataset[i]);
    }
    
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;
    
    cout << "Construction completed" << endl;
    cout << "Graph Insertion Latency : " << elapsed_insert.count() << " ms\n" << endl;

    vector<float> query(128, 0.5f);

    //  LINEAR SEARCH 
    cout << " Brute Force Linear Search (100% Accuracy) " << endl;
    auto start1 = chrono::high_resolution_clock::now();
    pair<int, float> result_linear = db.search_linear(query);
    auto end1 = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed1 = end1 - start1;
    cout << "Found Vector Index : " << result_linear.first << endl;
    cout << "Distance Score     : " << result_linear.second << endl;
    cout << "Latency            : " << elapsed1.count() << " ms\n" << endl;

    //  GREEDY GRAPH SEARCH 
    cout << " Greedy Graph Search (Approximate) " << endl;
    auto start2 = chrono::high_resolution_clock::now();
    pair<int, float> result_greedy = db.search_greedy(query);
    auto end2 = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed2 = end2 - start2;
    cout << "Found Vector Index : " << result_greedy.first << endl;
    cout << "Distance Score     : " << result_greedy.second << endl;
    cout << "Latency            : " << elapsed2.count() << " ms\n" << endl;

    //  CONCLUSION 
    float speedup = elapsed1.count() / elapsed2.count();
    cout << "Conclusion:" << endl;
    cout << "Graph traversal is " << speedup << "x faster." << endl;
    
    return 0;
}