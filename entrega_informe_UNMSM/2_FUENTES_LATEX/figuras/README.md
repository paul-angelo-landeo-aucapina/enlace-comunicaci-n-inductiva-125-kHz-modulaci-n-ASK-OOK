# Directorio de Figuras e Imágenes

Este directorio contiene los archivos gráficos referenciados por el informe IEEE.

## Nomenclatura y Archivos:

1. **Figuras Generadas Automáticamente (Teóricas y Simulación):**
   * `campo_b_distancia.pdf` / `.png`: Distribución axial de campo magnético $B_z(z)$ y caída dipolar $1/z^3$.
   * `acoplamiento_k_distancia.pdf` / `.png`: Inductancia mutua $M(z)$ y factor de acoplamiento $k(z)$.
   * `respuesta_frecuencia_tanques.pdf` / `.png`: Barrido en frecuencia mostrando la sintonía a $125.04\text{ kHz}$.

2. **Placeholders para Fotos y Mediciones Experimentales:**
   * `placeholder_montaje.png`: Foto del montaje del banco de pruebas (bobinas enfrentadas con regla/calibre, placas TX y RX, ESP32). Reemplazar por su foto real (p. ej. `foto_montaje.png` o sobreescribir este archivo).
   * `placeholder_osc_tx.png`: Oscilograma de transmisión a 125 kHz con modulación OOK (Canal 1: portadora modulada en la bobina, Canal 2: pin GPIO18 del ESP32).
   * `placeholder_osc_rx_amp.png`: Oscilograma de recepción (Canal 1: señal en mV inducida en el tanque RX, Canal 2: señal amplificada ~32x a la salida del MCP6002).
   * `placeholder_osc_demod.png`: Oscilograma de demodulación y conformación (Canal 1: envolvente filtrada tras el diodo BAT85, Canal 2: señal digital cuadrada recuperada en la salida del LM393 hacia el ESP32).

## Recomendaciones para Publicación IEEE:
* **Formato vectorial:** Para gráficos y diagramas, priorice archivos `.pdf` o `.eps`.
* **Fotografías y oscilogramas:** Utilice `.png` con resolución mínima de 300 DPI y buen contraste de grilla.
* **Ancho de columna:** En formato IEEE a dos columnas, las figuras estándar deben tener un ancho de `\columnwidth` (aprox. 8.8 cm / 3.5 pulgadas), y figuras anchas `\textwidth` (18 cm / 7.1 pulgadas).
