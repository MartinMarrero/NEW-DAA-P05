/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 1st of april, 2026
 * @brief Header file for the facility class
 */

#ifndef FACILITY_H
#define FACILITY_H

class Facility {
  public:
    int getOpeningCost() { return opening_cost_; }
    int getMaxCapacity() { return max_capacity_; }
  private:
    int opening_cost_; // (F)
    int max_capacity_; // (S)
};



#endif // FACILITY_H