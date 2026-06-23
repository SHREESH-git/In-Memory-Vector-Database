import requests
import random
import time

# 1. Generate a new random 128-D vector to insert
new_vector = [random.random() for _ in range(128)]

print("Sending INSERT request to C++ Vector Database.")
start_time = time.perf_counter()

# 2. Send the POST request to the /insert endpoint
response = requests.post(
    "http://127.0.0.1:8080/insert", 
    json={"vector": new_vector}
)

end_time = time.perf_counter()

# 3. Print the results!
print(f"Network Round-Trip Time: {(end_time - start_time) * 1000:.2f} ms")
print(f"Server Response: {response.elapsed.total_seconds()*1000:.3f} ms")
print(response.json())