# Ajuste de hiperparámetros en wlp02

Se intentó ejecutar el ajuste con las tres combinaciones pedidas:

- `alpha = 0.1`, `epsilon = 0.1`
- `alpha = 0.2`, `epsilon = 0.2`
- `alpha = 0.3`, `epsilon = 0.1`

## Estado

En este entorno, la primera corrida de `wlp02` no llegó a terminar dentro del tiempo de ejecución disponible para la herramienta, así que no hay una tabla fiable de resultados todavía.

## Configuración fijada

La mejor configuración que sí quedó validada y que se mantiene puesta en el código es:

- `alpha = 0.2`
- `epsilon = 0.2`

Esta configuración viene de `wlp01` y sigue siendo la referencia activa mientras `wlp02` no pueda completarse en el entorno actual.
