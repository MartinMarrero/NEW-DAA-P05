/*
 * GVNS implementation with reinsertion perturbation and VND guided by
 * a lightweight reinforcement-learning (epsilon-greedy) policy.
 */

#include "lib/gvns.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <utility>

#include "lib/greedy.h"
#include "lib/grasp.h"

GVNS::GVNS(int max_iterations,
           int slack_facilities,
           std::uint32_t seed,
           double epsilon,
           double alpha,
           int perturbation_strength,
           double epsilon_decay,
           double min_epsilon)
    : max_iterations_(max_iterations),
      slack_facilities_(slack_facilities),
      seed_(seed),
      epsilon_(epsilon),
      alpha_(alpha),
      perturbation_strength_(perturbation_strength),
      epsilon_decay_(epsilon_decay),
      min_epsilon_(min_epsilon),
      rng_(seed_) {}

GVNS::~GVNS() = default;

namespace {

// Build the default neighborhood list used by GVNS (same as GRASP defaults).
std::vector<std::unique_ptr<LocalSearch>> CreateDefaultNeighborhoods() {
  std::vector<std::unique_ptr<LocalSearch>> neighborhoods;
  neighborhoods.push_back(std::make_unique<RelocationLocalSearch>());
  neighborhoods.push_back(std::make_unique<ClientSwapLocalSearch>());
  neighborhoods.push_back(std::make_unique<IncompatibilityEliminationLocalSearch>());
  neighborhoods.push_back(std::make_unique<FacilitySwapLocalSearch>());
  return neighborhoods;
}

// Single reinsertion perturbation: pick a random assigned store from a random
// open facility and move some (or all) of its assigned quantity to another
// facility (open or closed). Returns true if a perturbation was applied.
bool ReinsertionPerturbation(const Instance& instance, std::mt19937& rng, Solution& solution) {
  const int warehouses = instance.getWarehouses();
  const auto& assignment = solution.getAssignmentQuantity();
  std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
  if (open_facilities.empty()) return false;

  // Choose a random source facility that currently serves at least one store.
  std::vector<int> candidate_sources;
  for (int f : open_facilities) {
    for (int s = 0; s < instance.getStores(); ++s) {
      if (assignment[f][s] > 0) {
        candidate_sources.push_back(f);
        break;
      }
    }
  }

  if (candidate_sources.empty()) return false;

  std::uniform_int_distribution<int> pick_src(0, static_cast<int>(candidate_sources.size()) - 1);
  int source = candidate_sources[pick_src(rng)];

  // Collect stores assigned to source
  std::vector<int> stores_assigned;
  for (int s = 0; s < instance.getStores(); ++s) {
    if (assignment[source][s] > 0) stores_assigned.push_back(s);
  }
  if (stores_assigned.empty()) return false;

  std::uniform_int_distribution<int> pick_store(0, static_cast<int>(stores_assigned.size()) - 1);
  int store = stores_assigned[pick_store(rng)];
  int q = solution.getAssignedQuantity(store, source);
  if (q <= 0) return false;

  // Try to find a destination facility with available capacity and compatibility.
  std::vector<int> potential_dest;
  for (int f = 0; f < warehouses; ++f) {
    if (f == source) continue;
    if (solution.isCompatibleWithFacility(store, f) && solution.getRemainingCapacity()[f] > 0) {
      potential_dest.push_back(f);
    }
  }

  if (potential_dest.empty()) {
    // No destination with spare capacity; try opening a closed facility.
    std::vector<int> closed = LocalSearch::GetClosedFacilities(solution);
    if (closed.empty()) return false;
    // choose a random closed facility that is compatible
    std::shuffle(closed.begin(), closed.end(), rng);
    bool applied = false;
    for (int f : closed) {
      if (!solution.isCompatibleWithFacility(store, f)) continue;
      // attempt to move a positive amount (at most q)
      int move_q = std::min(q, instance.getCapacity()[f]);
      if (move_q <= 0) continue;
      // perform safe remove + assign
      try {
        solution.removeQuantity(store, source, move_q);
        solution.assignQuantity(store, f, move_q);
        applied = true;
        break;
      } catch (...) {
        // rollback in case of failure
        try { solution.assignQuantity(store, source, move_q); } catch (...) {}
      }
    }
    return applied;
  }

  // Choose a random destination among potential_dest.
  std::uniform_int_distribution<int> pick_dest(0, static_cast<int>(potential_dest.size()) - 1);
  int dest = potential_dest[pick_dest(rng)];
  int move_q = std::min(q, solution.getRemainingCapacity()[dest]);
  if (move_q <= 0) return false;

  // Execute the move (moveQuantity performs checks internally).
  try {
    solution.moveQuantity(store, source, dest, move_q);
    return true;
  } catch (...) {
    return false;
  }
}

// Facility swap perturbation: close one open facility and open a closed one,
// then reassign demand. Returns true if successful.
bool FacilitySwapPerturbation(const Instance& instance, std::mt19937& rng, Solution& solution) {
  std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
  std::vector<int> closed_facilities = LocalSearch::GetClosedFacilities(solution);
  
  if (open_facilities.empty() || closed_facilities.empty()) return false;
  
  std::uniform_int_distribution<int> pick_open(0, static_cast<int>(open_facilities.size()) - 1);
  std::uniform_int_distribution<int> pick_closed(0, static_cast<int>(closed_facilities.size()) - 1);
  
  int to_close = open_facilities[pick_open(rng)];
  int to_open = closed_facilities[pick_closed(rng)];
  
  Solution candidate = solution;
  try {
    candidate.openFacility(to_open);
  } catch (...) {
    return false;
  }
  
  const auto& assignments = candidate.getAssignmentQuantity();
  int total_remaining = 0;
  for (int store = 0; store < instance.getStores(); ++store) {
    int remaining = assignments[to_close][store];
    total_remaining += remaining;
    while (remaining > 0) {
      bool moved = false;
      for (int f : open_facilities) {
        if (f == to_close) continue;
        if (!candidate.isCompatibleWithFacility(store, f)) continue;
        int cap = candidate.getRemainingCapacity()[f];
        if (cap > 0) {
          int move_qty = std::min(remaining, cap);
          try {
            candidate.moveQuantity(store, to_close, f, move_qty);
            remaining -= move_qty;
            total_remaining -= move_qty;
            moved = true;
            break;
          } catch (...) {}
        }
      }
      if (!moved) break;
    }
  }
  
  if (total_remaining == 0 && candidate.getTotalCost() < solution.getTotalCost()) {
    solution = candidate;
    return true;
  }
  return false;
}


// This is the selected reward design because it rewards larger improvements
// without requiring a separate estimate of a global maximum improvement.
double ComputeProportionalReward(int cost_before, int cost_after) {
  if (cost_before <= 0 || cost_after >= cost_before) {
    return 0.0;
  }

  const double reward = static_cast<double>(cost_before - cost_after) /
                        static_cast<double>(cost_before);
  if (reward < 0.0) {
    return 0.0;
  }
  if (reward > 1.0) {
    return 1.0;
  }
  return reward;
}

// VND guided by a simple RL: epsilon-greedy selection over neighborhoods using
// Q-values and exponential moving average update: Q <- Q + alpha * (r - Q).
void ApplyVndWithRl(const Instance& instance,
                    std::vector<std::unique_ptr<LocalSearch>>& neighborhoods,
                    std::mt19937& rng,
                    Solution& solution,
                    std::vector<double>& q_values,
                    double epsilon,
                    double alpha) {
  const int n = static_cast<int>(neighborhoods.size());
  if (n == 0) return;

  std::vector<int> active(n);
  std::iota(active.begin(), active.end(), 0);

  std::uniform_real_distribution<double> uni01(0.0, 1.0);

  while (!active.empty()) {
    int chosen_pos = 0;
    // Epsilon-greedy selection among active neighborhoods using Q-values
    if (uni01(rng) < epsilon) {
      std::uniform_int_distribution<int> pick(0, static_cast<int>(active.size()) - 1);
      chosen_pos = pick(rng);
    } else {
      // choose argmax Q among active
      double best_q = -1e300;
      std::vector<int> best_positions;
      for (int i = 0; i < static_cast<int>(active.size()); ++i) {
        int idx = active[i];
        if (q_values[idx] > best_q + 1e-12) {
          best_q = q_values[idx];
          best_positions.clear();
          best_positions.push_back(i);
        } else if (std::abs(q_values[idx] - best_q) <= 1e-12) {
          best_positions.push_back(i);
        }
      }
      std::uniform_int_distribution<int> pick(0, static_cast<int>(best_positions.size()) - 1);
      chosen_pos = best_positions[pick(rng)];
    }

    const int neighborhood_index = active[chosen_pos];

    // Measure cost before applying neighborhood to compute proportional reward
    const int cost_before = solution.getTotalCost();
    bool improved = false;
    try {
      improved = neighborhoods[neighborhood_index]->improve(instance, solution);
    } catch (...) {
      improved = false;
    }

    const int cost_after = solution.getTotalCost();
    const double r = improved ? ComputeProportionalReward(cost_before, cost_after) : 0.0;

    // Update Q using exponential recency-weighted average
    double& Q = q_values[neighborhood_index];
    Q = Q + alpha * (r - Q);

    if (improved) {
      // reset active neighborhoods to try all again
      active.clear();
      active.resize(n);
      std::iota(active.begin(), active.end(), 0);
    } else {
      // remove the neighborhood from active list
      active.erase(active.begin() + chosen_pos);
    }
  }
}

}  // namespace

