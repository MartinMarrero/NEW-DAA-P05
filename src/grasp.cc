#include "lib/grasp.h"

#include <algorithm>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

/**
 * Selects the facilities that will be opened during GRASP construction.
 * @param instance Problem instance.
 * @param slack_facilities Number of extra facilities to keep open.
 * @return Ordered list of open facilities.
 */
std::vector<int> SelectOpenFacilities(const Instance& instance, int slack_facilities) {
  if (slack_facilities < 0) {
    throw std::runtime_error("Slack facilities parameter cannot be negative");
  }

  std::vector<int> facility_order(instance.getWarehouses());
  std::iota(facility_order.begin(), facility_order.end(), 0);

  const std::vector<int>& fixed_cost = instance.getFixedCost();
  std::sort(facility_order.begin(), facility_order.end(), [&fixed_cost](int left, int right) {
    if (fixed_cost[left] != fixed_cost[right]) {
      return fixed_cost[left] < fixed_cost[right];
    }
    return left < right;
  });

  const int total_demand =
      std::accumulate(instance.getGoods().begin(), instance.getGoods().end(), 0);
  int accumulated_capacity = 0;
  std::vector<int> open_facilities;

  for (const int facility : facility_order) {
    if (accumulated_capacity >= total_demand) {
      break;
    }
    open_facilities.push_back(facility);
    accumulated_capacity += instance.getCapacity()[facility];
  }

  if (accumulated_capacity < total_demand) {
    throw std::runtime_error("GRASP construction failed: insufficient total capacity");
  }

  int extras_added = 0;
  for (const int facility : facility_order) {
    if (extras_added >= slack_facilities) {
      break;
    }

    if (std::find(open_facilities.begin(), open_facilities.end(), facility) == open_facilities.end()) {
      open_facilities.push_back(facility);
      ++extras_added;
    }
  }

  return open_facilities;
}

}  // namespace

/**
 * Creates the default local-search sequence used by GRASP.
 * @return Vector of local-search strategies.
 */
std::vector<std::unique_ptr<LocalSearch>> CreateDefaultLocalSearches() {
  std::vector<std::unique_ptr<LocalSearch>> searches;
  searches.push_back(std::make_unique<RelocationLocalSearch>());
  searches.push_back(std::make_unique<ClientSwapLocalSearch>());
  searches.push_back(std::make_unique<IncompatibilityEliminationLocalSearch>());
  searches.push_back(std::make_unique<FacilitySwapLocalSearch>());
  return searches;
}

/**
 * Applies Random Variable Neighborhood Descent (RVND) over a neighborhood list.
 * At each step, a random active neighborhood is selected. If it improves the
 * solution, the active list is reset; otherwise that neighborhood is removed.
 * @param instance Problem instance.
 * @param neighborhoods Neighborhood structures.
 * @param rng Random generator used for neighborhood selection.
 * @param solution Solution to improve in place.
 */
void ApplyRvnd(const Instance& instance,
               const std::vector<std::unique_ptr<LocalSearch>>& neighborhoods,
               std::mt19937& rng,
               Solution& solution) {
  if (neighborhoods.empty()) {
    return;
  }

  std::vector<int> active_neighborhoods(neighborhoods.size());
  std::iota(active_neighborhoods.begin(), active_neighborhoods.end(), 0);

  while (!active_neighborhoods.empty()) {
    std::uniform_int_distribution<int> pick(
        0, static_cast<int>(active_neighborhoods.size()) - 1);
    const int active_position = pick(rng);
    const int neighborhood_index = active_neighborhoods[active_position];

    if (neighborhoods[neighborhood_index]->improve(instance, solution)) {
      std::iota(active_neighborhoods.begin(), active_neighborhoods.end(), 0);
    } else {
      active_neighborhoods.erase(active_neighborhoods.begin() + active_position);
    }
  }
}

/**
 * Constructs a GRASP solver with optional local-search phases.
 * @param slack_facilities Number of extra facilities to open for slack.
 * @param rcl_size Restricted Candidate List size.
 * @param seed Random seed for the constructive phase.
 * @param local_searches Local-search phases to execute after construction.
 */
