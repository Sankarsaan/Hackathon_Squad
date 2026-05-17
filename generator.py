import random
import os

def generate_test_case(N, M, filename="massive_test.txt"):
    print(f"Generating test case with N={N} coders and M={M} conflicts...")
    
    with open(filename, 'w') as f:
        # 1. Write N and M
        f.write(f"{N} {M}\n")
        
        # 2. Generate and write N random skill ratings (1 to 1,000,000,000)
        print("Generating skill ratings...")
        weights = [str(random.randint(1, 1000000000)) for _ in range(N)]
        f.write(" ".join(weights) + "\n")
        
        # 3. Generate M unique conflict pairs
        print("Generating conflict edges (this might take a few seconds)...")
        edges = set()
        
        # To make it realistic and test component splitting, we won't just make it uniform.
        # We'll create some dense local clusters and sparse connections.
        while len(edges) < M:
            u = random.randint(1, N)
            # Create local clustering 80% of the time, global random 20% of the time
            if random.random() < 0.8:
                # Pick a v close to u to form dense sub-components
                v_offset = random.randint(-50, 50)
                v = u + v_offset
                if v < 1 or v > N:
                    v = random.randint(1, N)
            else:
                v = random.randint(1, N)
                
            if u != v:
                # Enforce (u, v) order to avoid A-B and B-A duplicates in the set
                if u > v: 
                    u, v = v, u
                edges.add((u, v))
        
        # 4. Write all edges to the file
        print("Writing data to file...")
        for u, v in edges:
            f.write(f"{u} {v}\n")
            
    print(f"Success! Saved test case to '{filename}'. File size: {os.path.getsize(filename) / (1024*1024):.2f} MB")

# ==========================================
# TEST PARAMETERS
# ==========================================
# Push the algorithm to the absolute maximum constraints
NUM_CODERS = 200000
NUM_CONFLICTS = 600000 

generate_test_case(NUM_CODERS, NUM_CONFLICTS)