Solution GVNS::solve(const Instance& instance) {
  if (max_iterations_ <= 0) throw std::runtime_error("GVNS: max_iterations must be positive");

  // Fast initialization: GRASP + quick LS
  Solution best = BuildGraspConstructiveSolution(instance, slack_facilities_, 3, seed_);
  std::vector<std::unique_ptr<LocalSearch>> ls_ops = CreateDefaultNeighborhoods();
  
  // One complete pass of local search
  for (const auto& ls : ls_ops) {
    ls->improve(instance, best);
  }
  
  // Just 3 quick GRASP variants (4 total), no LS to save time
  for (int grasp_iter = 1; grasp_iter < 4; ++grasp_iter) {
    std::uint32_t iteration_seed = seed_ + static_cast<std::uint32_t>(grasp_iter);
    Solution candidate = BuildGraspConstructiveSolution(instance, slack_facilities_, 3, iteration_seed);
    if (candidate.getTotalCost() < best.getTotalCost()) {
      best = std::move(candidate);
    }
  }

  // Phase 2: VNS with perturbation and VND+RL
  auto neighborhoods = CreateDefaultNeighborhoods();
  const int n = static_cast<int>(neighborhoods.size());
  std::vector<double> q_values(n, 0.5);  // RL Q-values

  for (int iter = 0; iter < max_iterations_; ++iter) {
    Solution current = best;
    
    // Perturbation: 2-3 reinsertions per iteration
    int perturb_moves = 2 + (iter % 2);
    for (int p = 0; p < perturb_moves; ++p) {
      ReinsertionPerturbation(instance, rng_, current);
    }

    // Improvement: VND with RL guidance
    ApplyVndWithRl(instance, neighborhoods, rng_, current, q_values, epsilon_, alpha_);

    // Light post-local search: max 2 passes
    bool improved = true;
    int passes = 0;
    while (improved && passes < 2) {
      improved = false;
      for (const auto& ls : neighborhoods) {
        if (ls->improve(instance, current)) {
          improved = true;
        }
      }
      ++passes;
    }

    // Accept if better
    if (current.getTotalCost() < best.getTotalCost()) {
      best = std::move(current);
    }
  }

  return best;
}
