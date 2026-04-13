#ifndef INCOMPATIBILITY_SEARCH_H_
#define INCOMPATIBILITY_SEARCH_H_

#include "local_search.h"

class IncompatibilityEliminationLocalSearch final : public LocalSearch {
 public:
  /**
   * Targets high-conflict stores in expensive facilities and relocates them.
   * This tends to reduce transport cost while freeing constrained neighborhoods.
   * @param instance Problem instance.
   * @param solution Solution to improve.
   * @return True if at least one improving relocation was applied.
   */
  bool improve(const Instance& instance, Solution& solution) const override;
  /** @return "IncompatibilityEliminationLocalSearch". */
  std::string getName() const override { return "IncompatibilityEliminationLocalSearch"; }
};

#endif  // INCOMPATIBILITY_SEARCH_H_