#include "local_search.h"

class RelocationLocalSearch final : public LocalSearch {
 public:
  /**
   * Applies relocation moves to improve the solution.
   * @param instance Problem instance.
   * @param solution Solution to improve.
   * @return True if at least one improvement was made.
   */
  bool improve(const Instance& instance, Solution& solution) const override;
  /** @return "RelocationLocalSearch". */
  std::string getName() const override { return "RelocationLocalSearch"; }
};