/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 3rd of april, 2026
 * @brief Instance data and reader for .dzn files
 */

#ifndef INSTANCE_H
#define INSTANCE_H

#include <string>
#include <utility>
#include <vector>

class Instance {
 public:
  /**
   * Loads an instance from a MiniZinc .dzn file.
   * @param file_name Path to the instance file.
   * @return True on success.
   */
  bool loadFromFile(const std::string& file_name);

  /** @return Number of warehouses in the instance. */
  int getWarehouses() const { return warehouses_; }
  /** @return Number of stores in the instance. */
  int getStores() const { return stores_; }
  /** @return Facility capacities. */
  const std::vector<int>& getCapacity() const { return capacity_; }
  /** @return Facility opening costs. */
  const std::vector<int>& getFixedCost() const { return fixed_cost_; }
  /** @return Store demands. */
  const std::vector<int>& getGoods() const { return goods_; }
  /** @return Transport cost matrix indexed by facility and store. */
  const std::vector<std::vector<int>>& getSupplyCost() const { return supply_cost_; }
  /** @return Number of incompatibility pairs in the instance. */
  int getIncompatibilities() const { return incompatibilities_; }
  /** @return Pairs of incompatible stores. */
  const std::vector<std::pair<int, int>>& getIncompatiblePairs() const {
    return incompatible_pairs_;
  }

 private:
  int warehouses_ = 0;
  int stores_ = 0;
  std::vector<int> capacity_;
  std::vector<int> fixed_cost_;
  std::vector<int> goods_;
  std::vector<std::vector<int>> supply_cost_;
  int incompatibilities_ = 0;
  std::vector<std::pair<int, int>> incompatible_pairs_;
};

#endif // INSTANCE_H