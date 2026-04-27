/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería Informática
 * Grado en Ingeniería Informática
 * Diseño de Análisis y Algoritmos
 * Práctica 5
 * @brief Fichero de cabecera para la clase GVNS
 * @since 19th of april 2026
 */

#ifndef GVNS_H
#define GVNS_H

#include "solver.h"

#include <cstdint>
#include <memory>
#include <random>
#include <string>
#include <vector>

#include "local_search.h"

class GVNS : public Solver {
 public:
  explicit GVNS(int max_iterations = 1000,
                int slack_facilities = 5,
                std::uint32_t seed = 5489u,
                double epsilon = 0.2,
                double alpha = 0.2,
                int perturbation_strength = 3,
                double epsilon_decay = 0.995,
                double min_epsilon = 0.05);
  ~GVNS() override;

  Solution solve(const Instance& instance) override;
  std::string getName() const override { return "GVNS"; }

 private:
  int max_iterations_;
  int slack_facilities_;
  std::uint32_t seed_;
  double epsilon_;  // exploration probability for epsilon-greedy
  double alpha_;    // learning rate for Q update
  int perturbation_strength_;
  double epsilon_decay_;
  double min_epsilon_;
  // Q vector for neighborhoods (initialized to 0.5)
  std::mt19937 rng_;
};

#endif  // GVNS_H