#include <cstdint>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "lib/console_colors.h"
#include "lib/grasp.h"
#include "lib/greedy.h"
#include "lib/gvns.h"
#include "lib/instance.h"
#include "lib/solver.h"

struct RunSummary {
  std::string algorithm;
  int total_cost = 0;
  int fixed_cost = 0;
  int transport_cost = 0;
  int open_facilities = 0;
  int unserved_demand = 0;
  bool incompatibility_violated = false;
  bool has_optimal_reference = false;
  int optimal_cost = 0;
  double error_percentage = 0.0;
};

struct TableCell {
  std::string text;
  console_colors::Color color = console_colors::Color::kReset;
};

std::string ToLower(std::string text) {
  for (char& character : text) {
    character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
  }
  return text;
}

std::string GetInstanceId(const std::string& instance_path) {
  const std::size_t slash_position = instance_path.find_last_of("/\\");
  const std::string filename =
      (slash_position == std::string::npos) ? instance_path : instance_path.substr(slash_position + 1);
  const std::size_t dot_position = filename.find_last_of('.');
  const std::string stem = (dot_position == std::string::npos) ? filename : filename.substr(0, dot_position);
  return ToLower(stem);
}

const std::unordered_map<std::string, int>& GetOptimalCosts() {
  static const std::unordered_map<std::string, int> kOptimalCosts = {
      {"wlp01", 28716},
      {"wlp02", 52952},
      {"wlp03", 64296},
      {"wlp04", 84633},
      {"wlp05", 103857},
      {"wlp06", 111654},
      {"wlp07", 162277},
      {"wlp08", 187938},
  };
  return kOptimalCosts;
}

std::string FormatPercentage(double value) {
  std::ostringstream output;
  output << std::fixed << std::setprecision(2) << value << "%";
  return output.str();
}

RunSummary SummarizeRun(const std::string& algorithm,
                        const std::string& instance_id,
                        const Solution& solution) {
  RunSummary summary;
  summary.algorithm = algorithm;
  summary.total_cost = solution.getTotalCost();
  summary.fixed_cost = solution.getFixedCost();
  summary.transport_cost = solution.getTransportCost();

  for (bool is_open : solution.getOpenFacilities()) {
    if (is_open) {
      ++summary.open_facilities;
    }
  }

  for (int remaining : solution.getRemainingDemand()) {
    summary.unserved_demand += remaining;
  }

  summary.incompatibility_violated = solution.isIncompatibilityViolated();

  const std::unordered_map<std::string, int>& optimal_costs = GetOptimalCosts();
  const auto optimal_it = optimal_costs.find(instance_id);
  if (optimal_it != optimal_costs.end()) {
    summary.has_optimal_reference = true;
    summary.optimal_cost = optimal_it->second;
    summary.error_percentage =
        100.0 * static_cast<double>(summary.total_cost - summary.optimal_cost) /
        static_cast<double>(summary.optimal_cost);
  }

  return summary;
}

void PrintSingleRun(const RunSummary& summary) {
  std::cout << console_colors::Paint("Algorithm: ", console_colors::Color::kBold)
            << console_colors::Paint(summary.algorithm, console_colors::Color::kCyan) << "\n";
  std::cout << console_colors::Paint("Total cost: ", console_colors::Color::kBold)
            << console_colors::Paint(std::to_string(summary.total_cost), console_colors::Color::kGreen)
            << "\n";
  std::cout << console_colors::Paint("Fixed cost: ", console_colors::Color::kBold)
            << summary.fixed_cost << "\n";
  std::cout << console_colors::Paint("Transport cost: ", console_colors::Color::kBold)
            << summary.transport_cost << "\n";
  std::cout << console_colors::Paint("Open facilities: ", console_colors::Color::kBold)
            << summary.open_facilities << "\n";
  std::cout << console_colors::Paint("Unserved demand: ", console_colors::Color::kBold)
            << console_colors::Paint(std::to_string(summary.unserved_demand), console_colors::Color::kYellow)
            << "\n";
  std::cout << console_colors::Paint("Incompatibility violated: ", console_colors::Color::kBold)
            << (summary.incompatibility_violated
                    ? console_colors::Paint("YES (BUG!)", console_colors::Color::kRed)
                    : console_colors::Paint("NO (valid)", console_colors::Color::kGreen))
            << "\n";
    std::cout << console_colors::Paint("Error vs. optimal: ", console_colors::Color::kBold)
        << (summary.has_optimal_reference
          ? console_colors::Paint(FormatPercentage(summary.error_percentage),
                summary.error_percentage > 0.0
              ? console_colors::Color::kYellow
              : console_colors::Color::kGreen)
          : console_colors::Paint("N/A", console_colors::Color::kYellow))
        << "\n";
}

