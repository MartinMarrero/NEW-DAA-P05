#ifndef LOCAL_SEARCH_H_
#define LOCAL_SEARCH_H_

#include <memory>
#include <string>
#include <vector>

#include "instance.h"
#include "solution.h"

class LocalSearch {
 public:
  /** Destroys the local-search strategy. */
  virtual ~LocalSearch() = default;

  /**
   * Collects the indices of currently open facilities.
   * @param solution Current solution.
   * @return Open facility indices.
   */
  static std::vector<int> GetOpenFacilities(const Solution& solution);

  /**
   * Collects the indices of currently closed facilities.
   * @param solution Current solution.
   * @return Closed facility indices.
   */
  static std::vector<int> GetClosedFacilities(const Solution& solution);

  /**
   * Tries to improve a solution in place.
   * @param instance Problem instance.
   * @param solution Solution to improve.
   * @return True if the search found an improving move.
   */
  virtual bool improve(const Instance& instance, Solution& solution) const = 0;
  /** @return Human-readable search name. */
  virtual std::string getName() const = 0;
};

#endif  // LOCAL_SEARCH_H_