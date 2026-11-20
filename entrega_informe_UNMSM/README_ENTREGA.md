# Paquete de Entrega Académica: Enlace Inductivo a 125 kHz (ASK/OOK) — Formato IEEE

**Universidad Nacional Mayor de San Marcos (UNMSM)**  
*Facultad de Ingeniería Electrónica y Eléctrica (FIEE)*  
*Escuela Profesional de Ingeniería de Telecomunicaciones*  
*Asignatura:* Teoría de Campos Electromagnéticos  
*Docente Asesor:* Merma Jara, Marco Antonio  

**Estudiantes / Autores:**
* Landeo Aucapiña, Paul Angelo (`paul.landeo@unmsm.edu.pe`)
* Rosales Aranda, Sergio Alessandro (`sergio.rosales@unmsm.edu.pe`)

---

## 1. Contenido del Paquete de Entrega

El presente paquete consolida todos los recursos de investigación, desarrollo analítico, simulaciones numéricas, código fuente de firmware embebido y documentación técnica del proyecto:

```text
entrega_informe_UNMSM/
├── 1_DOCUMENTO_FINAL/
│   ├── informe_ieee_UNMSM_Landeo_Rosales.pdf     # Documento principal en PDF listo para evaluación
│   └── BORRADOR_CONSOLIDADO.md                    # Versión unificada del informe en Markdown
├── 2_FUENTES_LATEX/                              # Paquete autónomo para recompilación en Overleaf o local
│   ├── main.tex                                  # Documento maestro LaTeX
│   ├── referencias.bib                           # Bibliografía formato BibTeX
│   ├── IEEEtran.cls                              # Clase formal IEEE Transactions / Conference
│   ├── Makefile                                  # Automatización de compilación
│   ├── secciones/                                # Módulos de texto (Introducción, Metodología, etc.)
│   └── figuras/                                  # Figuras vectoriales (PDF) y rasterizadas (PNG, JPG)
├── 3_SIMULACIONES_PYTHON/                        # Scripts científicos reproducibles
│   ├── generar_oscilogramas_simulados.py         # Genera oscilogramas simulados (Figuras 6 a 9)
│   ├── simular_enlace_datos.py                   # Simulador físico transitorio extremo a extremo
│   ├── calcular_parametros_enlace.py             # Deducción analítica de ganancias, potencias e ISI
│   ├── generar_graficos.py                       # Curvas de Biot-Savart, acoplamiento y tanques LC
│   ├── mediciones_distancia.csv                  # Datos de barrido de distancia y validación CRC-8
│   └── requirements.txt                          # Dependencias de Python (numpy, matplotlib)
├── 4_FIRMWARE_ESP32/                             # Código fuente para microcontroladores
│   ├── firmware_tx/                              # Transmisor ESP32 (Modulación Manchester, CRC-8)
│   └── firmware_rx/                              # Receptor ESP32 (Interrupciones, filtro anti-glitch)
├── README_ENTREGA.md                             # Esta guía
└── entrega_enlace_inductivo_125kHz_UNMSM.zip     # Archivo comprimido integral
```

---

## 2. Instrucciones de Reproducibilidad y Compilación

### A. Visualización Directa del Informe
El documento final listo para lectura, revisión o impresión se encuentra en:
* [`1_DOCUMENTO_FINAL/informe_ieee_UNMSM_Landeo_Rosales.pdf`](1_DOCUMENTO_FINAL/informe_ieee_UNMSM_Landeo_Rosales.pdf)

### B. Recompilación de Fuentes LaTeX (`2_FUENTES_LATEX/`)

#### Opción 1: En Overleaf
1. Comprimir en un archivo `.zip` todo el contenido de la carpeta `2_FUENTES_LATEX/`.
2. En Overleaf, crear un nuevo proyecto seleccionando **Upload Project**.
3. Compilar con el motor estándar `pdfLaTeX`. El archivo `IEEEtran.cls` y las figuras se encuentran en las rutas relativas correspondientes.

