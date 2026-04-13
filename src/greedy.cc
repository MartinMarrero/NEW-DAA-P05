#include "lib/greedy.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

/**
 * Selects the facilities that will be opened by the greedy constructor.
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
    throw std::runtime_error("Greedy construction failed: insufficient total capacity");
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
 * Builds a greedy solution for the given instance.
 * @param instance Problem instance.
 * @param slack_facilities Number of extra facilities to keep open.
 * @return A feasible greedy solution.
 */
Solution BuildGreedySolution(const Instance& instance, int slack_facilities) {
  Solution solution(instance);
  const std::vector<int> open_facilities = SelectOpenFacilities(instance, slack_facilities);

  for (const int facility : open_facilities) {
    solution.openFacility(facility);
  }

  for (int store = 0; store < instance.getStores(); ++store) {
    std::vector<int> ranked_facilities = open_facilities;
    std::sort(ranked_facilities.begin(), ranked_facilities.end(), [&instance, store](int left, int right) {
      const int left_cost = instance.getSupplyCost()[left][store];
      const int right_cost = instance.getSupplyCost()[right][store];
      if (left_cost != right_cost) {
        return left_cost < right_cost;
      }
      return left < right;
    });

    while (solution.getRemainingDemand()[store] > 0) {
      bool assigned = false;

      for (const int facility : ranked_facilities) {
        // Check incompatibility: the facility must not contain any store incompatible with current store
        // This is done inside getMaxFeasibleQuantity() via isCompatibleWithFacility()
        const int quantity = solution.getMaxFeasibleQuantity(store, facility);
        if (quantity <= 0) {
          // Either insufficient capacity or incompatibility constraint violated
          continue;
        }

        solution.assignQuantity(store, facility, quantity);
        assigned = true;
        break;
      }

      if (!assigned) {
        throw std::runtime_error(
            "Greedy construction failed: no feasible facility left for remaining demand");
      }
    }
  }

  return solution;
}

/**
 * Solves the instance using the greedy strategy.
 * @param instance Problem instance.
 * @return A feasible greedy solution.
 */
Solution GreedySolver::solve(const Instance& instance) {
  return BuildGreedySolution(instance, slack_facilities_);
}