void PrintComparisonTable(const std::vector<RunSummary>& summaries, int warehouse_count) {
  int best_index = -1;
  bool choose_by_error = true;
  for (std::size_t index = 0; index < summaries.size(); ++index) {
    if (!summaries[index].has_optimal_reference) {
      choose_by_error = false;
      break;
    }
  }

  for (std::size_t index = 0; index < summaries.size(); ++index) {
    if (best_index == -1) {
      best_index = static_cast<int>(index);
      continue;
    }

    const bool better = choose_by_error
                            ? summaries[index].error_percentage < summaries[best_index].error_percentage
                            : summaries[index].total_cost < summaries[best_index].total_cost;
    if (better) {
      best_index = static_cast<int>(index);
    }
  }

  const std::vector<std::string> headers = {
      "Algorithm", "Total cost", "Fixed cost", "Transport cost",
      "Open facilities", "Unserved demand", "Incompatibility", "Optimal", "Error %"};

  std::vector<std::size_t> widths = {
      headers[0].size(), headers[1].size(), headers[2].size(), headers[3].size(),
      headers[4].size(), headers[5].size(), headers[6].size(), headers[7].size(), headers[8].size()};

  for (const RunSummary& summary : summaries) {
    widths[0] = std::max(widths[0], summary.algorithm.size());
    widths[1] = std::max(widths[1], std::to_string(summary.total_cost).size());
    widths[2] = std::max(widths[2], std::to_string(summary.fixed_cost).size());
    widths[3] = std::max(widths[3], std::to_string(summary.transport_cost).size());
    widths[4] = std::max(widths[4],
                         std::to_string(summary.open_facilities).size() + 3 +
                             std::to_string(warehouse_count).size());
    widths[5] = std::max(widths[5], std::to_string(summary.unserved_demand).size());
    widths[6] = std::max(widths[6], std::string("NO (valid)").size());
    widths[7] = std::max(widths[7],
                         summary.has_optimal_reference
                             ? std::to_string(summary.optimal_cost).size()
                             : std::string("N/A").size());
    widths[8] = std::max(widths[8],
               summary.has_optimal_reference
                 ? FormatPercentage(summary.error_percentage).size()
                 : std::string("N/A").size());
  }

  if (best_index >= 0) {
    widths[1] = std::max(widths[1], summaries[best_index].algorithm.size());
  }

  auto print_separator = [&widths]() {
    std::cout << '+';
    for (std::size_t width : widths) {
      std::cout << std::string(width + 2, '-') << '+';
    }
    std::cout << '\n';
  };

  auto print_row = [&widths](const std::vector<TableCell>& cells) {
    std::cout << '|';
    for (std::size_t index = 0; index < cells.size(); ++index) {
      const std::string padded = cells[index].text + std::string(widths[index] - cells[index].text.size(), ' ');
      std::cout << ' ' << console_colors::Paint(padded, cells[index].color) << '|' ;
    }
    std::cout << '\n';
  };

  std::cout << console_colors::Paint("Comparison table\n", console_colors::Color::kBold);
  print_separator();
  print_row({
      {headers[0], console_colors::Color::kCyan},
      {headers[1], console_colors::Color::kCyan},
      {headers[2], console_colors::Color::kCyan},
      {headers[3], console_colors::Color::kCyan},
      {headers[4], console_colors::Color::kCyan},
      {headers[5], console_colors::Color::kCyan},
      {headers[6], console_colors::Color::kCyan},
      {headers[7], console_colors::Color::kCyan},
      {headers[8], console_colors::Color::kCyan},
  });
  print_separator();

  for (const RunSummary& summary : summaries) {
    print_row({
        {summary.algorithm, console_colors::Color::kCyan},
        {std::to_string(summary.total_cost), console_colors::Color::kGreen},
        {std::to_string(summary.fixed_cost), console_colors::Color::kBlue},
        {std::to_string(summary.transport_cost), console_colors::Color::kBlue},
        {std::to_string(summary.open_facilities) + " / " + std::to_string(warehouse_count),
         console_colors::Color::kYellow},
        {std::to_string(summary.unserved_demand), console_colors::Color::kYellow},
        {summary.incompatibility_violated ? "YES (BUG!)" : "NO (valid)",
         summary.incompatibility_violated ? console_colors::Color::kRed
                                          : console_colors::Color::kGreen},
        {summary.has_optimal_reference ? std::to_string(summary.optimal_cost) : "N/A",
         summary.has_optimal_reference ? console_colors::Color::kGreen : console_colors::Color::kYellow},
        {summary.has_optimal_reference ? FormatPercentage(summary.error_percentage) : "N/A",
         !summary.has_optimal_reference
             ? console_colors::Color::kYellow
             : (summary.error_percentage > 0.0 ? console_colors::Color::kYellow
                                               : console_colors::Color::kGreen)},
    });
  }

  if (best_index >= 0) {
    const RunSummary& best = summaries[best_index];
    print_separator();
    print_row({
        {"Best", console_colors::Color::kBold},
        {best.algorithm, console_colors::Color::kGreen},
        {"-", console_colors::Color::kBlue},
        {"-", console_colors::Color::kBlue},
        {"-", console_colors::Color::kYellow},
        {"-", console_colors::Color::kYellow},
        {"-", console_colors::Color::kGreen},
        {best.has_optimal_reference ? std::to_string(best.optimal_cost) : "N/A",
         best.has_optimal_reference ? console_colors::Color::kGreen : console_colors::Color::kYellow},
        {best.has_optimal_reference ? FormatPercentage(best.error_percentage) : "N/A",
         best.has_optimal_reference ? console_colors::Color::kGreen : console_colors::Color::kYellow},
    });
  }

  print_separator();
}

