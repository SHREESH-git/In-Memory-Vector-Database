# In-Memory Vector Database

## Purpose of the Project
The purpose of this project is to build an in-memory vector database from scratch in C++. By avoiding external libraries for the core algorithms, this project demonstrates a deep understanding of Data Structures and Algorithms (DSA) and Database Management System (DBMS) principles. It is specifically designed to store and search high-dimensional AI embeddings (like those from LLMs) entirely in volatile RAM, eliminating disk I/O bottlenecks.

## What Was Implemented
This folder showcases the evolutionary stages of building a vector search engine. Instead of generating random data on the fly, all algorithms now load from a unified, deterministic dataset (`dataset.txt` and `queries.txt`) to ensure fair, reproducible benchmarking.

1. **Linear Search (`LinearSearch.cpp`)**: A brute-force $k$-Nearest Neighbor (k-NN) baseline using Squared Euclidean distance. It achieves 100% recall but scales poorly as the dataset grows.
2. **Greedy Graph Search (`GreedyGraphSearch.cpp`)**: Introduces a single-layer ANN graph. It massively speeds up queries by traversing node connections greedily, but suffers from an expensive $O(N^2)$ graph construction time.
3. **Simple HNSW (`SimpleHNSW.cpp`)**: Introduces a multi-layer probabilistic graph (Hierarchical Navigable Small World). This significantly drops index construction time to $O(N \log N)$ by zooming down sparse upper layers before doing dense searches.
4. **Optimized HNSW Server (`VectorDBServer.cpp` & `HNSW.cpp`)**: The final optimized algorithm. It utilizes priority queues during insertion (`efConstruction`) and searching (`efSearch`) to maintain multiple candidate paths, improving recall compared to greedy traversal. The server wraps this algorithm in a REST API using `cpp-httplib`, exposing `/insert` and `/search` endpoints.

## Results Comparison
I benchmarked the algorithms using **10,000 base vectors** and **1,000 query vectors** (128 dimensions each). 

| Algorithm | Index Construction Time (ms) | Average Search Latency (ms) | Queries Per Second (QPS) |
| :--- | :--- | :--- | :--- |
| **Linear Search** | 3.01 ms | 0.681 ms | ~1,467 |
| **Greedy Graph Search** | 6,954.98 ms | 0.029 ms | ~33,556 |
| **Simple HNSW** | 1,509.11 ms | 0.037 ms | ~26,555 |
| **Optimized HNSW** | 3,872.34 ms | 0.533 ms | ~1,875 |

> **Note**: While **Simple HNSW** and **Greedy Graph Search** show incredibly low sub-millisecond latencies, their pure greedy approach suffers from **low recall (accuracy)**. The **Optimized HNSW** provides the best balance between search latency and retrieval accuracy. It utilizes priority queues during insertion (`efConstruction`) and searching (`efSearch`) to improve recall by exploring multiple candidate paths instead of following a single greedy route. HNSW really shines on larger datasets (1M+ vectors), where Linear Search latency increases linearly with the number of stored vectors, while HNSW maintains approximately logarithmic search behavior.

### High-Scale Benchmark (100,000 Vectors)
To demonstrate HNSW's true scaling capabilities, I generated an extended dataset of **100,000 base vectors**.

| Algorithm | Index Construction Time | Average Search Latency |
| :--- | :--- | :--- |
| **Linear Search** | 20.95 ms | 6.81 ms |
| **Optimized HNSW** | 138,382 ms (138 s)| 1.2939 ms |

As the dataset grows from 10,000 to 100,000 vectors, Linear Search latency scales linearly (10x slower). However, HNSW scales logarithmically, slashing search times. On datasets exceeding 100,000 vectors, **Optimized HNSW reduces query latency by over 80%** compared to an exhaustive linear scan.
## Time and Space Complexity

Here is the visual representation of the complexity for the Vector Search Architectures:

![Time and Space Complexity](../FinalVectorDB/Time&Space.png)

## REST API

The final HNSW implementation is exposed through HTTP endpoints.

### Insert Vector

POST /insert

Request:

{
    "vector": [0.12, 0.43, ...]
}

### Search Vector

POST /search

Request:

{
    "query": [0.25, 0.61, ...]
}

Response:

{
    "status": "success",
    "nearest_node_index": 421,
    "distance_score": 0.52,
    "latency_ms": 0.04,
    "algorithm": "HNSW efSearch"
}

**To run the benchmarks yourself:**
Use the provided `.bat` files in this directory! For example, run `run_generate_dataset.bat` first, followed by `run_vectordbserver.bat`.
