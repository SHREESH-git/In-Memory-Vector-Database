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
        
        // Euclidean Distance
        static float squared_euclidean_distance(const vector<float>& a, const vector<float>& b){
            float sum = 0.0f;
            for(size_t i = 0 ; i < a.size(); i++){
                float diff = a[i] - b[i];
                sum += diff * diff;
            }
            return sum;
        }

        vector<Node> nodes;
        int M = 16; // From HNSW paper -> gives good decay rate
        
        // Determines how quickly layers thin out.
        double level_mult = 1.0 / log(16.0); // According to research paper: P(level >= l) = e^(-l / level_multiplier)
        // level_multiplier inc -> more nodes in higher layers -> more memory
        
        int max_level = -1;
        int enter_point = -1; // The "King" node at the absolute highest layer

        // Probabilistic exponential decay for layer assignment
        int get_random_level() {
            double r = ((double)rand() / (RAND_MAX)); // 0.0 <= r < 1.0 
            if (r == 0.0) r = 0.000001; // Prevent log(0)
            return (int)(-log(r) * level_mult);
        }

        // level 0 -> 0.1<r<1
        // level 1 -> 0.01<r<0.1
        // level 2 -> 0.001<r<0.01

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
        // Simple HNSW SEARCH: O(log N)
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


        // Connect only 1 neighbour per layer to the new inserted node(only in the layers it exist in)
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

            // Phase 2: Once I reach the target level, connect and drop down to Layer 0
            for (int l = min(max_level, level); l >= 0; --l) {
                // Find nearest neighbor at this specific level
                curr_node = greedy_search_level(vec, curr_node, l);

                // Connect 1 node per layer
                nodes[new_idx].neighbors[l].push_back(curr_node);
                nodes[curr_node].neighbors[l].push_back(new_idx);
            }

            // If this node rolled higher than anything else, it is the new King node
            if (level > max_level) {
                max_level = level;
                enter_point = new_idx;
            }
        }
    };