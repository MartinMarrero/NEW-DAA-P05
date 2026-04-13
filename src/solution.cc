#include "lib/solution.h"

#include <algorithm>
#include <stdexcept>

/**
 * Builds the internal state for a new solution.
 * @param instance Problem instance to bind to the solution.
 */
Solution::Solution(const Instance& instance) : instance_(&instance) {
  const int warehouses = instance_->getWarehouses();
  const int stores = instance_->getStores();

  remaining_demand_ = instance_->getGoods();
  remaining_capacity_ = instance_->getCapacity();
  facility_open_.assign(warehouses, false);
  assignment_quantity_.assign(warehouses, std::vector<int>(stores, 0));
  facility_stores_.assign(warehouses, std::unordered_set<int>{});
  incompatibility_graph_.assign(stores, {});

  for (const auto& pair : instance_->getIncompatiblePairs()) {
    const int first_store = pair.first - 1;
    const int second_store = pair.second - 1;

    if (first_store < 0 || first_store >= stores || second_store < 0 || second_store >= stores) {
      throw std::runtime_error("Invalid incompatible pair index in instance data");
    }

    incompatibility_graph_[first_store].insert(second_store);
    incompatibility_graph_[second_store].insert(first_store);
  }
}

/**
 * Checks whether two stores are incompatible.
 * @param first_store First store index.
 * @param second_store Second store index.
 * @return True if the stores cannot share a facility.
 */
bool Solution::areIncompatible(int first_store, int second_store) const {
  return incompatibility_graph_[first_store].count(second_store) > 0;
}

/**
 * Checks whether a store can be added to a facility.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @return True if the assignment respects incompatibilities.
 */
bool Solution::isCompatibleWithFacility(int store_index, int facility_index) const {
  if (store_index < 0 || store_index >= static_cast<int>(remaining_demand_.size())) {
    return false;
  }

  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    return false;
  }

  if (assignment_quantity_[facility_index][store_index] > 0) {
    return true;
  }

  for (const int assigned_store : facility_stores_[facility_index]) {
    if (areIncompatible(store_index, assigned_store)) {
      return false;
    }
  }

  return true;
}

/**
 * Checks whether a quantity can be assigned.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @param quantity Quantity to assign.
 * @return True if the move is feasible.
 */
bool Solution::canAssignQuantity(int store_index, int facility_index, int quantity) const {
  if (quantity <= 0) {
    return false;
  }

  if (store_index < 0 || store_index >= static_cast<int>(remaining_demand_.size())) {
    return false;
  }

  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    return false;
  }

  if (remaining_demand_[store_index] < quantity) {
    return false;
  }

  if (remaining_capacity_[facility_index] < quantity) {
    return false;
  }

  return isCompatibleWithFacility(store_index, facility_index);
}

/**
 * Checks whether a quantity can be removed.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @param quantity Quantity to remove.
 * @return True if the removal is feasible.
 */
bool Solution::canRemoveQuantity(int store_index, int facility_index, int quantity) const {
  if (quantity <= 0) {
    return false;
  }

  if (store_index < 0 || store_index >= static_cast<int>(remaining_demand_.size())) {
    return false;
  }

  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    return false;
  }

  return assignment_quantity_[facility_index][store_index] >= quantity;
}

/**
 * Checks whether a quantity can be moved between facilities.
 * @param store_index Store index.
 * @param source_facility_index Source facility index.
 * @param destination_facility_index Destination facility index.
 * @param quantity Quantity to move.
 * @return True if the move is feasible.
 */
bool Solution::canMoveQuantity(int store_index,
                               int source_facility_index,
                               int destination_facility_index,
                               int quantity) const {
  if (source_facility_index == destination_facility_index) {
    return false;
  }

  if (!canRemoveQuantity(store_index, source_facility_index, quantity)) {
    return false;
  }

  if (destination_facility_index < 0 ||
      destination_facility_index >= static_cast<int>(facility_open_.size())) {
    return false;
  }

  if (!facility_open_[destination_facility_index]) {
    return false;
  }

  if (remaining_capacity_[destination_facility_index] < quantity) {
    return false;
  }

  return isCompatibleWithFacility(store_index, destination_facility_index);
}

/**
 * Returns the largest feasible assignment quantity for a pair.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @return Maximum feasible quantity.
 */
int Solution::getMaxFeasibleQuantity(int store_index, int facility_index) const {
  if (!isCompatibleWithFacility(store_index, facility_index)) {
    return 0;
  }

  if (store_index < 0 || store_index >= static_cast<int>(remaining_demand_.size())) {
    return 0;
  }

  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    return 0;
  }

  return std::min(remaining_demand_[store_index], remaining_capacity_[facility_index]);
}

