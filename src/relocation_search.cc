#include "lib/relocation_search.h"

#include <algorithm>
#include <vector>

/**
 * Applies improving relocation moves until no improvement remains.
 * @param instance Problem instance.
 * @param solution Solution to improve.
 * @return True if at least one improving move was applied.
 */
bool RelocationLocalSearch::improve(const Instance& instance, Solution& solution) const {
  bool improved = false;

  while (true) {
    int best_delta = 0;
    int best_store = -1;
    int best_source = -1;
    int best_destination = -1;
    int best_quantity = 0;

    const std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
    const std::vector<std::vector<int>>& assignments = solution.getAssignmentQuantity();
    const std::vector<int>& remaining_capacity = solution.getRemainingCapacity();

    for (const int source : open_facilities) {
      for (int store = 0; store < static_cast<int>(assignments[source].size()); ++store) {
        const int assigned_quantity = assignments[source][store];
        if (assigned_quantity <= 0) {
          continue;
        }

        for (const int destination : open_facilities) {
          if (destination == source) {
            continue;
          }

          if (!solution.isCompatibleWithFacility(store, destination)) {
            continue;
          }

          const int transferable = std::min(assigned_quantity, remaining_capacity[destination]);
          if (transferable <= 0) {
            continue;
          }

          const int source_cost = instance.getSupplyCost()[source][store];
          const int destination_cost = instance.getSupplyCost()[destination][store];
          const int delta = (destination_cost - source_cost) * transferable;
          if (delta < best_delta) {
            best_delta = delta;
            best_store = store;
            best_source = source;
            best_destination = destination;
            best_quantity = transferable;
          }
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