RunSummary RunGreedy(const Instance& instance, const std::string& instance_id, int slack_k) {
  GreedySolver solver(slack_k);
  Solution solution = solver.solve(instance);
  return SummarizeRun(solver.getName(), instance_id, solution);
}

RunSummary RunGrasp(const Instance& instance,
                    const std::string& instance_id,
                    int slack_k,
                    int rcl_size,
                    std::uint32_t seed,
                    int iterations) {
  GraspSolver solver(slack_k, rcl_size, seed, iterations);
  Solution solution = solver.solve(instance);
  return SummarizeRun(solver.getName(), instance_id, solution);
}

RunSummary RunGvns(const Instance& instance,
                   const std::string& instance_id,
                   int slack_k,
                   int max_iterations,
                   std::uint32_t seed,
                   double epsilon = 0.2,
                   double alpha = 0.2) {
  GVNS solver(max_iterations, slack_k, seed, epsilon, alpha);
  Solution solution = solver.solve(instance);
  return SummarizeRun(solver.getName(), instance_id, solution);
}

/**
 * Program entry point.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @return Zero on success, nonzero on error.
 */
int main(int argc, char* argv[]) {
  if (argc < 3) {
    std::cerr << console_colors::Paint("Usage:\n", console_colors::Color::kYellow);
    std::cerr << "  " << argv[0]
              << " <instance.dzn> "
              << console_colors::Paint("greedy", console_colors::Color::kCyan)
              << " [slack_k]\n";
    std::cerr << "  " << argv[0]
              << " <instance.dzn> "
              << console_colors::Paint("grasp", console_colors::Color::kCyan)
              << " [slack_k] [rcl_size] [seed] [iterations]\n";
    std::cerr << "  " << argv[0]
              << " <instance.dzn> "
              << console_colors::Paint("gvns", console_colors::Color::kCyan)
              << " [slack_k] [max_iterations] [seed]\n";
    std::cerr << "  " << argv[0]
          << " <instance.dzn> "
          << console_colors::Paint("all", console_colors::Color::kCyan)
          << " [slack_k] [rcl_size] [seed] [iterations]\n";
    return 1;
  }

  try {
    const std::string instance_path = argv[1];
    const std::string algorithm = argv[2];
    const std::string instance_id = GetInstanceId(instance_path);

    Instance instance;
    instance.loadFromFile(instance_path);

    RunSummary greedy_summary;
    RunSummary grasp_summary;

    if (algorithm == "greedy") {
      int slack_k = 5;
      if (argc >= 4) {
        slack_k = std::stoi(argv[3]);
      }
        greedy_summary = RunGreedy(instance, instance_id, slack_k);
    } else if (algorithm == "gvns") {
      int slack_k = 5;
      if (argc >= 4) {
        slack_k = std::stoi(argv[3]);
      }

      int max_iterations = 100;
      if (argc >= 5) {
        max_iterations = std::stoi(argv[4]);
      }

      std::uint32_t seed = 5489u;
      if (argc >= 6) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[5]));
      }

      double epsilon = 0.2;
      if (argc >= 7) {
        epsilon = std::stod(argv[6]);
      }

      double alpha = 0.2;
      if (argc >= 8) {
        alpha = std::stod(argv[7]);
      }

      RunSummary gvns_summary = RunGvns(instance, instance_id, slack_k, max_iterations, seed, epsilon, alpha);
      PrintSingleRun(gvns_summary);
      return 0;
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

      int iterations = 1;
      if (argc >= 7) {
        iterations = std::stoi(argv[6]);
      }

        grasp_summary = RunGrasp(instance, instance_id, slack_k, rcl_size, seed, iterations);
    } else if (algorithm == "all") {
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

      int iterations = 1;
      if (argc >= 7) {
        iterations = std::stoi(argv[6]);
      }
      // Use a stronger GVNS budget in comparison mode so the table reflects
      // the actual optimized variant rather than a one-iteration placeholder.
      const int gvns_iterations = 500;
      RunSummary gvns_summary = RunGvns(instance, instance_id, slack_k, gvns_iterations, seed);

        greedy_summary = RunGreedy(instance, instance_id, slack_k);
        grasp_summary = RunGrasp(instance, instance_id, slack_k, rcl_size, seed, iterations);

      PrintComparisonTable({greedy_summary, grasp_summary, gvns_summary}, instance.getWarehouses());
      return 0;
    } else {
      throw std::runtime_error("Unknown algorithm. Use 'greedy', 'grasp', 'gvns', or 'all'");
    }

    if (algorithm == "greedy") {
      PrintSingleRun(greedy_summary);
    } else {
      PrintSingleRun(grasp_summary);
    }
  } catch (const std::exception& exception) {
    std::cerr << console_colors::Paint("Error: ", console_colors::Color::kBold)
              << console_colors::Paint(exception.what(), console_colors::Color::kRed) << "\n";
    return 1;
  }

  return 0;
}
