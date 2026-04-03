/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 1st of april, 2026
 * @brief Header file for the FacilityManager class
 */

#ifndef FACILITY_MANAGER_H
#define FACILITY_MANAGER_H

#include "facility.h"
#include <vector>

class FacilityManager {
  public:
    std::vector<Facility> getFacilities() { return facilities_; }
  private:
    std::vector<Facility> facilities_; // (M)
};

#endif // FACILITY_MANAGER_H