#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "lib/grasp.h"
#include "lib/greedy.h"
#include "lib/instance.h"
#include "lib/solver.h"

/**
 * Program entry point.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @return Zero on success, nonzero on error.
 */
int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << "Usage:\n";
    std::cerr << "  " << argv[0]
              << " <instance.dzn> greedy [slack_k]\n";
    std::cerr << "  " << argv[0]
              << " <instance.dzn> grasp [slack_k] [rcl_size] [seed]\n";
    return 1;
  }

  try {
    const std::string instance_path = argv[1];
    const std::string algorithm = argv[2];

    Instance instance;
    instance.loadFromFile(instance_path);

    std::unique_ptr<Solver> solver;

    if (algorithm == "greedy") {
      int slack_k = 5;
      if (argc >= 4) {
        slack_k = std::stoi(argv[3]);
      }
      solver = std::make_unique<GreedySolver>(slack_k);
    } else if (algorithm == "grasp") {
      int slack_k = 5;
      if (argc >= 4) {
        slack_k = std::stoi(argv[3]);
      }

      int rcl_size = 3;
      if (argc >= 5) {
        rcl_size = std::stoi(argv[4]);
      }

      std::uint32_t seed = 5489u;
      if (argc >= 6) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[5]));
      }

      solver = std::make_unique<GraspSolver>(slack_k, rcl_size, seed);
    } else {
      throw std::runtime_error("Unknown algorithm. Use 'greedy' or 'grasp'");
    }

    Solution solution = solver->solve(instance);

    int open_count = 0;
    for (bool is_open : solution.getOpenFacilities()) {
      if (is_open) {
        ++open_count;
      }
    }

    int unserved_demand = 0;
    for (int remaining : solution.getRemainingDemand()) {
      unserved_demand += remaining;
    }

    std::cout << "Algorithm: " << solver->getName() << "\n";
    std::cout << "Total cost: " << solution.getTotalCost() << "\n";
    std::cout << "Fixed cost: " << solution.getFixedCost() << "\n";
    std::cout << "Transport cost: " << solution.getTransportCost() << "\n";
    std::cout << "Open facilities: " << open_count << " / " << instance.getWarehouses() << "\n";
    std::cout << "Unserved demand: " << unserved_demand << "\n";
    std::cout << "Incompatibility violated: " << (solution.isIncompatibilityViolated() ? "YES (BUG!)" : "NO (valid)") << "\n";
  } catch (const std::exception& exception) {
    std::cerr << "Error: " << exception.what() << "\n";
    return 1;
  }

  return 0;
}
