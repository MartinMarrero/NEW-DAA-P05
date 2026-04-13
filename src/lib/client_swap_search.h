#ifndef CLIENT_SWAP_SEARCH_H_
#define CLIENT_SWAP_SEARCH_H_

#include "local_search.h"

class ClientSwapLocalSearch final : public LocalSearch {
 public:
  /**
   * Swaps equal assigned quantities of two stores between two facilities.
   * The move preserves total served demand and facility loads.
   * @param instance Problem instance.
   * @param solution Solution to improve.
   * @return True if at least one improving swap was applied.
   */
  bool improve(const Instance& instance, Solution& solution) const override;
  /** @return "ClientSwapLocalSearch". */
  std::string getName() const override { return "ClientSwapLocalSearch"; }
};

#endif  // CLIENT_SWAP_SEARCH_H_