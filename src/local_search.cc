#include "lib/local_search.h"

#include <vector>

/**
 * Collects the indices of currently open facilities.
 * @param solution Current solution.
 * @return Open facility indices.
 */
std::vector<int> LocalSearch::GetOpenFacilities(const Solution& solution) {
  std::vector<int> facilities;
  const std::vector<bool>& open_facilities = solution.getOpenFacilities();
  for (int facility = 0; facility < static_cast<int>(open_facilities.size()); ++facility) {
    if (open_facilities[facility]) {
      facilities.push_back(facility);
    }
  }
  return facilities;
}

/**
 * Collects the indices of currently closed facilities.
 * @param solution Current solution.
 * @return Closed facility indices.
 */
std::vector<int> LocalSearch::GetClosedFacilities(const Solution& solution) {
  std::vector<int> facilities;
  const std::vector<bool>& open_facilities = solution.getOpenFacilities();
  for (int facility = 0; facility < static_cast<int>(open_facilities.size()); ++facility) {
    if (!open_facilities[facility]) {
      facilities.push_back(facility);
    }
  }
  return facilities;
}
