#include<bits/stdc++.h>
using namespace std;

class Vector {
    private:
        vector<vector<float>> database;
    public:

        // Store a high dimensional vector
        void insert(const vector<float>&vec){
            database.push_back(vec);
            cout << "Vector inserted successfully." << endl;
        }

        // Calculating the distance between two vectors(Euclidean distance)
        static float euclidean_distance(const vector<float>&a, const vector<float>&b){
            if(a.size()!=b.size()) throw invalid_argument("Vectors must have same dimension.");

            float sum = 0.0f;
            for(size_t i =0 ; i<a.size(); i++){
                sum += pow((a[i]-b[i]),2);

            }

            return sqrt(sum);
        }

        // Measures the angle between two vectors (1.0 = identical, 0.0 = orthogonal , -1.0 = opposite)
        static float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
            if (a.size() != b.size()) throw std::invalid_argument("Vector dimensions must match.");
            float dot = 0.0f;
            float norm_a = 0.0f;
            float norm_b = 0.0f;

            for(size_t i=0; i<a.size(); i++){
                dot += a[i] * b[i];
                norm_a += a[i] * a[i];
                norm_b += b[i] * b[i];
            }

            if (norm_a == 0 || norm_b == 0) return 0.0f;
            return dot / (sqrt(norm_a) * sqrt(norm_b));
        }

        // (Cosine Similarity) Brute force linear nearest neighbour search
        int search(const vector<float>& query){
            if (database.empty()) throw std::runtime_error("Database is empty.");
        
            int best_index = -1;
            float best_score = -1.0f; // Cosine similarity ranges from -1 to 1

            for (size_t i = 0; i < database.size(); ++i) {
                float score = cosine_similarity(query, database[i]);
                if (score > best_score) {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
            return best_index;
        }

        // (Euclidean distance) Brute force linear nearest neighbour search
        int search_euclidean(const vector<float>& query){
            if (database.empty()) throw std::runtime_error("Database is empty.");
        
            int best_index = -1;
            float best_score = FLT_MAX;

            for (size_t i = 0; i < database.size(); ++i) {
                float score = euclidean_distance(query, database[i]);
                if (score < best_score) {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
            return best_index;
        }
};

int main() {
    Vector db;

    db.insert({0.1f, 0.8f, 0.3f}); // Vector 0 // Financial Stress
    db.insert({0.9f, 0.2f, 0.1f}); // Vector 1 // Salary Delay
    db.insert({0.4f, 0.5f, 0.6f}); // Vector 2 // Low Credit Score

    vector<float> query = {0.8f, 0.3f, 0.2f};

    cout << "Starting brute-force linear vector search..." << endl;

    // Using both:
    int closest_idx_1 = db.search(query);
    int closest_idx_2 = db.search_euclidean(query);

    cout << "The closest mathematical match(using Cosine Similarity) is Vector Index: " << closest_idx_1 << endl;
    cout << "The closest mathematical match(using Euclidean distance) is Vector Index: " << closest_idx_2 << endl;

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