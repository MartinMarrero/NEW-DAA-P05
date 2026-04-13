#ifndef GRASP_H_
#define GRASP_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <iostream>

#include "client_swap_search.h"
#include "incompatibility_search.h"
#include "instance.h"
#include "local_search.h"
#include "relocation_search.h"
#include "solution.h"
#include "swap_search.h"
#include "solver.h"

/**
 * GRASP constructive solver for the MS-CFLP-CI problem.
 * Implements a randomized greedy construction phase using
 * a Restricted Candidate List (RCL) approach, followed by
 * Random Variable Neighborhood Descent (RVND).
 */
class GraspSolver : public Solver {
 public:
  /**
   * Constructs a GraspSolver with specified parameters.
   * @param slack_facilities Number of extra facilities to open for slack.
   * @param rcl_size Size of the Restricted Candidate List.
   * @param seed Random seed for reproducibility.
   * @param local_searches Optional local-search phases to run after construction.
   */
  GraspSolver(int slack_facilities = 5,
              int rcl_size = 3,
              std::uint32_t seed = 5489u,
              std::vector<std::unique_ptr<LocalSearch>> local_searches = {});

  /** Destroys the solver and releases owned local-search phases. */
  ~GraspSolver() override;

  /**
    * Solves the instance using GRASP construction + RVND improvement.
   * @param instance The problem instance to solve.
   * @return A feasible Solution.
   */
  Solution solve(const Instance& instance) override;

  /**
   * Returns the name of this solver.
   * @return "GRASP".
   */
  std::string getName() const override { return "GRASP"; }

 private:
  int slack_facilities_;
  int rcl_size_;
  std::uint32_t seed_;
  std::vector<std::unique_ptr<LocalSearch>> local_searches_;
};

/**
 * Legacy function interface for backward compatibility.
 * Wraps the GraspSolver class.
 * @param instance Problem instance to solve.
 * @param slack_facilities Number of extra facilities to open for slack.
 * @param rcl_size Size of the Restricted Candidate List.
 * @param seed Random seed for reproducibility.
 * @return A feasible constructive GRASP solution.
 */
Solution BuildGraspConstructiveSolution(const Instance& instance,
                                        int slack_facilities = 5,
                                        int rcl_size = 3,
                                        std::uint32_t seed = 5489u);

#endif  // GRASP_H_
