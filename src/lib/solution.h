/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 1st of april, 2026
 * @brief Header file for the Solution class
 */

#ifndef SOLUTION_H_
#define SOLUTION_H_

#include <unordered_set>
#include <vector>

#include "instance.h"

class Solution {
 public:
  /**
   * Creates an empty solution state for the given instance.
   * @param instance Problem instance to represent.
   */
  explicit Solution(const Instance& instance);

  /**
   * Checks whether a store can still be assigned to a facility.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @return True when the facility is compatible with the store.
   */
  bool isCompatibleWithFacility(int store_index, int facility_index) const;
  /**
   * Checks whether a quantity can be assigned to a facility.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @param quantity Quantity to assign.
   * @return True if the assignment is feasible.
   */
  bool canAssignQuantity(int store_index, int facility_index, int quantity) const;
  /**
   * Checks whether a quantity can be removed from a facility.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @param quantity Quantity to remove.
   * @return True if the removal is feasible.
   */
  bool canRemoveQuantity(int store_index, int facility_index, int quantity) const;
  /**
   * Checks whether a quantity can be moved between two facilities.
   * @param store_index Store index.
   * @param source_facility_index Source facility index.
   * @param destination_facility_index Destination facility index.
   * @param quantity Quantity to move.
   * @return True if the move is feasible.
   */
  bool canMoveQuantity(int store_index,
                       int source_facility_index,
                       int destination_facility_index,
                       int quantity) const;
  /**
   * Returns the maximum feasible quantity for a store/facility pair.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @return Maximum feasible quantity, or zero if infeasible.
   */
  int getMaxFeasibleQuantity(int store_index, int facility_index) const;
  /**
   * Opens a facility and updates the fixed-cost component.
   * @param facility_index Facility index.
   */
  void openFacility(int facility_index);
  /**
   * Closes an empty facility and updates the fixed-cost component.
   * @param facility_index Facility index.
   */
  void closeFacility(int facility_index);
  /**
   * Assigns a quantity from a facility to a store.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @param quantity Quantity to assign.
   */
  void assignQuantity(int store_index, int facility_index, int quantity);
  /**
   * Removes a quantity assigned to a facility.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @param quantity Quantity to remove.
   */
  void removeQuantity(int store_index, int facility_index, int quantity);
  /**
   * Moves quantity from one facility to another.
   * @param store_index Store index.
   * @param source_facility_index Source facility index.
   * @param destination_facility_index Destination facility index.
   * @param quantity Quantity to move.
   */
  void moveQuantity(int store_index,
                    int source_facility_index,
                    int destination_facility_index,
                    int quantity);

  /** @return Fixed-cost component of the solution. */
  int getFixedCost() const { return fixed_cost_; }
  /** @return Transport-cost component of the solution. */
  int getTransportCost() const { return transport_cost_; }
  /** @return Total solution cost. */
  int getTotalCost() const { return total_cost_; }
  /** @return Remaining demand for each store. */
  const std::vector<int>& getRemainingDemand() const { return remaining_demand_; }
  /** @return Remaining capacity for each facility. */
  const std::vector<int>& getRemainingCapacity() const { return remaining_capacity_; }
  /** @return Assignment matrix indexed by facility and store. */
  const std::vector<std::vector<int>>& getAssignmentQuantity() const { return assignment_quantity_; }
  /** @return Open/closed status for each facility. */
  const std::vector<bool>& getOpenFacilities() const { return facility_open_; }
  /**
   * Returns the assigned quantity for a store/facility pair.
   * @param store_index Store index.
   * @param facility_index Facility index.
   * @return Assigned quantity.
   */
  int getAssignedQuantity(int store_index, int facility_index) const;
  /**
   * Checks whether the current solution violates incompatibility constraints.
   * @return True if at least one violation exists.
   */
  bool isIncompatibilityViolated() const;

 private:
  /**
   * Checks whether two stores are incompatible.
   * @param first_store First store index.
   * @param second_store Second store index.
   * @return True if the stores cannot share a facility.
   */
  bool areIncompatible(int first_store, int second_store) const;

  const Instance* instance_ = nullptr;
  std::vector<int> remaining_demand_;
  std::vector<int> remaining_capacity_;
  std::vector<bool> facility_open_;
  std::vector<std::vector<int>> assignment_quantity_;
  std::vector<std::unordered_set<int>> facility_stores_;
  std::vector<std::unordered_set<int>> incompatibility_graph_;
  int fixed_cost_ = 0;
  int transport_cost_ = 0;
  int total_cost_ = 0;
};

#endif