# In-Memory Vector Database

Educational in-memory vector database built from scratch in C++ to apply DSA and DBMS concepts to AI embeddings. Designed to ingest, index, and query high-dimensional arrays in volatile RAM for extreme low-latency retrieval.

## Current Implementation (MVP)
This project is currently in active development. The foundational math engine and baseline metrics are established:
* **RAM Storage:** Native `std::vector` implementation to hold high-dimensional data purely in-memory, eliminating disk I/O latency.
* **Mathematical Engine:** Custom implementations for Cosine Similarity and Euclidean Distance.
* **Baseline k-NN Search:** Currently implements a naive, brute-force linear scan to establish the baseline $O(M \times N)$ time complexity and latency metrics.

## Development Roadmap
Upcoming algorithmic upgrades to reduce search latency:
- [ ] **HNSW Graph Indexing:** Rip out the linear scan and implement a Hierarchical Navigable Small World (HNSW) graph for approximate nearest neighbor (ANN) searches to achieve $O(\log M)$ time complexity.
- [ ] **C++ REST API:** Integrate a lightweight web framework (e.g., `httplib` or `Crow`) to expose `/insert` and `/search` endpoints for network accessibility.
- [ ] **Benchmarking:** Run scale tests comparing brute-force baseline latency vs. HNSW latency across 10,000+ vectors.

## Quick Start
Compile using a standard C++ compiler (e.g., MSYS2 MinGW-w64 on Windows):
`g++ vector_db.cpp -o vectordb`
