# GVNS Performance Improvements Summary

## Initial State
- **Error on wlp01**: ~13.5%
- **Main issue**: Parameters not affecting results differentiation, perturbation too aggressive/weak
- **Root causes**:
  1. epsilon_decay was making parameter differences invisible
  2. perturbation_strength=1 was insufficient
  3. Imbalanced initialization vs iteration allocation

## Critical Fixes

### 1. **Algorithm fix**: Removed automatic epsilon decay
- **Problem**: epsilon values (0.1, 0.2, 0.3) were decaying to almost identical values
- **Solution**: Keep epsilon constant during solve() - it's a parameter, not decayed
- **Result**: Different epsilon values now produce different exploration strategies

### 2. **Perturbation strength**: Increased default and made variable
- **Old**: Fixed 1 reinsertion per iteration (too weak, solution barely perturbed)
- **New**: 2-3 reinsertions per iteration, variable based on iteration number
- **Result**: Better escape from local optima

### 3. **Initialization strategy**: Balanced for time efficiency
- **Old approach**: 30 GRASP iterations with exhaustive local search → Very slow init, few main iterations
- **New approach**: 4 quick GRASP runs (1 with LS) → Fast init, many main iterations
- **Rationale**: Better to have 500 main loop iterations with good init than 100 iterations with perfect init

### 4. **Increased default iterations**: 100 → 500
- **Reasoning**: With minimal init overhead, allocate time to the main optimization loop
- **Result**: Better convergence with more perturbation-improvement cycles

## Configuration Evolution

### Initial (broken)
```
epsilon=0.1, alpha=0.3 (header)
epsilon=0.2, alpha=0.2 (main.cc) [inconsistent!]
perturbation_strength=1
max_iterations=100
epsilon_decay=0.995 [automatic decay - PROBLEM!]
```

### Intermediate attempts
1. Heavy init (16+ GRASP) → Too slow, diminishing returns
2. Aggressive LS (10 passes) → Slow, didn't improve quality
3. Complex perturbations (facility swap + reinsertions) → Added overhead, minimal gain

### Final (optimized)
```
epsilon=0.2, alpha=0.2 [consistent across codebase]
perturbation_strength=3
max_iterations=500 [default]
Removed epsilon_decay from solve() - epsilon stays constant
Init: 4 quick GRASP runs (minimal overhead)
Post-LS: 2 polishing passes (light)
```

## Performance Results

| Iterations | Cost  | Error % | Time (approx) |
|------------|-------|---------|---------------|
| 50         | 32400 | 12.83%  | ~30s          |
| 100        | 32384 | 12.77%  | ~1m           |
| 200        | 32108 | 11.81%  | ~1.5m         |
| 300        | 32099 | 11.78%  | ~2m           |
| **500**    | **32001** | **11.44%**  | **~3m**       |

### Improvement Summary
- **From initial**: 13.5% → 11.44% (2.1 percentage points improvement)
- **Relative improvement**: ~16% reduction in error
- **Best viable configuration**: 500 iterations (good quality-time tradeoff)

## Hyperparameter Tuning Results (wlp01, 5 iterations each)

Early testing showed:
- **α=0.2, ε=0.2**: 32651 cost (13.70% error) ← Best balanced
- **α=0.1, ε=0.1**: 32637 cost (13.65% error)
- **α=0.3, ε=0.3**: 32731 cost (13.98% error)

Note: These were with old config; new config shows better overall results.

## Algorithm Components Working Well

1. **RL Q-learning**: ε-greedy selection with Q ← Q+α(r-Q) is effective
2. **Reward function**: Proportional reward r = (cost_before - cost_after) / cost_before works well
3. **VND structure**: 4 neighborhoods (Relocation, ClientSwap, IncompatibilityElimination, FacilitySwap) provides good exploration
4. **Multi-start**: Even 4 quick GRASP runs improves initial solution significantly

## Remaining Gap to <5% Error

Current best: **11.44%** (goal: <5%)

To reach <5%, would likely require:
1. **Much longer execution time** (10-20 min vs current 3 min)
2. **Alternative metaheuristics** (e.g., Tabu Search, Simulated Annealing)
3. **More sophisticated perturbation** (e.g., double bridge moves)
4. **Advanced LS techniques** (e.g., don't-look bits, move caching)

The current approach represents a good balance between solution quality and computational time.

## Usage

Default (500 iterations, best quality-time tradeoff):
```bash
./ms-cflp-ci instances/wlp01.dzn gvns
```

Custom iterations and hyperparameters:
```bash
./ms-cflp-ci instances/wlp01.dzn gvns [slack_k] [max_iterations] [seed] [epsilon] [alpha]
./ms-cflp-ci instances/wlp01.dzn gvns 5 1000 5489 0.2 0.2  # 1000 iter for better quality
./ms-cflp-ci instances/wlp01.dzn gvns 5 50 5489 0.2 0.2    # 50 iter for quick results
```

## Files Modified

- `src/lib/gvns.h`: Changed defaults (max_iterations 100→500, perturbation_strength 1→3, epsilon default 0.1→0.2, alpha 0.3→0.2)
- `src/gvns.cc`: 
  - Removed automatic epsilon decay from main loop
  - Simplified initialization strategy
  - Added variable perturbation strength
  - Light post-LS polish instead of exhaustive
- `src/main.cc`: Already had proper defaults (epsilon=0.2, alpha=0.2)
