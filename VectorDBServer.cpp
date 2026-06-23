#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <queue> 

// Network LIBRARIES 
#include "httplib.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json; 

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

        // OPTIMIZED INSERTION: Using a Max-Heap Priority Queue
        void insert(const vector<float>& vec) {
            Node new_node(vec);
            int new_node_index = nodes.size();

            if (nodes.empty()) {
                nodes.push_back(new_node);
                return;
            }

            // Max-Heap: keeps the LARGEST distance at the top.
            // This way, I can easily pop() the furthest neighbors and keep only the closest M.
            priority_queue<pair<float, int>> top_candidates;

            for (size_t i = 0; i < nodes.size(); ++i) {
                float dist = squared_euclidean_distance(vec, nodes[i].data);
                top_candidates.push({dist, i});
                
                // If heap exceeds M, kick out the furthest node
                if (top_candidates.size() > M) {
                    top_candidates.pop(); 
                }
            }

            // Extract the surviving closest M nodes and connect the graph
            while(!top_candidates.empty()) {
                int neighbor_index = top_candidates.top().second;
                top_candidates.pop();

                new_node.neighbors.push_back(neighbor_index);
                nodes[neighbor_index].neighbors.push_back(new_node_index);
            }

            nodes.push_back(new_node);
        }

        // Greedy Search Traversal
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

// THE NETWORK LAYER (REST API)

int main() {
    VectorDatabase db;

    int num_vectors = 10000; 
    cout << "Inserting " << num_vectors << " base vectors using Priority Queue Graph.." << endl;
    
    srand(42); 
    auto start_insert = chrono::high_resolution_clock::now();
    for(int i = 0; i < num_vectors; ++i) {
        vector<float> v(128);
        for(int j = 0; j < 128; ++j) v[j] = static_cast<float>(rand()) / RAND_MAX;
        db.insert(v);
    }
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;

    cout << "Graph Construction Complete!" << endl;
    cout << "Insertion Time: " << elapsed_insert.count() << " ms" << endl;

    httplib::Server svr;

    // POST /insert Endpoint
    svr.Post("/insert", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            vector<float> vec = j["vector"].get<vector<float>>();

            if(vec.size() != 128) {
                res.status = 400;
                res.set_content(R"({"error": "Dimension mismatch."})", "application/json");
                return;
            }

            auto start = chrono::high_resolution_clock::now();
            db.insert(vec);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = end - start;

            json response;
            response["status"] = "success";
            response["message"] = "Vector indexed.";
            response["latency_ms"] = elapsed.count();

            res.set_content(response.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error": "Invalid JSON."})", "application/json");
        }
    });

    // POST /search Endpoint
    svr.Post("/search", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            vector<float> query = j["query"].get<vector<float>>();

            if(query.size() != 128) {
                res.status = 400;
                res.set_content(R"({"error": "Dimension mismatch."})", "application/json");
                return;
            }

            auto start = chrono::high_resolution_clock::now();
            pair<int, float> result = db.search_greedy(query);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = end - start;

            json response;
            response["status"] = "success";
            response["nearest_node_index"] = result.first;
            response["distance_score"] = result.second;
            response["latency_ms"] = elapsed.count();
            response["algorithm"] = "Greedy ANN Search";

            res.set_content(response.dump(), "application/json");
        } catch (...) {
            res.status = 400;
            res.set_content(R"({"error": "Invalid JSON."})", "application/json");
        }
    });

    cout << " Vector Database API is LIVE!" << endl;
    cout << "Listening for connections on http://localhost:8080" << endl;

    svr.listen("0.0.0.0", 8080);

    

    return 0;
}

/*
Single Insert:
O(N×d + N log M)

Graph Construction:
O(N²×d + N² log M)
≈ O(N²×d)

Greedy Search (Average):
O(M×logN×d)
≈ O(logN×d)

Greedy Search (Worst Case):
O(N×d)

Space:
O(N×(d+M))
*/