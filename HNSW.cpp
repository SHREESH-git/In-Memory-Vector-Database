#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cfloat>
#include <algorithm>
#include <queue>
#include <unordered_set> // Added to keep track of visited nodes


// THIRD-PARTY LIBRARIES (Network Layer)

#include "httplib.h"
#include "json.hpp"

using namespace std;
using json = nlohmann::json;


// THE CORE ENGINE: True HNSW with efSearch

struct Node {
    vector<float> data;
    vector<vector<int>> neighbors; // Multi-layer Graph (Array of Arrays)
    int level;
    Node(const vector<float>& vec, int l) : data(vec), level(l) {
        neighbors.resize(level + 1);
    }
};

class VectorDatabase {
    private:
        vector<Node> nodes;
        int M = 16; 
        
        // --- HNSW PARAMETERS ---
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

        
        // PRIORITY QUEUE CANDIDATE SEARCH (The brain of HNSW)
        
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
                
                // OPTIMIZATION: If the closest node left to explore is further away than 
                // the WORST node on our VIP list, we can safely stop exploring this branch!
                if (curr.first > top_ef_nodes.top().first) {
                    break; 
                }

                for (int neighbor : nodes[curr.second].neighbors[layer]) {
                    if (visited.find(neighbor) == visited.end()) {
                        visited.insert(neighbor);
                        float dist = squared_euclidean_distance(query, nodes[neighbor].data);
                        
                        // If we haven't filled our VIP list, OR this neighbor is closer than the worst VIP
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

            // Phase 1: Zoom down the top layers (Express Train). ef=1 because we just want speed, not accuracy here.
            for (int l = max_level; l > level; --l) {
                // search_layer returns a heap. We just grab the single best item from it.
                auto W = search_layer(vec, curr_node, 1, l);
                
                pair<float, int> best_node = {FLT_MAX, -1};
                while(!W.empty()) {
                    if(W.top().first < best_node.first) best_node = W.top();
                    W.pop();
                }
                curr_node = best_node.second;
            }

            // Phase 2: We hit our target layer. Connect and drop down to base layer.
            for (int l = min(max_level, level); l >= 0; --l) {
                
                // efConstruction: Find a pool of candidates to wire connections to
                auto W = search_layer(vec, curr_node, efConstruction, l);

                // Transfer candidates to a vector so we can easily pick the top M
                vector<pair<float, int>> candidates_list;
                while(!W.empty()){
                    candidates_list.push_back(W.top());
                    W.pop();
                }
                // Sort to get smallest distances
                sort(candidates_list.begin(), candidates_list.end());

                // Wire bidirectional edges to the top M candidates
                int num_connections = min((int)candidates_list.size(), M);
                for (int i = 0; i < num_connections; ++i) {
                    int n = candidates_list[i].second;
                    if (n != new_idx) { 
                        nodes[new_idx].neighbors[l].push_back(n);
                        nodes[n].neighbors[l].push_back(new_idx);
                    }
                }
                curr_node = candidates_list[0].second; // The closest node is our entry point for the next layer down
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

            // Phase 1: Express Train down to Layer 0
            for (int l = max_level; l > 0; --l) {
                auto W = search_layer(query, curr_node, 1, l);
                pair<float, int> best_node = {FLT_MAX, -1};
                while(!W.empty()) {
                    if(W.top().first < best_node.first) best_node = W.top();
                    W.pop();
                }
                curr_node = best_node.second;
            }

            // Phase 2: Local Train (Layer 0) using efSearch for maximum accuracy
            auto W = search_layer(query, curr_node, efSearch, 0);
            
            // Find the absolute best among the 'ef' VIP candidates
            pair<float, int> best_node = {FLT_MAX, -1};
            while(!W.empty()) {
                if(W.top().first < best_node.first) best_node = W.top();
                W.pop();
            }
            
            return {best_node.second, best_node.first};
        }
};


// HTTP SERVER 

int main() {
    VectorDatabase db;

    cout << "Initializing Engine..." << endl;
    int num_vectors = 10000; 
    cout << "Ingesting " << num_vectors << " base vectors using True HNSW (efConstruction=32)..." << endl;
    
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
    cout << "HNSW Ingestion Time: " << elapsed_insert.count() << " ms" << endl;

    httplib::Server svr;

    svr.Post("/insert", [&](const httplib::Request& req, httplib::Response& res) {
        // [omitted for brevity, keeping exact same JSON logic as before]
        try {
            auto j = json::parse(req.body);
            vector<float> vec = j["vector"].get<vector<float>>();
            auto start = chrono::high_resolution_clock::now();
            db.insert(vec);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = end - start;
            json response = {{"status", "success"}, {"message", "Vector indexed."}, {"latency_ms", elapsed.count()}};
            res.set_content(response.dump(), "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error": "Invalid JSON."})", "application/json"); }
    });

    svr.Post("/search", [&](const httplib::Request& req, httplib::Response& res) {
        try {
            auto j = json::parse(req.body);
            vector<float> query = j["query"].get<vector<float>>();
            auto start = chrono::high_resolution_clock::now();
            pair<int, float> result = db.search_hnsw(query);
            auto end = chrono::high_resolution_clock::now();
            chrono::duration<double, milli> elapsed = end - start;
            json response = {
                {"status", "success"}, 
                {"nearest_node_index", result.first}, 
                {"distance_score", result.second}, 
                {"latency_ms", elapsed.count()}, 
                {"algorithm", "HNSW efSearch"}
            };
            res.set_content(response.dump(), "application/json");
        } catch (...) { res.status = 400; res.set_content(R"({"error": "Invalid JSON."})", "application/json"); }
    });

    cout << "Vector Database API is LIVE!" << endl;
    cout << "Listening for connections on http://localhost:8080" << endl;

    svr.listen("0.0.0.0", 8080);
    return 0;
}