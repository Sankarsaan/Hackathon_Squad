# Ultimate Hackathon Team Selector: Exact & Heuristic MWIS Solver

## 📌 Problem Statement
You are tasked with assembling the ultimate hackathon dream team from a pool of $N$ (up to $200,000$) freshman coders. Each coder has a specific **Skill Rating** (Weight), but certain pairs refuse to work together (Conflict Edges). 

This is a classic **Maximum Weight Independent Set (MWIS)** problem on a large, sparse graph. Because MWIS is fundamentally **NP-hard**, solving it via brute force on $N=200,000$ requires roughly $O(2^{200000})$ time, which would take longer than the age of the universe. 

This solver circumvents mathematical impossibility by implementing a highly optimized **Branch-and-Reduce** framework combined with **Hybrid Iterated Local Search (HILS)**, bounded by a strict 5-minute execution limit.

---

##  Algorithm Architecture

The solver operates in four distinct phases to shrink, shatter, and dynamically solve the graph.

### Phase 1: Advanced Kernelization (Graph Shrinking)
Before any heavy computation begins, we prune the graph (reduce it to a "kernel") using mathematically proven operations. By aggressively deleting suboptimal vertices, we exponentially reduce the search space.

* **Neighborhood Removal:** If a coder's skill rating is greater than or equal to the *sum* of all their active rivals' ratings, we permanently add this coder to the team and delete all their rivals.
  * *Proof:* Let $v$ be the coder. If $w(v) \ge w(N(v))$, swapping out all neighbors in any optimal set $\mathcal{I}$ for $v$ yields a new independent set $\mathcal{I}'$ where $w(\mathcal{I}') \ge w(\mathcal{I})$. There is always an optimal solution that includes $v$.
* **Weighted Domination:** If coder `U` has all the same conflicts as coder `V` (and maybe more), but coder `U` has a lower or equal skill rating, we permanently delete coder `U` from the pool.
  * *Proof:* Because $v$ has fewer or equal constraints compared to $u$, any configuration that safely includes $u$ can substitute $u$ with $v$. Since $w(v) \ge w(u)$, this swap yields equal or greater weight. Thus, $u$ can be safely deleted.

### Phase 2: Component Discovery (Graph Shattering)
* **Strategy:** We run a Breadth-First Search (BFS) to identify completely isolated rival networks (components) within the remaining pool of coders.
* **Proof:** If a graph $G$ consists of disconnected subgraphs $G_1, G_2, \dots, G_k$, the selection of nodes in $G_1$ has zero impact on $G_2$. Therefore, $\alpha_w(G) = \sum \alpha_w(G_i)$. Shattering the graph changes the time complexity from an unsolvable $O(2^N)$ to a highly manageable $O(2^{N_1}) + O(2^{N_2}) + \dots + O(2^{N_k})$.

### Phase 3: Dynamic Component Solving
The algorithm processes each isolated component independently, dynamically choosing the optimal strategy based on the component's size.

#### Sub-Phase 3A: Exact Branch-and-Bound (Micro Components $\le 26$ nodes)
For small components, we compute the 100% mathematically perfect score using a recursive search tree, accelerated by heavy pruning.
* **Upper Bound Pruning (Weighted Clique Cover):** We group the remaining available coders into cliques. Since you can only pick a maximum of **one** node from any clique, the sum of their max weights acts as an absolute mathematical ceiling. If `current_score + upper_bound <= best_found_score`, we prune that branch of the search tree.
* **Smart Branching:** We always branch on the node with the highest dynamic degree, removing the most heavily-conflicted node to shrink the remaining graph as fast as possible.

#### Sub-Phase 3B: Hybrid Iterated Local Search (Macro Components $> 26$ nodes)
Massive components are solved using the **HILS heuristic**, dynamically allocating the remaining time from the global 295-second budget.
* **$(w, 1)$-swaps:** If a non-team coder's skill is higher than the *combined* skill of their team-member rivals, we swap them in and kick the rivals out.
* **$(1, 2)$-swaps:** If adding Coder X requires kicking Coder Y, the algorithm checks if it can *also* add Coder Z. If $w(X) + w(Z) > w(Y)$, it executes a 2-for-1 trade.
* **Plateau Search & Perturbations:** To escape dead ends, the algorithm occasionally executes 0-net-point swaps to slide laterally across "plateaus", or forces a random suboptimal swap to knock the algorithm out of a local maximum.

### Phase 4: Final Output Generation
Once the 295-second execution limit is reached, the algorithm aggregates the selection arrays, performs a final $O(N)$ safety sweep to catch isolated nodes, and prints the result.

---

##  Performance Metrics
* **Time Complexity:** * Kernelization: $\approx O(N + M)$ 
  * Exact Solver: $O(2^K)$ strictly bounded to small $K$.
  * HILS Heuristic: $O(\text{Time Limit})$ execution bounded by `<chrono>`.
* **Space Complexity:** $O(N + M)$ utilizing dynamic 1D standard vectors to safely respect memory limits (No MLE).
* **Data Limits:** Strict utilization of `long long` for weight aggregates prevents 32-bit integer overflows (handles scores reaching into the Trillions).

---

##  How to Run and Test

Due to the massive scale of the constraints ($N = 200,000$, $M$ up to millions), standard terminal outputs will lag or crash if forced to print the entire array at once. Follow these steps to generate test cases, compile, and safely save the output to a file.

### Step 1: Generate a Massive Test Case
Use the provided Python script to generate a valid, highly clustered test graph.
```bash
python generator.py
```

### Step 2: Compile the C++ Solver
Ensure you compile with the `-O3` flag. This applies maximum execution optimizations, which is standard for competitive programming judges and drastically speeds up the exact solver.

```bash
g++ -O3 solution.cpp -o solution


### Step 3: Execute and Redirect Output
To prevent the terminal from lagging while printing up to 100,000 chosen coders, use input/output redirection (`<` and `>`) to read directly from the test file and write the answer to a new text file.

**For Windows (Command Prompt):**
```cmd
.\solution.exe < massive_test.txt > final_answer.txt
```

**For Windows (PowerShell with Execution Timer):**
If you want to measure the exact execution time while safely routing the output, wrap a CMD call inside `Measure-Command`:
```powershell
Measure-Command { cmd /c '.\solution.exe < massive_test.txt > final_answer.txt' }
```

**For Linux / Mac:**
```bash
time ./solution < massive_test.txt > final_answer.txt
```

>  **Note:** The program will intentionally utilize its global runtime budget to maximize the heuristic score on macro components. Do not close the window while the terminal cursor is blinking!

### Step 4: View the Results
Once the script finishes executing, open the newly created `final_answer.txt` file in your directory. The output will be structured as follows:

* **Line 1:** Your ultimate Maximum Skill Score (often reaching into the trillions for massive datasets).
* **Line 2:** A space-separated list of selected, conflict-free coder IDs, sorted in ascending order.
