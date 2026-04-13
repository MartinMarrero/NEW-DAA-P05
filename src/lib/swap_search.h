#ifndef SWAP_SEARCH_H_
#define SWAP_SEARCH_H_

#include "local_search.h"

class FacilitySwapLocalSearch final : public LocalSearch {
 public:
  /**
   * Swaps an open facility with a closed one by reopening and reassigning demand.
   * @param instance Problem instance.
   * @param solution Solution to improve.
   * @return True if at least one improving swap was applied.
   */
  bool improve(const Instance& instance, Solution& solution) const override;
  /** @return "FacilitySwapLocalSearch". */
  std::string getName() const override { return "FacilitySwapLocalSearch"; }
};

#endif  // SWAP_SEARCH_H_