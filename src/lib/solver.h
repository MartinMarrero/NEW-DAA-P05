/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 3rd of april, 2026
 * @brief Base abstract class for solving the MS-CFLP-CI problem
 */

#ifndef SOLVER_H_
#define SOLVER_H_

#include "instance.h"
#include "solution.h"

/**
 * Abstract base class for all solution algorithms.
 * Uses the Strategy pattern to allow different solvers
 * to be swapped at runtime.
 */
class Solver {
 public:
  virtual ~Solver() = default;

  /**
   * Solves the given instance and returns a feasible solution.
   * @param instance The problem instance to solve.
   * @return A Solution object containing the assignment and cost.
   */
  virtual Solution solve(const Instance& instance) = 0;

  /**
   * Returns the name of the solver algorithm.
   * @return A string identifying this solver (e.g., "Greedy", "GRASP").
   */
  virtual std::string getName() const = 0;
};

#endif  // SOLVER_H_
