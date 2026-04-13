#include "lib/incompatibility_search.h"

#include <algorithm>
#include <vector>

namespace {

/**
 * Computes incompatibility degree for each store.
 * @param instance Problem instance.
 * @return Degree per store index.
 */
std::vector<int> ComputeStoreDegrees(const Instance& instance) {
  std::vector<int> degree(instance.getStores(), 0);
  for (const auto& pair : instance.getIncompatiblePairs()) {
    const int first_store = pair.first - 1;
    const int second_store = pair.second - 1;
    if (first_store >= 0 && first_store < instance.getStores()) {
      ++degree[first_store];
    }
    if (second_store >= 0 && second_store < instance.getStores()) {
      ++degree[second_store];
    }
  }
  return degree;
}

}  // namespace

/**
 * Applies improving relocations focused on high-conflict clients in expensive facilities.
 * @param instance Problem instance.
 * @param solution Solution to improve.
 * @return True if at least one improving move was applied.
 */
bool IncompatibilityEliminationLocalSearch::improve(const Instance& instance,
                                                    Solution& solution) const {
  bool improved = false;
  const std::vector<int> conflict_degree = ComputeStoreDegrees(instance);

  while (true) {
    int best_delta = 0;
    int best_store = -1;
    int best_source = -1;
    int best_destination = -1;
    int best_quantity = 0;

    const std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
    const std::vector<std::vector<int>>& assignments = solution.getAssignmentQuantity();
    const std::vector<int>& remaining_capacity = solution.getRemainingCapacity();

    int target_facility = -1;
    int max_facility_transport = -1;
    for (const int facility : open_facilities) {
      int facility_transport = 0;
      for (int store = 0; store < instance.getStores(); ++store) {
        const int quantity = assignments[facility][store];
        if (quantity > 0) {
          facility_transport += quantity * instance.getSupplyCost()[facility][store];
        }
      }

      if (facility_transport > max_facility_transport) {
        max_facility_transport = facility_transport;
        target_facility = facility;
      }
    }

    if (target_facility == -1) {
      break;
    }

    std::vector<int> candidate_stores;
    for (int store = 0; store < instance.getStores(); ++store) {
      if (assignments[target_facility][store] > 0) {
        candidate_stores.push_back(store);
      }
    }

    std::sort(candidate_stores.begin(), candidate_stores.end(), [&conflict_degree](int left, int right) {
      if (conflict_degree[left] != conflict_degree[right]) {
        return conflict_degree[left] > conflict_degree[right];
      }
      return left < right;
    });

    for (const int store : candidate_stores) {
      const int assigned_quantity = assignments[target_facility][store];
      if (assigned_quantity <= 0) {
        continue;
      }

      for (const int destination : open_facilities) {
        if (destination == target_facility) {
          continue;
        }

        if (!solution.isCompatibleWithFacility(store, destination)) {
          continue;
        }

        const int transferable = std::min(assigned_quantity, remaining_capacity[destination]);
        if (transferable <= 0) {
          continue;
        }

        const int delta =
            (instance.getSupplyCost()[destination][store] -
             instance.getSupplyCost()[target_facility][store]) *
            transferable;

        if (delta < best_delta) {
          best_delta = delta;
          best_store = store;
          best_source = target_facility;
          best_destination = destination;
          best_quantity = transferable;
        }
      }
    }

    if (best_delta >= 0) {
      break;
    }

    const int old_total_cost = solution.getTotalCost();
    solution.moveQuantity(best_store, best_source, best_destination, best_quantity);
    if (solution.getTotalCost() >= old_total_cost) {
      break;
    }
    improved = true;
  }

  return improved;
}