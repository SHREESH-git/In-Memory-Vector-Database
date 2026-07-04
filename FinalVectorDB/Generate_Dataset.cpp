#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>

using namespace std;

void generate_file(const string& filename, int num_vectors, int dimensions, int seed) {
    ofstream out(filename);
    if (!out.is_open()) {
        cerr << "Failed to open " << filename << " for writing." << endl;
        return;
    }

    srand(seed);

    for (int i = 0; i < num_vectors; ++i) {
        for (int j = 0; j < dimensions; ++j) {
            float val = static_cast<float>(rand()) / RAND_MAX;
            out << val;
            if (j < dimensions - 1) {
                out << " ";
            }
        }
        out << "\n";
    }

    out.close();
    cout << "Successfully generated " << filename << " with " << num_vectors << " vectors." << endl;
}

int main() {
    int dimensions = 128;

    cout << "Generating datasets..." << endl;

    // Generate 10,000 vectors for the database
    generate_file("dataset.txt", 10000, dimensions, 42);

    // Generate 1,000 query vectors
    generate_file("queries.txt", 1000, dimensions, 99);

    cout << "Done!" << endl;
    return 0;
}