#### Opción 2: En Linux con Make o Tectonic
Desde la terminal en el directorio `2_FUENTES_LATEX/`:
```bash
make
# O bien utilizando tectonic de forma directa:
tectonic main.tex
```

### C. Ejecución de Simulaciones en Python (`3_SIMULACIONES_PYTHON/`)

Para ejecutar los modelos numéricos discretos y regenerar los oscilogramas y curvas:

1. Instalar dependencias:
   ```bash
   pip install -r requirements.txt
   ```
2. Ejecutar los scripts:
   * **Generar oscilogramas del enlace nominal (Figuras 6, 7, 8 y 9):**
     ```bash
     python3 generar_oscilogramas_simulados.py
     ```
   * **Simulador transitorio extremo a extremo con verificación CRC-8:**
     ```bash
     python3 simular_enlace_datos.py --distancia 7.0 --graficar
     ```
   * **Deducción de ganancias, potencias y polos GBW:**
     ```bash
     python3 calcular_parametros_enlace.py
     ```
   * **Curvas de Biot-Savart y respuesta en frecuencia de tanques resonantes:**
     ```bash
     python3 generar_graficos.py
     ```

### D. Flasheo de Firmwares en Microcontroladores ESP32 (`4_FIRMWARE_ESP32/`)

Los firmwares están organizados como proyectos compatibles con **Arduino IDE** o **PlatformIO**:
* **Transmisor (`firmware_tx/`):** Abrir `firmware_tx.ino`, seleccionar la placa *ESP32 Dev Module* y flashear. Conectar el pin `GPIO18` al driver TC4427A.
* **Receptor (`firmware_rx/`):** Abrir `firmware_rx.ino`, seleccionar la placa *ESP32 Dev Module* y flashear. Conectar el pin `GPIO19` a la salida del comparador analógico LM393. La consola serie a `115200 bps` mostrará los paquetes decodificados con validación de integridad CRC-8.

---

## 3. Síntesis Técnica del Proyecto

* **Frecuencia Portadora:** $125.04\text{ kHz}$ (Banda RFID LF).
* **Modulación:** ASK / OOK (*Amplitude Shift Keying / On-Off Keying*).
* **Codificación Digital:** Manchester IEEE 802.3 a $250\text{ bps}$ ($T_{\text{bit}} = 4.0\text{ ms}$, $T_{\text{half}} = 2.0\text{ ms}$).
* **Canal Físico:** Inducción cuasiestática de campo cercano reactivo ($r \ll \lambda / 2\pi$, $\lambda = 2.4\text{ km}$).
* **Etapa de Potencia TX:** Driver push-pull TC4427A excitando un tanque serie ($L_1 = 1.0\text{ mH}$, $C_1 = 1.62\text{ nF}$, $R_{\text{limit}} = 150\,\Omega$, $I_{\text{rms}} \approx 13.8\text{ mA}$).
* **Etapa Analógica RX:** Tanque paralelo sintonizado ($Q_{\text{eff}} \approx 10.95$), preamplificador de dos etapas MCP6002 ($A_v(f_0) = 18.63\text{ V/V}$ por límite GBW), detector de envolvente Schottky BAT85 con filtro $R_7 C_6$ ($\tau = 1.0\text{ ms}$, descarga intersimbólica $>86\%$) y conformador digital LM393 con histéresis ($V_{\text{th}} = 0.30\text{ V} \pm 25\text{ mV}$).
* **Alcance Máximo Confiable:** $7.0\text{ cm}$ con tasa de error de paquete nula ($\text{PER} = 0$).
* **Caso de Falla Experimental Documentado:** Bobina artesanal tipo electroimán sobre ferrita ($3.5 \times 0.5 \times 1.3\text{ cm}$, $L \approx 42.5\,\mu\text{H}$) evidenciando la desintonización resonante a $f_0' \approx 607\text{ kHz}$ y atenuación del momento dipolar en $-57.9\text{ dB}$, corroborada con oscilograma Tektronix (CH1 a $2.16\text{ V}_{\text{pp}}$, CH2 en $0\text{ V}$ plano).
