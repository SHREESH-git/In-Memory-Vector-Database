// Naive k-Nearest-Neighbour Search

#include<bits/stdc++.h>
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
            if(a.size() != b.size()) throw invalid_argument("Vectors must have same dimension.");
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
            if (a.size() != b.size()) throw std::invalid_argument("Vector dimensions must match.");
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
            if (database.empty()) throw std::runtime_error("Database is empty.");
        
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
            if (database.empty()) throw std::runtime_error("Database is empty.");
        
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

int main() {
    Vector db;
    int num_vectors = 1000000; // 1 Million Vectors

    cout << "Generating " << num_vectors << " 1 Million vectors (This will take ~500MB RAM and a few seconds)..." << endl;
    
    // 1. Generate 1,000,000 dummy vectors (128 dimensions)
    for(int i = 0; i < num_vectors; ++i) {
        vector<float> v;
        for(int j = 0; j < 128; ++j) {
            v.push_back(static_cast<float>(rand()) / RAND_MAX);
        }
        db.insert(v);
    }
    cout << "Data generation complete!" << endl;

    vector<float> query(128, 0.5f);

    cout << "\nStarting brute-force linear vector search (Squared Euclidean)" << endl;

    // 2. Start the timer
    auto start = chrono::high_resolution_clock::now();

    // 3. Squared distance search
    int result = db.search_euclidean(query);
    
    // 4. End the timer
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> elapsed = end - start;

    cout << "The closest match is Vector Index: " << result << endl;
    cout << "Search time for 1,000,000 vectors: " << elapsed.count() << " ms" << endl;
    cout << "\nConclusion: O(N) linear search is too slow for production. HNSW Graph required." << endl;

    return 0;
}

// Time Complexity of each function:
// insert(): O(1) time on average
// euclidean_distance(): O(d) time where d is the dimension of the vector
// cosine_similarity(): O(d) time where d is the dimension of the vector
// search(): O(n*d) time where n is the number of vectors and d is the dimension of the vector

// Space Complexity for search(): O(n) Stores the vector (database) in RAM

// Advantages:
// Zero Disk Latency: Latency eliminated since data is stored in RAM not on SSD/HDDs.
// Computational Power better than secondary storage

// Disadvantages:
// Volatily: RAM is volatile (data is lost when power is off or C++ program crashes)
// Capacity Limitation: Limited by the amount of RAM installed on the system.
// Cost: More expensive than HDDs, but cheaper than SSDs.

// Vector databases in real world use Cosine Similarity because LLM embeddings care more about the angle (the contextual direction) than the magnitude (the length) of the vector.
