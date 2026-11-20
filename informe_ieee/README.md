# Informe Técnico IEEE: Enlace de Comunicación Inductiva a 125 kHz (ASK/OOK)

**Universidad Nacional Mayor de San Marcos (UNMSM)**  
*Facultad de Ingeniería Electrónica y Eléctrica (FIEE)*  
*Escuela Profesional de Ingeniería de Telecomunicaciones*  
*Asignatura:* Teoría de Campos Electromagnéticos  
*Docente Asesor:* Merma Jara, Marco Antonio  

**Autores / Estudiantes:**
* **Landeo Aucapiña, Paul Angelo** (`paul.landeo@unmsm.edu.pe`)
* **Rosales Aranda, Sergio Alessandro** (`sergio.rosales@unmsm.edu.pe`)

---

## 1. Organización del Directorio

Este directorio unifica de forma autónoma y descomprimida la totalidad de los recursos académicos, científicos y de ingeniería del proyecto:

```text
informe_ieee/
├── main.pdf                                  # Documento final compilado (14 páginas, formato IEEEtran)
├── BORRADOR.md                               # Documento consolidado del informe en Markdown
├── main.tex                                  # Archivo maestro de compilación LaTeX
├── IEEEtran.cls                              # Clase formal IEEE Transactions / Conference
├── referencias.bib                           # Base de datos bibliográfica BibTeX
├── Makefile                                  # Automatización de compilación (Tectonic / pdflatex)
├── README.md                                 # Guía unificada de este directorio
├── secciones/                                # Código fuente LaTeX modular
│   ├── 01_introduccion.tex                   # Introducción, estado del arte y objetivos
│   ├── 02_metodologia.tex                    # Modelo físico, esquemas KiCad y análisis paramétrico
│   ├── 03_resultados.tex                     # Cálculos, oscilogramas simulados y evidencia experimental
│   ├── 04_discusion_conclusiones.tex         # Análisis forense de fallas, ruidos y conclusiones
│   ├── 05_anexo_firmware.tex                 # Código fuente comentado de firmwares ESP32
│   └── 06_anexo_simulacion.tex               # Código fuente comentado de scripts en Python
├── figuras/                                  # Banco de recursos gráficos de alta resolución
│   ├── circuito_tx_esquematico.png           # Esquema electrónico KiCad del Transmisor
│   ├── circuito_rx_esquematico.png           # Esquema electrónico KiCad del Receptor
│   ├── sim_montaje_enlace.pdf / .png         # Diagrama en bloques de la arquitectura de simulación
│   ├── sim_osc_tx.pdf / .png                 # Oscilograma simulado TX (Fondo blanco, 125 kHz)
│   ├── sim_osc_rx_amp.pdf / .png             # Oscilograma simulado RX (Fondo blanco, MCP6002)
│   ├── sim_osc_demod.pdf / .png              # Oscilograma demodulación BAT85 y LM393 (Fondo blanco)
│   ├── bobina_electroiman_real.jpg           # Registro fotográfico del electroimán experimental
│   ├── oscilograma_fallo_tx_rx.jpg           # Captura Tektronix evidenciando el fallo experimental
│   ├── campo_b_distancia.pdf / .png          # Distribución de densidad de flujo axial Biot-Savart
│   ├── acoplamiento_k_distancia.pdf / .png   # Inductancia mutua y factor de acoplamiento k(z)
│   └── respuesta_frecuencia_tanques.pdf/.png # Respuesta armónica de tanques LC en SPICE
├── datos/                                    # Scripts reproducibles de simulación numérica
│   ├── generar_oscilogramas_simulados.py     # Genera las figuras 6 a 9 con fondo blanco
│   ├── simular_enlace_datos.py                   # Simulador físico transitorio extremo a extremo
│   ├── calcular_parametros_enlace.py             # Deducción cuantitativa de ganancias y potencias
│   ├── generar_graficos.py                       # Cálculo numérico de curvas electromagnéticas
│   ├── mediciones_distancia.csv                  # Registro de barrido de distancia y verificación CRC-8
│   └── requirements.txt                          # Dependencias de Python (numpy, matplotlib)
└── firmware/                                 # Código fuente para microcontroladores ESP32
    ├── firmware_tx/                              # Firmware transmisor (Manchester 250 bps, CRC-8)
    └── firmware_rx/                              # Firmware receptor (Interrupción GPIO19, anti-glitch)
```

---

## 2. Instrucciones de Compilación y Reproducibilidad

### A. Documento PDF Compilado
El informe técnico completo y listo para revisión se encuentra disponible en:
* [`main.pdf`](main.pdf) (14 páginas, verificado con `qpdf --check`).

### B. Compilación Local de LaTeX
Para recompilar el documento desde la terminal en este directorio:
```bash
make
# O bien utilizando directamente el compilador Tectonic:
tectonic main.tex
```

### C. Uso en Overleaf
Este directorio es compatible de forma directa con Overleaf:
1. Comprimir o cargar los archivos de esta carpeta (`main.tex`, `IEEEtran.cls`, `referencias.bib`, `secciones/` y `figuras/`) en Overleaf.
2. Seleccionar el motor estándar `pdfLaTeX`. El archivo `IEEEtran.cls` está en la raíz, por lo que compila de manera transparente.

### D. Ejecución de Simulaciones en Python (`datos/`)
Para reproducir numéricamente las curvas teóricas y oscilogramas:
```bash
cd datos
pip install -r requirements.txt

# Generar oscilogramas de simulación con fondo blanco (Figuras 6 a 9):
python3 generar_oscilogramas_simulados.py

# Simular transmisión y recepción completa a 7.0 cm:
python3 simular_enlace_datos.py --distancia 7.0 --graficar

# Calcular balance analítico de ganancias y potencias:
python3 calcular_parametros_enlace.py

# Generar curvas de campo magnético y respuesta en frecuencia:
python3 generar_graficos.py
```

### E. Flasheo de Firmwares ESP32 (`firmware/`)
* **Transmisor (`firmware/firmware_tx/`):** Abrir en Arduino IDE / PlatformIO, seleccionar placa *ESP32 Dev Module* y flashear. El pin `GPIO18` emite la señal de control moduladora hacia el driver TC4427A.
* **Receptor (`firmware/firmware_rx/`):** Flashear en el ESP32 receptor. Conectar el pin `GPIO19` a la salida del comparador analógico LM393. La consola serie a `115200 bps` mostrará las tramas decodificadas y validadas mediante CRC-8.
