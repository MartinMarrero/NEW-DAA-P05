#include "lib/instance.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>

namespace {

/**
 * Reads an entire text file into memory.
 * @param file_name Path to the file.
 * @return File contents.
 */
std::string ReadFile(const std::string& file_name) {
  std::ifstream input(file_name);
  if (!input) {
    throw std::runtime_error("Unable to open instance file: " + file_name);
  }

  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

/**
 * Extracts the right-hand side of a named MiniZinc assignment.
 * @param text Source text.
 * @param key Assignment name.
 * @return Raw assigned expression.
 */
std::string ExtractAssignment(const std::string& text, const std::string& key) {
  const std::regex pattern(key + R"(\s*=\s*([^;]+);)");
  std::smatch match;
  if (!std::regex_search(text, match, pattern)) {
    throw std::runtime_error("Missing field: " + key);
  }

  return match[1].str();
}

/**
 * Extracts an integer scalar from a MiniZinc assignment.
 * @param text Source text.
 * @param key Assignment name.
 * @return Parsed integer value.
 */
int ExtractScalar(const std::string& text, const std::string& key) {
  const std::string raw_value = ExtractAssignment(text, key);
  return std::stoi(raw_value);
}

/**
 * Parses a list of integers from raw text.
 * @param raw_value Raw list text.
 * @return Parsed integer values.
 */
std::vector<int> ParseIntegerList(const std::string& raw_value) {
  std::vector<int> values;
  const std::regex number_pattern(R"(-?\d+)");
  auto begin = std::sregex_iterator(raw_value.begin(), raw_value.end(), number_pattern);
  auto end = std::sregex_iterator();

  for (auto it = begin; it != end; ++it) {
    values.push_back(std::stoi(it->str()));
  }

  return values;
}

/**
 * Parses a dense matrix from raw text.
 * @param raw_value Raw matrix text.
 * @param rows Expected number of rows.
 * @param columns Expected number of columns.
 * @return Parsed matrix.
 */
std::vector<std::vector<int>> ParseMatrix(const std::string& raw_value,
                                          int rows,
                                          int columns) {
  const std::vector<int> values = ParseIntegerList(raw_value);
  if (static_cast<int>(values.size()) != rows * columns) {
    throw std::runtime_error("Invalid matrix size in instance file");
  }

  std::vector<std::vector<int>> matrix(rows, std::vector<int>(columns));
  int value_index = 0;
  for (int row = 0; row < rows; ++row) {
    for (int column = 0; column < columns; ++column) {
      matrix[row][column] = values[value_index++];
    }
  }

  return matrix;
}

/**
 * Parses a list of integer pairs from raw text.
 * @param raw_value Raw pair list text.
 * @param expected_pairs Expected number of pairs.
 * @return Parsed pairs.
 */
std::vector<std::pair<int, int>> ParsePairs(const std::string& raw_value,
                                            int expected_pairs) {
  const std::vector<int> values = ParseIntegerList(raw_value);
  if (static_cast<int>(values.size()) != expected_pairs * 2) {
    throw std::runtime_error("Invalid pair list size in instance file");
  }

  std::vector<std::pair<int, int>> pairs;
  pairs.reserve(expected_pairs);
  for (int index = 0; index < static_cast<int>(values.size()); index += 2) {
    pairs.emplace_back(values[index], values[index + 1]);
  }

  return pairs;
}

}  // namespace

bool Instance::loadFromFile(const std::string& file_name) {
  const std::string contents = ReadFile(file_name);

  const int warehouses = ExtractScalar(contents, "Warehouses");
  const int stores = ExtractScalar(contents, "Stores");
  const std::vector<int> capacity = ParseIntegerList(ExtractAssignment(contents, "Capacity"));
  const std::vector<int> fixed_cost = ParseIntegerList(ExtractAssignment(contents, "FixedCost"));
  const std::vector<int> goods = ParseIntegerList(ExtractAssignment(contents, "Goods"));
  const std::vector<std::vector<int>> supply_cost =
      ParseMatrix(ExtractAssignment(contents, "SupplyCost"), warehouses, stores);
  const int incompatibilities = ExtractScalar(contents, "Incompatibilities");
  const std::vector<std::pair<int, int>> incompatible_pairs =
      ParsePairs(ExtractAssignment(contents, "IncompatiblePairs"), incompatibilities);

  if (static_cast<int>(capacity.size()) != warehouses ||
      static_cast<int>(fixed_cost.size()) != warehouses) {
    throw std::runtime_error("Facility data size mismatch in instance file");
  }

  if (static_cast<int>(goods.size()) != stores) {
    throw std::runtime_error("Client demand size mismatch in instance file");
  }

  warehouses_ = warehouses;
  stores_ = stores;
  capacity_ = capacity;
  fixed_cost_ = fixed_cost;
  goods_ = goods;
  supply_cost_ = supply_cost;
  incompatibilities_ = incompatibilities;
  incompatible_pairs_ = incompatible_pairs;

  return true;
}