GraspSolver::GraspSolver(int slack_facilities,
                         int rcl_size,
                         std::uint32_t seed,
                         int iterations,
                         std::vector<std::unique_ptr<LocalSearch>> local_searches)
    : slack_facilities_(slack_facilities),
      rcl_size_(rcl_size),
      seed_(seed),
      iterations_(iterations),
      local_searches_(local_searches.empty() ? CreateDefaultLocalSearches()
                                             : std::move(local_searches)) {}

/**
 * Destroys the GRASP solver.
 */
GraspSolver::~GraspSolver() = default;

/**
 * Builds the constructive GRASP solution before local search.
 * @param instance Problem instance.
 * @param slack_facilities Number of extra facilities to keep open.
 * @param rcl_size Restricted Candidate List size.
 * @param seed Random seed for the constructive phase.
 * @return A feasible constructive solution.
 */
Solution BuildGraspConstructiveSolution(const Instance& instance,
                                        int slack_facilities,
                                        int rcl_size,
                                        std::uint32_t seed) {
  if (rcl_size <= 0) {
    throw std::runtime_error("RCL size must be strictly positive");
  }

  Solution solution(instance);
  const std::vector<int> open_facilities = SelectOpenFacilities(instance, slack_facilities);

  for (const int facility : open_facilities) {
    solution.openFacility(facility);
  }

  std::mt19937 rng(seed);

  for (int store = 0; store < instance.getStores(); ++store) {
    while (solution.getRemainingDemand()[store] > 0) {
      std::vector<int> candidates;
      candidates.reserve(open_facilities.size());

      for (const int facility : open_facilities) {
        if (solution.getMaxFeasibleQuantity(store, facility) > 0) {
          candidates.push_back(facility);
        }
      }

      if (candidates.empty()) {
        throw std::runtime_error(
            "GRASP construction failed: no feasible facility in candidate list");
      }

      std::sort(candidates.begin(), candidates.end(), [&instance, store](int left, int right) {
        const int left_cost = instance.getSupplyCost()[left][store];
        const int right_cost = instance.getSupplyCost()[right][store];
        if (left_cost != right_cost) {
          return left_cost < right_cost;
        }
        return left < right;
      });

      // RCL: only include facilities that are compatible with current store
      // (incompatibility check is implicitly done in getMaxFeasibleQuantity())
      const int rcl_bound = std::min(static_cast<int>(candidates.size()), rcl_size);
      std::uniform_int_distribution<int> pick(0, rcl_bound - 1);
      const int facility = candidates[pick(rng)];
      const int quantity = solution.getMaxFeasibleQuantity(store, facility);

      if (quantity <= 0) {
        throw std::runtime_error("GRASP construction failed: invalid randomized choice");
      }

      solution.assignQuantity(store, facility, quantity);
    }
  }

  return solution;
}

/**
 * Solves the instance using constructive GRASP followed by local search.
 * @param instance Problem instance.
 * @return A feasible improved solution.
 */
Solution GraspSolver::solve(const Instance& instance) {
  if (iterations_ <= 0) {
    throw std::runtime_error("GRASP iterations must be strictly positive");
  }

  // First iteration initializes the best solution.
  Solution best_solution =
      BuildGraspConstructiveSolution(instance, slack_facilities_, rcl_size_, seed_);
  std::mt19937 best_rng(seed_);
  ApplyRvnd(instance, local_searches_, best_rng, best_solution);

  for (int iteration = 1; iteration < iterations_; ++iteration) {
    const std::uint32_t iteration_seed = seed_ + static_cast<std::uint32_t>(iteration);
    Solution current_solution =
        BuildGraspConstructiveSolution(instance, slack_facilities_, rcl_size_, iteration_seed);
    std::mt19937 current_rng(iteration_seed);
    ApplyRvnd(instance, local_searches_, current_rng, current_solution);

    if (current_solution.getTotalCost() < best_solution.getTotalCost()) {
      best_solution = std::move(current_solution);
    }
  }

  return best_solution;
}
