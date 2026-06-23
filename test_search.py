import requests
import random
import time

# 1. Generate a new random 128-D vector to insert
query_vector = [random.random() for _ in range(128)]

start_time = time.perf_counter()

print("Sending Search request to C++ Vector Database.")

# 2. Send the POST request to the /insert endpoint
response = requests.post(
    "http://127.0.0.1:8080/search",
    json={"query": query_vector}
)

end_time = time.perf_counter()

print(f"Network Round-Trip Time: {(end_time-start_time)*1000:.3f} ms")
print(f"Requests measured: {response.elapsed.total_seconds()*1000:.3f} ms")
print(response.json())