/**
 * Opens a facility if it is not already open.
 * @param facility_index Facility index.
 */
void Solution::openFacility(int facility_index) {
  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    throw std::runtime_error("Invalid facility index when opening facility");
  }

  if (!facility_open_[facility_index]) {
    fixed_cost_ += instance_->getFixedCost()[facility_index];
    facility_open_[facility_index] = true;
  }

  total_cost_ = fixed_cost_ + transport_cost_;
}

/**
 * Closes an empty facility.
 * @param facility_index Facility index.
 */
void Solution::closeFacility(int facility_index) {
  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    throw std::runtime_error("Invalid facility index when closing facility");
  }

  if (!facility_open_[facility_index]) {
    return;
  }

  if (!facility_stores_[facility_index].empty()) {
    throw std::runtime_error("Cannot close a facility that still has assigned stores");
  }

  fixed_cost_ -= instance_->getFixedCost()[facility_index];
  facility_open_[facility_index] = false;
  total_cost_ = fixed_cost_ + transport_cost_;
}

/**
 * Assigns quantity from a facility to a store.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @param quantity Quantity to assign.
 */
void Solution::assignQuantity(int store_index, int facility_index, int quantity) {
  if (!canAssignQuantity(store_index, facility_index, quantity)) {
    throw std::runtime_error("Cannot assign quantity to facility: infeasible move");
  }

  openFacility(facility_index);

  const int shipping_cost = instance_->getSupplyCost()[facility_index][store_index];
  transport_cost_ += shipping_cost * quantity;

  assignment_quantity_[facility_index][store_index] += quantity;
  remaining_demand_[store_index] -= quantity;
  remaining_capacity_[facility_index] -= quantity;
  facility_stores_[facility_index].insert(store_index);

  total_cost_ = fixed_cost_ + transport_cost_;
}

/**
 * Removes quantity assigned from a facility to a store.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @param quantity Quantity to remove.
 */
void Solution::removeQuantity(int store_index, int facility_index, int quantity) {
  if (!canRemoveQuantity(store_index, facility_index, quantity)) {
    throw std::runtime_error("Cannot remove quantity from facility: infeasible move");
  }

  const int shipping_cost = instance_->getSupplyCost()[facility_index][store_index];
  transport_cost_ -= shipping_cost * quantity;

  assignment_quantity_[facility_index][store_index] -= quantity;
  remaining_demand_[store_index] += quantity;
  remaining_capacity_[facility_index] += quantity;

  if (assignment_quantity_[facility_index][store_index] == 0) {
    facility_stores_[facility_index].erase(store_index);
  }

  total_cost_ = fixed_cost_ + transport_cost_;
}

/**
 * Moves quantity from one facility to another.
 * @param store_index Store index.
 * @param source_facility_index Source facility index.
 * @param destination_facility_index Destination facility index.
 * @param quantity Quantity to move.
 */
void Solution::moveQuantity(int store_index,
                            int source_facility_index,
                            int destination_facility_index,
                            int quantity) {
  if (!canMoveQuantity(store_index, source_facility_index, destination_facility_index, quantity)) {
    throw std::runtime_error("Cannot move quantity between facilities: infeasible move");
  }

  removeQuantity(store_index, source_facility_index, quantity);
  assignQuantity(store_index, destination_facility_index, quantity);
}

/**
 * Returns the assigned quantity for a store/facility pair.
 * @param store_index Store index.
 * @param facility_index Facility index.
 * @return Assigned quantity.
 */
int Solution::getAssignedQuantity(int store_index, int facility_index) const {
  if (store_index < 0 || store_index >= static_cast<int>(remaining_demand_.size())) {
    return 0;
  }

  if (facility_index < 0 || facility_index >= static_cast<int>(facility_open_.size())) {
    return 0;
  }

  return assignment_quantity_[facility_index][store_index];
}

/**
 * Checks whether the solution violates incompatibility constraints.
 * @return True if any facility contains an incompatible pair.
 */
bool Solution::isIncompatibilityViolated() const {
  for (int facility = 0; facility < static_cast<int>(facility_stores_.size()); ++facility) {
    const auto& stores_in_facility = facility_stores_[facility];

    // Check every pair of stores in this facility
    for (int first_store : stores_in_facility) {
      for (int second_store : stores_in_facility) {
        if (first_store < second_store && areIncompatible(first_store, second_store)) {
          return true;  // Incompatibility violation found
        }
      }
    }
  }
  return false;  // No incompatibility violations
}
