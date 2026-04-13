#ifndef GREEDY_H_
#define GREEDY_H_

#include "instance.h"
#include "solution.h"
#include "solver.h"

/**
 * Greedy solver for the MS-CFLP-CI problem.
 * Implements Algorithm 1 from the assignment PDF:
 * Phase 1: Select facilities based on ascending fixed cost.
 * Phase 2: Assign clients to facilities greedily by minimum cost.
 */
class GreedySolver : public Solver {
 public:
  /**
   * Constructs a GreedySolver with specified parameters.
   * @param slack_facilities Number of extra facilities to open for slack.
   */
  explicit GreedySolver(int slack_facilities = 5)
      : slack_facilities_(slack_facilities) {}

  /**
   * Solves the instance using the greedy algorithm.
   * @param instance The problem instance to solve.
   * @return A feasible Solution.
   */
  Solution solve(const Instance& instance) override;

  /**
   * Returns the name of this solver.
   * @return "Greedy".
   */
  std::string getName() const override { return "Greedy"; }

 private:
  int slack_facilities_;
};

/**
 * Legacy function interface for backward compatibility.
 * Wraps the GreedySolver class.
 * @param instance Problem instance to solve.
 * @param slack_facilities Number of extra facilities to open for slack.
 * @return A feasible greedy solution.
 */
Solution BuildGreedySolution(const Instance& instance, int slack_facilities = 5);

#endif  // GREEDY_H_
