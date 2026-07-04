// Graph-Based Greedy Approximate Nearest neighbor Search:

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

using namespace std;

// Graph Node
struct Node{
    vector<float> data;
    vector<int> neighbors; // The indices of the "friend" vectors
    Node(const vector<float>& vec) : data(vec) {}
};

class Graph{
    private:
        vector<Node> nodes;
        int M = 16; // Maximum friends per Node

    public:
        // OPTIMIZED: Squared Euclidean Distance
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        // Build a single layered Graph
        void insert(const vector<float>& vec){
            Node new_node(vec);
            int new_node_index = nodes.size();
            if (nodes.empty()){
                nodes.push_back(new_node); 
                return;
            }
            // Find the nearest neighbors to connect to
            vector<pair<float,int>> distances;
            for(size_t i=0;i<nodes.size();++i){
                float dist = squared_euclidean_distance(vec,nodes[i].data);
                distances.push_back({dist,i});    
            }
            sort(distances.begin(),distances.end());

            // Connect bidirectional Edges 
            int num_connections = min((int)distances.size(),M);
            for(int i=0;i<num_connections;++i){
                int neighbor_index = distances[i].second;
                new_node.neighbors.push_back(neighbor_index);
                nodes[neighbor_index].neighbors.push_back(new_node_index);
            }
            nodes.push_back(new_node);
        }

        // Greedy Search
        int greedy_search(const vector<float>& query){
            if(nodes.empty()) throw runtime_error("Graph is empty.");

            int current_node = 0; // Start at random Node
            float current_dist = squared_euclidean_distance(query,nodes[current_node].data);  

            while(true){ 
                int best_neighbor_idx = -1;
                float best_neighbor_dist = current_dist;

                // Check the nearest neighbor of current node to the target query
                for(int neighbor_index: nodes[current_node].neighbors){
                    float dist = squared_euclidean_distance(query,nodes[neighbor_index].data); 
                    if(dist<best_neighbor_dist){
                        best_neighbor_dist = dist;
                        best_neighbor_idx = neighbor_index;
                    }
                }
                
                // If a neighbor is closer to the target than the current node
                if(best_neighbor_idx!=-1){
                    current_node = best_neighbor_idx;
                    current_dist = best_neighbor_dist;
                }else{
                    // No closer neighbor found, break the loop
                    break;
                }

            }
            return current_node;
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

int main(){
    Graph db;

    cout << "Loading dataset.txt..." << endl;
    vector<vector<float>> dataset = load_dataset("dataset.txt");
    if (dataset.empty()) {
        cerr << "Failed to load dataset." << endl;
        return 1;
    }

    cout << "Building Graph with " << dataset.size() << " nodes (This takes a moment).." << endl;
    
    auto start_insert = chrono::high_resolution_clock::now();
    for(const auto& vec : dataset) {
        db.insert(vec);
    }
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;

    cout << "Graph construction complete! Time taken: " << elapsed_insert.count() << " ms" << endl;

    cout << "Loading queries.txt..." << endl;
    vector<vector<float>> queries = load_dataset("queries.txt");
    if (queries.empty()) {
        cerr << "Failed to load queries." << endl;
        return 1;
    }

    cout << "\nStarting Greedy Graph Search..." << endl;

    double total_search_time = 0.0;
    volatile long long dummy_sum = 0; // Prevent compiler optimization
    for (const auto& query : queries) {
        auto start = chrono::high_resolution_clock::now();
        int result = db.greedy_search(query);
        auto end = chrono::high_resolution_clock::now();
        chrono::duration<double, milli> elapsed = end - start;
        total_search_time += elapsed.count();
        dummy_sum += result;
    }

    cout << "Total search time for " << queries.size() << " queries: " << total_search_time << " ms" << endl;
    cout << "Average search latency: " << (total_search_time / queries.size()) << " ms" << endl;
    cout << "QPS (Queries per second): " << (1000.0 / (total_search_time / queries.size())) << endl;

    return 0;
}
