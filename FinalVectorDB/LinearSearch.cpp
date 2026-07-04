// Naive k-Nearest-Neighbour Search

#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cfloat>

using namespace std;

class Vector {
    private:
        vector<vector<float>> database;
    public:
        
        void insert(const vector<float>& vec){
            database.push_back(vec);
        }

        // OPTIMIZED: Squared Euclidean Distance (No expensive sqrt and pow())
        // We use this to prove that even the absolute fastest math is too slow in a Linear Search.
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        // Cosine Similarity
        // Angle between vectors using dot product
        static float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
            float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
            for(size_t i=0; i<a.size(); i++){
                dot += a[i] * b[i];
                norm_a += a[i] * a[i];
                norm_b += b[i] * b[i];
            }
            if (norm_a == 0 || norm_b == 0) return 0.0f;
            return dot / (sqrt(norm_a) * sqrt(norm_b));
        }

        // SEARCH: (Squared Euclidean)
        int search_euclidean(const vector<float>& query){
            int best_index = -1;
            float best_score = FLT_MAX; // Looking for the MINIMUM distance

            for (size_t i = 0; i < database.size(); ++i) {
                float score = squared_euclidean_distance(query, database[i]);
                if (score < best_score) {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
            return best_index;
        }

        // SEARCH: Using Cosine Similarity
        int search(const vector<float>& query){
            int best_index = -1;
            float best_score = -1.0f; // Looking for MAXIMUM similarity

            for (size_t i = 0; i < database.size(); ++i) {
                float score = cosine_similarity(query, database[i]);
                if (score > best_score) {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
            return best_index;
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
    Vector db;

    cout << "Loading dataset.txt..." << endl;
    vector<vector<float>> dataset = load_dataset("dataset.txt");
    if (dataset.empty()) {
        cerr << "Failed to load dataset." << endl;
        return 1;
    }

    auto start_insert = chrono::high_resolution_clock::now();
    for (const auto& vec : dataset) {
        db.insert(vec);
    }
    auto end_insert = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed_insert = end_insert - start_insert;
    cout << "Database construction complete! Time taken: " << elapsed_insert.count() << " ms" << endl;

    cout << "Loading queries.txt..." << endl;
    vector<vector<float>> queries = load_dataset("queries.txt");
    if (queries.empty()) {
        cerr << "Failed to load queries." << endl;
        return 1;
    }

    cout << "\nStarting brute-force linear vector search (Squared Euclidean)" << endl;

    double total_search_time = 0.0;
    volatile long long dummy_sum = 0; // Prevent compiler optimization
    for (const auto& query : queries) {
        auto start = chrono::high_resolution_clock::now();
        int result = db.search_euclidean(query);
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
