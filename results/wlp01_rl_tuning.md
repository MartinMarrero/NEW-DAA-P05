# Ajuste de hiperparámetros en wlp01

Configuración base usada:
- `slack_k = 5`
- `max_iterations = 10`
- `seed = 5489`

## Resultados

| alpha | epsilon | total_cost | error_pct |
|---:|---:|---:|---:|
| 0.1 | 0.1 | 32637 | 13.65% |
| 0.2 | 0.2 | 32615 | 13.58% |
| 0.3 | 0.1 | 32637 | 13.65% |

## Mejor configuración

La mejor de estas tres combinaciones en `wlp01` es `alpha = 0.2`, `epsilon = 0.2`, porque obtuvo el menor coste total y el menor error relativo frente al óptimo conocido.
