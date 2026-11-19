# Directorio de Datos Experimentales y Scripts

Este directorio contiene las mediciones tabuladas y las rutinas computacionales reproducibles para el informe.

## Archivos:
1. `mediciones_distancia.csv`:
   * Tabla con distancias axiales ($z$ en cm), valores teóricos calculados, valores simulados y valores medidos en banco de pruebas con osciloscopio ($V_{tank}$ en $\text{mV}_{pp}$), además de la bandera de verificación de CRC-8 de la trama recibida en el ESP32.
2. `generar_graficos.py`:
   * Script Python que lee los parámetros o el CSV y genera automáticamente las figuras en formato PDF/PNG para la carpeta `../figuras/`.
3. `crear_placeholders.py`:
   * Script auxiliar para regenerar los diagramas placeholder con leyendas claras.

## Modos de Uso:
Para actualizar los gráficos tras tomar mediciones con osciloscopio:
1. Modifique los valores en `mediciones_distancia.csv` con sus datos reales medidos.
2. Ejecute `python3 generar_graficos.py`.
3. Recompile el documento LaTeX en Neovim o en terminal.
