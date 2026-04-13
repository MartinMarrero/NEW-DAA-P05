#include "lib/swap_search.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace {

/**
 * Orders candidate facilities by shipping cost for a store.
 * @param instance Problem instance.
 * @param store Store index.
 * @param facilities Candidate facility indices.
 * @param excluded_facility Facility to remove from the candidate list.
 * @return Ranked facility indices.
 */
std::vector<int> RankFacilitiesByShippingCost(const Instance& instance,
                                              int store,
                                              const std::vector<int>& facilities,
                                              int excluded_facility) {
  std::vector<int> ranked = facilities;
  ranked.erase(std::remove(ranked.begin(), ranked.end(), excluded_facility), ranked.end());

  std::sort(ranked.begin(), ranked.end(), [&instance, store](int left, int right) {
    const int left_cost = instance.getSupplyCost()[left][store];
    const int right_cost = instance.getSupplyCost()[right][store];
    if (left_cost != right_cost) {
      return left_cost < right_cost;
    }
    return left < right;
  });

  return ranked;
}

/**
 * Tries Swap-Instalaciones(jopen, jclosed): close one open facility and open one closed facility,
 * then reassign all demand of jopen to jclosed or other open facilities.
 * @param instance Problem instance.
 * @param solution Solution to improve.
 * @param open_facility Facility to close.
 * @param closed_facility Facility to open.
 * @return True if the swap is feasible and improves the objective.
 */
bool TrySwapFacilities(const Instance& instance,
                       Solution& solution,
                       int open_facility,
                       int closed_facility) {
  Solution candidate = solution;
  candidate.openFacility(closed_facility);

  const std::vector<std::vector<int>>& assignments = candidate.getAssignmentQuantity();
  const std::vector<int> assigned_quantities = assignments[open_facility];

  for (int store = 0; store < static_cast<int>(assigned_quantities.size()); ++store) {
    int remaining = assigned_quantities[store];
    while (remaining > 0) {
      bool moved = false;
      const std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(candidate);
      const std::vector<int> ranked_destinations =
          RankFacilitiesByShippingCost(instance, store, open_facilities, open_facility);

      for (const int destination : ranked_destinations) {
        const int transferable =
            std::min(remaining, candidate.getMaxFeasibleQuantity(store, destination));
        if (transferable <= 0) {
          continue;
        }

        candidate.moveQuantity(store, open_facility, destination, transferable);
        remaining -= transferable;
        moved = true;
        break;
      }

      if (!moved) {
        return false;
      }
    }
  }

  candidate.closeFacility(open_facility);

  if (candidate.getTotalCost() < solution.getTotalCost()) {
    solution = std::move(candidate);
    return true;
  }

  return false;
}

}  // namespace

/**
 * Applies improving open-closed facility swaps until no further improvement exists.
 * @param instance Problem instance.
 * @param solution Solution to improve.
 * @return True if at least one improving swap was applied.
 */
bool FacilitySwapLocalSearch::improve(const Instance& instance, Solution& solution) const {
  bool improved = false;

  while (true) {
    bool step_improved = false;
    const std::vector<int> open_facilities = LocalSearch::GetOpenFacilities(solution);
    const std::vector<int> closed_facilities = LocalSearch::GetClosedFacilities(solution);

    for (const int open_facility : open_facilities) {
      for (const int closed_facility : closed_facilities) {
        if (TrySwapFacilities(instance, solution, open_facility, closed_facility)) {
          improved = true;
          step_improved = true;
          break;
        }
      }

      if (step_improved) {
        break;
      }
    }

    if (!step_improved) {
      break;
    }
  }

  return improved;
}