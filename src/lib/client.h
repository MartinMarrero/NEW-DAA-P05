/**
 * Universidad de La Laguna
 * Escuela Superior de Ingeniería y Tecnología
 * Grado en Ingeniería Informática
 * Diseño y Análisis de Algoritmos
 * @author Martín José Marrero Quintans
 * @since 1st of april, 2026
 * @brief Header file for the Client class
 */

class Client {
 public:
  int getID()     { return id_;     }
  int getDemand() { return demand_; }
 private:
  int id_;
  int demand_;
};