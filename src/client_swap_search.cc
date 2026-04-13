#include "lib/client_swap_search.h"

#include <algorithm>
#include <vector>

/**
 * Applies improving client-quantity swaps between facilities until no improvement remains.
 * @param instance Problem instance.
 * @param solution Solution to improve.
 * @return True if at least one improving swap was applied.
 */
bool ClientSwapLocalSearch::improve(const Instance& instance, Solution& solution) const {
  bool improved = false;

  while (true) {
    int best_delta = 0;
    int best_store_a = -1;
    int best_store_b = -1;
    int best_facility_a = -1;
    int best_facility_b = -1;
    int best_quantity = 0;

    const std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
    const std::vector<std::vector<int>>& assignments = solution.getAssignmentQuantity();
    const int stores = instance.getStores();

    for (int store_a = 0; store_a < stores; ++store_a) {
      for (int store_b = store_a + 1; store_b < stores; ++store_b) {
        for (const int facility_a : open_facilities) {
          const int quantity_a = assignments[facility_a][store_a];
          if (quantity_a <= 0) {
            continue;
          }

          for (const int facility_b : open_facilities) {
            if (facility_b == facility_a) {
              continue;
            }

            const int quantity_b = assignments[facility_b][store_b];
            if (quantity_b <= 0) {
              continue;
            }

            const int swapped_quantity = std::min(quantity_a, quantity_b);
            if (swapped_quantity <= 0) {
              continue;
            }

            Solution candidate = solution;
            if (!candidate.canRemoveQuantity(store_a, facility_a, swapped_quantity) ||
                !candidate.canRemoveQuantity(store_b, facility_b, swapped_quantity)) {
              continue;
            }

            candidate.removeQuantity(store_a, facility_a, swapped_quantity);
            candidate.removeQuantity(store_b, facility_b, swapped_quantity);

            if (!candidate.canAssignQuantity(store_a, facility_b, swapped_quantity) ||
                !candidate.canAssignQuantity(store_b, facility_a, swapped_quantity)) {
              continue;
            }

            candidate.assignQuantity(store_a, facility_b, swapped_quantity);
            candidate.assignQuantity(store_b, facility_a, swapped_quantity);

            const int delta = candidate.getTotalCost() - solution.getTotalCost();
            if (delta < best_delta) {
              best_delta = delta;
              best_store_a = store_a;
              best_store_b = store_b;
              best_facility_a = facility_a;
              best_facility_b = facility_b;
              best_quantity = swapped_quantity;
            }
          }
        }
      }
    }

    if (best_delta >= 0) {
      break;
    }

    solution.removeQuantity(best_store_a, best_facility_a, best_quantity);
    solution.removeQuantity(best_store_b, best_facility_b, best_quantity);
    solution.assignQuantity(best_store_a, best_facility_b, best_quantity);
    solution.assignQuantity(best_store_b, best_facility_a, best_quantity);
    improved = true;
  }

  return improved;
}