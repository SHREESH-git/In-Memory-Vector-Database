// Graph-Based Greedy Approximate Nearest neighbor Search:

#include<bits/stdc++.h>
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

int main(){
    Graph db;
    // NOTE: Drop to 10,000 vectors here because building the graph
    // takes O(N^2) upfront computation. This is the "Write Penalty".
    int num_vectors = 10000; 

    cout << "Building Graph with " << num_vectors << " nodes (This takes a moment).." << endl;
    
    for(int i = 0; i < num_vectors; ++i) {
        vector<float> v;
        for(int j = 0; j < 128; ++j) {
            v.push_back(static_cast<float>(rand()) / RAND_MAX);
        }
        db.insert(v);
    }
    cout << "Graph construction complete!" << endl;

    vector<float> query(128, 0.5f);
    cout << "\nStarting Greedy Graph Search..." << endl;

    auto start = chrono::high_resolution_clock::now();
    int result = db.greedy_search(query);
    auto end = chrono::high_resolution_clock::now();
    
    chrono::duration<double, milli> elapsed = end - start;

    cout << "The closest match is Vector Index: " << result << endl;
    cout << "Search time for graph traversal: " << elapsed.count() << " ms" << endl;

    return 0;
}


/*
Search:
Average: O(log N × d)
Worst Case: O(N × d)

Insertion:
O(N × d)

Graph Construction:
O(N² × d)

Space:
O(N × (d + M))
*/