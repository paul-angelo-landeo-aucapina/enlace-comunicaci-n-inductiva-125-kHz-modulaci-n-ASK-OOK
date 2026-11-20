# Enlace de Comunicación Inductiva a 125 kHz (ASK/OOK)
**Diseño, Arquitectura y Estado de Implementación Hardware**

---

## 1. Resumen Ejecutivo del Proyecto

El proyecto consiste en el diseño, análisis teórico y evaluación experimental de un enlace de comunicación inalámbrica por inducción magnética de corto alcance (campo cercano reactivo, sin propagación por radiación EM).

* **Frecuencia de portadora:** $125\text{ kHz}$ (Banda RFID LF).
* **Modulación:** ASK / OOK (*Amplitude Shift Keying / On-Off Keying*).
* **Fundamento electromagnético:** Ley de Faraday-Lenz ($\mathcal{E} = -\frac{d\Phi_B}{dt}$).
* **Plataforma de procesamiento:** Microcontroladores ESP32 DevKitC (uno para TX y otro para RX).
* **Componentes inductivos:** Bobinas RFID LF de $1\text{ mH}$.

---

## 2. Arquitectura del Sistema

```
ETAPA TRANSMISORA (TX - Alimentación 5V)
┌────────────────┐      ┌────────────────┐      ┌────────────────┐      ┌─────────────────────────┐
│ ESP32 DevKitC  │ GPIO │ TLC555         │ OUT  │ TC4427A        │ OUT  │ Tanque Resonante Serie  │
│ Datos OOK      ├─────>│ Oscilador      ├─────>│ Driver MOSFET  ├─────>│ R_limit (150 Ω)        │
│                │ 18   │ 125 kHz (~RST) │      │ Buffer de cte. │      │ C_res (1.62 nF)         │
└────────────────┘      └────────────────┘      └────────────────┘      │ L_coil (1 mH) ──> GND   │
                                                                        └───────────┬─────────────┘
                                                                                    │ Campo B (125 kHz)
ETAPA RECEPTORA (RX - Alimentación 3.3V)                                            ▼
┌────────────────┐      ┌────────────────┐      ┌────────────────┐      ┌─────────────────────────┐
│ ESP32 DevKitC  │ GPIO │ LM393          │ OUT  │ BAT85          │ OUT  │ Tanque Resonante Par.   │
│ Lectura Digital│<─────┤ Comparador con │<─────┤ Detector de    │<─────┤ L_coil (1 mH) ||        │
│ Interrupción   │ 19   │ Histéresis     │      │ Envolvente     │      │ C_res (1.62 nF)         │
└────────────────┘      └────────────────┘      └───────┬────────┘      │ Ref: Vref (1.65 V)      │
                                                        │               └─────────────────────────┘
                                                ┌───────┴────────┐
                                                │ MCP6002        │
                                                │ Preamplificador│
                                                │ 2 Etapas (~32x)│
                                                └────────────────┘
```

---

## 3. División de Componentes y BOM

### Etapa Transmisora (TX)
* **Control:** 1x ESP32 DevKitC (alimenta el circuito mediante pin `5V (VIN)` y `GND`; genera modulación en `GPIO18`).
* **Generación de Portadora:** 1x TLC555 (DIP-8).
* **Driver de Potencia:** 1x TC4427A (Driver dual de MOSFETs en DIP-8, $1.5\text{ A}$ pico).
* **Tanque Resonante TX:** 
  * 1x Bobina RFID LF de $1\text{ mH}$.
  * 1x Capacitor $1.5\text{ nF}$ (NP0/C0G) + 1x Capacitor $120\text{ pF}$ (NP0/C0G) en paralelo ($C_{eq} = 1.62\text{ nF} \rightarrow f_0 = 125.04\text{ kHz}$).
* **Pasivos y Ajuste:**
  * $R_1$: $4.7\text{ k}\Omega$ (Resistencia $R_A$ del 555).
  * $R_2$: $1\text{ k}\Omega$ en serie con Potenciómetro $RV_1$ de $10\text{ k}\Omega$ ($R_B$ calibrable).
  * $R_3$: $100\text{ k}\Omega$ (Pull-down de seguridad en pin 4 `~RST`).
  * $R_4$: $150\ \Omega$ (Amortiguamiento y límite de corriente serie para el driver).
  * $C_1$: $1\text{ nF}$ (Capacitor de temporización del 555).
  * $C_2$: $10\text{ nF}$ (Desacoplo pin 5 `CONT` del 555).
  * $C_3$, $C_4$: $100\text{ nF}$ cerámicos (Desacoplo local de U1 y U2).
  * $C_5$: $10\ \mu\text{F}$ electrolítico (Reserva de corriente para pulsos del TC4427A).

### Etapa Receptora (RX)
* **Tanque Resonante RX:**
  * 1x Bobina RFID LF de $1\text{ mH}$.
  * 1x Capacitor $1.5\text{ nF}$ + 1x Capacitor $120\text{ pF}$ en paralelo ($1.62\text{ nF}$).
* **Tierra Virtual ($V_{ref} = 1.65\text{ V}$):**
  * $R_1, R_2$: $2\times 100\text{ k}\Omega$ (Divisor simétrico desde 3.3V).
  * $C_1$: $1\ \mu\text{F}$ electrolítico + $C_2$: $100\text{ nF}$ cerámico (Filtrado de rizado).
* **Preamplificador de 2 Etapas (MCP6002):**
  * 1x MCP6002 (Op-Amp doble rail-to-rail, DIP-8, $\text{GBW} = 1.0\text{ MHz}$).
  * $C_5$: $10\text{ nF}$ (Acoplo AC desde el tanque resonante).
  * $R_3, R_5$: $2\times 10\text{ k}\Omega$ (Resistencias de entrada inversora $R_{in1}, R_{in2}$).
  * $R_4$: $68\text{ k}\Omega$ (Realimentación Etapa 1, $A_{v1} = -6.8$).
  * $R_6$: $47\text{ k}\Omega$ (Realimentación Etapa 2, $A_{v2} = -4.7$).
  * Ganancia total teórica: $A_{total} = (-6.8) \times (-4.7) \approx 32\text{ V/V}$.
* **Demodulador de Envolvente:**
  * $C_7$: $100\text{ nF}$ (Desacoplo AC de salida del MCP6002).
  * $R_{12}$: $100\text{ k}\Omega$ (Restauración de nivel DC a $0\text{ V}$ GND).
  * $D_1$: Diodo Schottky BAT85 (Baja caída directa $V_F \approx 0.25 - 0.30\text{ V}$).
  * $C_6$: $10\text{ nF}$ (Filtro pasa-bajos de envolvente).
  * $R_7$: $22\text{ k}\Omega - 100\text{ k}\Omega$ (Resistencia de descarga de envolvente).
* **Conformación Digital (LM393):**
  * 1x LM393 (Comparador dual colector abierto, DIP-8).
  * $R_8$: $100\text{ k}\Omega$ y $R_9$: $10\text{ k}\Omega$ (Divisor de umbral fijo en $V_{th} \approx 0.30\text{ V}$).
  * $R_{10}$: $1\text{ M}\Omega$ (Resistencia de histéresis positiva).
  * $R_{11}$: $4.7\text{ k}\Omega$ (Pull-up a 3.3V).
  * Desacoplos: $C_8, C_9$ ($100\text{ nF}$).
  * Salida a ESP32 DevKitC Pin `GPIO19`.

---

## 4. Decisiones de Diseño y Correcciones Realizadas

1. **Separación de Circuitos TX y RX:**
   * Inicialmente se encontraban mezclados en un único archivo. Se independizaron en directorios y proyectos KiCad dedicados: `circuito_tx/` y `circuito_rx/`.
2. **Topología de los Tanques Resonantes:**
   * **TX (Resonancia Serie):** Se conectó en serie ($C_{res} \rightarrow L_{coil} \rightarrow \text{GND}$). En resonancia ($X_L = X_C$), la reactancia se anula y la impedancia es mínima ($Z = R_{limit} + R_{ESR} \approx 160\,\Omega$), maximizando la corriente en la bobina emisora ($I_{peak} \approx 30\text{ mA}$) para generar el mayor flujo magnético posible.
   * **RX (Resonancia Paralela):** Se conectó en paralelo ($L_{coil} \parallel C_{res}$). En resonancia, la impedancia es máxima y multiplica la fuerza electromotriz inducida por el factor de calidad ($V_{tank} \approx Q \cdot \mathcal{E}_{ind}$), logrando ganancia pasiva de tensión previa a la amplificación.
3. **Sintonía de Frecuencia Exacta:**
   * Se corrigió la disposición de los capacitores de sintonía ($1.5\text{ nF}$ y $120\text{ pF}$), colocándolos en paralelo para sumar $1.62\text{ nF}$, logrando:
     $$f_0 = \frac{1}{2\pi \sqrt{1\text{ mH} \cdot 1.62\text{ nF}}} = 125.04\text{ kHz}$$
4. **Resolución del Offset DC en el Demodulador Monofuente:**
   * El MCP6002 opera con alimentación simple ($3.3\text{ V}$ y $\text{GND}$) y su punto de reposo está polarizado a $V_{ref} = 1.65\text{ V}$.
   * Si la salida del operacional se conecta directamente al diodo rectificador, el detector retiene $1.65\text{ V} - V_F \approx 1.35\text{ V}$. Frente a un comparador con umbral de $0.30\text{ V}$, la salida quedaba permanentemente saturada en nivel alto.
   * **Solución aplicada:** Se insertó un capacitor de desacoplo AC ($C_7 = 100\text{ nF}$) seguido de una resistencia a masa ($R_{12} = 100\text{ k}\Omega$), eliminando la componente continua y referenciando la señal rectificada estrictamente a $0\text{ V}$.
5. **Corrección de Conexión del Comparador LM393:**
   * Se corrigió un corto que unía la señal del detector al bus `#PWR12` (GND) y dejaba el Pin 3 (`+IN A`) flotante.
   * El Pin 3 quedó conectado directamente a la salida del filtro detector de envolvente (Cátodo de $D_1$, $C_6 \parallel R_7$) y a la realimentación de histéresis ($R_{10} = 1\text{ M}\Omega$).
6. **Protección del TLC555:**
   * Se colocó $R_2 = 1\text{ k}\Omega$ fija en serie con el potenciómetro de calibración $RV_1$ para evitar que la resistencia $R_B$ caiga a $0\,\Omega$ al girar el cursor al extremo, lo que destruiría el transistor interno de descarga (pin 7). El cursor de $RV_1$ se conectó al pin 3 para funcionar como reóstato seguro.
7. **Terminación de Pines en el Driver TC4427A:**
   * La entrada no utilizada `IN_B` (pin 4) se conectó a GND para evitar consumos y oscilaciones por ruido parásito.
   * Los pines sin conexión interna (pines 1 y 8 `NC`) y la salida no usada (pin 5 `OUT_B`) se marcaron con banderas explícitas `no_connect` ($\times$).

---

## 5. Estado Actual de los Archivos de Diseño

Ambos esquemáticos cuentan con **0 errores de Reglas Eléctricas (ERC)** en KiCad:

* **Transmisor (TX):**
  * Esquemático KiCad: [`circuito_tx/circuito_tx.kicad_sch`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_tx/circuito_tx.kicad_sch)
  * Plano exportado en PDF: [`circuito_tx/circuito_tx.pdf`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_tx/circuito_tx.pdf)
  * Proyecto: [`circuito_tx/circuito_tx.kicad_pro`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_tx/circuito_tx.kicad_pro)
* **Receptor (RX):**
  * Esquemático KiCad: [`circuito_rx/circuito_rx.kicad_sch`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_rx/circuito_rx.kicad_sch)
  * Plano exportado en PDF: [`circuito_rx/circuito_rx.pdf`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_rx/circuito_rx.pdf)
  * Proyecto: [`circuito_rx/circuito_rx.kicad_pro`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/circuito_rx/circuito_rx.kicad_pro)

---

## 6. Firmwares del Enlace Inductivo (`firmware_rx/` y `firmware_tx/`)

Se desarrollaron e integraron ambos firmwares optimizados para **ESP32 DevKitC** bajo el entorno **Arduino IDE / ESP-IDF**, garantizando una arquitectura de modulación y demodulación simétrica y robusta a $250\text{ bps}$:

### 6.1 Firmware Transmisor ESP32 (`firmware_tx/`)
Ubicado en [`firmware_tx/`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/firmware_tx/firmware_tx.ino):
* **Pin de Modulación:** `GPIO18` (controla por gating el pin 4 `~RST` del oscilador TLC555).
* **Indicador de Actividad:** `GPIO2` (LED onboard, se enciende durante la transmisión).
* **Motor de Temporización de Alta Precisión:** Utiliza el temporizador de hardware de microsegundos de ESP-IDF (`esp_timer`) configurado a $500\text{ Hz}$ ($T_{half} = 2000\ \mu\text{s}$). Ejecución no bloqueante que previene el jitter y no satura los timers de FreeRTOS.
* **Codificación de Línea:** Manchester IEEE 802.3 (Bit `'0'` $\rightarrow$ LOW luego HIGH; Bit `'1'` $\rightarrow$ HIGH luego LOW).
* **Constructor de Tramas (`PacketBuilder`):**
  1. **Preámbulo:** 2 bytes `0xAA, 0xAA` ($16$ transiciones para clock recovery del receptor).
  2. **Delimitador de Inicio (SFD):** `0x7E` (`01111110`).
  3. **Longitud:** 1 byte ($1 \le N \le 32$).
  4. **Payload:** $N$ bytes de datos binarios o texto ASCII.
  5. **Verificación:** CRC-8 (Polinomio $0\text{x}07$, Init $0\text{x}00$) calculado sobre $[Longitud] + [Payload]$.
* **Consola Interactiva USB Serial (115200 baud):**
  * `imu on|off`: Activa/desactiva la transmisión continua de telemetría inercial del sensor MPU6050 cada $600\text{ ms}$.
  * `read`: Realiza una lectura instantánea del MPU6050 y reporta aceleraciones, temperatura y ángulos Pitch/Roll en Serial y OLED.
  * `send <texto>`: Transmite una trama arbitraria con el texto especificado.
  * `ping`: Transmite paquete de prueba estándar `"PING"`.
  * `bench <N>`: Emite una ráfaga de $N$ paquetes consecutivos con intervalo de guarda para pruebas de PER (*Packet Error Rate*).
  * `carrier on|off`: Activa la portadora en `GPIO18` de forma continua a $125\text{ kHz}$ para sintonía con osciloscopio en el banco de trabajo.
  * `beacon on|off`: Baliza periódica de telemetría cada $3\text{ s}$.
  * `status`: Métricas de hardware, periféricos I2C y paquetes transmitidos.
* **Integración de Sensores y Pantalla (Bus I2C en `GPIO21`/`GPIO22`):**
  * **Sensor Inercial MPU6050 (`0x68`):** Controlador directo (`mpu6050_driver.cpp`) sin librerías externas pesadas. Lee en ráfaga aceleraciones triaxiales ($\pm 2g$) y temperatura con filtro pasabajos digital de $44\text{ Hz}$. Incluye modo simulado automático si el sensor no está conectado.
  * **Pantalla OLED SSD1306 (`0x3C`):** Controlador de texto ligero (`oled_display.cpp`) con fuente 5x7 en memoria de programa (sin consumo de buffer dinámico de 1KB). Visualiza en vivo el estado del enlace, secuencia de paquete, componentes de aceleración en $g$, ángulos Pitch/Roll estimados y temperatura.
  * **Paquete Binario de Telemetría (`imu_telemetry.h`):** Estructura empaquetada de solo $10\text{ bytes}$ (`[0x01] [Seq] [Ax,Ay,Az] [Temp]`), transmitida en $480\text{ ms}$ a $250\text{ bps}$ ($\sim 2\text{ Hz}$).

### 6.2 Firmware Receptor ESP32 (`firmware_rx/`)
Ubicado en [`firmware_rx/`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/firmware_rx/firmware_rx.ino):
* **Velocidad de Enlace:** $250\text{ bps}$ ($T_{bit} = 4000\ \mu\text{s}$, $T_{half} = 2000\ \mu\text{s}$). Adecuado para la constante de tiempo $\tau = R_7 C_6 = 1.0\text{ ms}$ del demodulador analógico.
* **Codificación de Línea:** Manchester IEEE 802.3 (Flanco ascendente en punto medio = `'0'`, descendente = `'1'`).
* **Entrada de Datos:** Pin `GPIO19` (conectado al comparador LM393 mediante interrupción `CHANGE`).
* **Tolerancia a Jitter y Ruido:**
  * Filtro anti-glitch para transitorios $< 600\ \mu\text{s}$.
  * Ventana de aceptación $\pm 25\%$ ($T_{half}: 1400 - 2600\ \mu\text{s}$, $T_{bit}: 2800 - 5200\ \mu\text{s}$).
  * Timeout automático de silencio a los $14\text{ ms}$ ($> 3\times T_{bit}$).
* **Estructura de Trama Esperada:**
  1. **Preámbulo:** `0xAA` para sincronización de reloj y máquina de estados.
  2. **Delimitador de Inicio (SFD):** `0x7E`.
  3. **Longitud:** 1 byte ($1 \le N \le 32$).
  4. **Carga Útil (Payload):** $N$ bytes.
  5. **Verificación:** CRC-8 (Polinomio $0\text{x}07$).
* **Decodificador Inteligente de Telemetría IMU:**
  * Detecta tramas de $10\text{ bytes}$ con cabecera `0x01` (`PKT_TYPE_IMU`).
  * Desempaqueta las aceleraciones $A_x, A_y, A_z$ y temperatura, calculando en tiempo real los ángulos de inclinación (Pitch y Roll en grados) para visualización inmediata en el monitor serie del receptor.

### 6.3 Suites de Pruebas Automatizadas y Loopback (`tests/`)
* [`tests/test_rx_decoder.cpp`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/tests/test_rx_decoder.cpp): Valida la decodificación Manchester, filtrado de transitorios espurios y CRC-8 frente a jitter inyectado.
* [`tests/test_tx_encoder.cpp`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/tests/test_tx_encoder.cpp): Valida el empaquetado de tramas, codificación de símbolos IEEE 802.3 y ejecuta una prueba **Closed-Loop Loopback** conectando la salida del transmisor a la entrada del receptor con 100% de tramas decodificadas sin error de CRC.
* [`tests/test_imu_telemetry.cpp`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/tests/test_imu_telemetry.cpp): Valida el empaquetamiento estricto de $10\text{ bytes}$ de la telemetría IMU, cálculos trigonométricos de Pitch y Roll, y transmisión en streaming por loopback cerrado sin error de bit.
* Compilación y ejecución de toda la suite mediante [`tests/Makefile`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/tests/Makefile): `make test`.

---

## 7. Validación Cuantitativa y Cálculos de Ingeniería (`informe_ieee/datos/`)

Se implementó el script automatizado [`informe_ieee/datos/calcular_parametros_enlace.py`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/datos/calcular_parametros_enlace.py) para modelar y corroborar numéricamente el enlace:

* **Ganancia del Sistema:**
  * Ganancia pasiva del tanque receptor: $Q_0 \approx 78.56$ ($+37.9\text{ dB}$).
  * Efecto de carga del preamplificador ($R_{\text{in}} = 10\text{ k}\Omega$): $Q_{\text{eff}} \approx 10.95$ ($+20.8\text{ dB}$).
  * Ganancia activa MCP6002 (2 etapas): $31.96\text{ V/V}$ en DC $\rightarrow 18.63\text{ V/V}$ a $125\text{ kHz}$ por limitación dinámica de GBW ($1.0\text{ MHz}$).
  * Ganancia total real del sistema: $A_{\text{total,real}} \approx 204.09\text{ V/V}$ ($+46.2\text{ dB}$).
* **Balance de Potencia Estimada:**
  * Transmisor (TX): Corriente fundamental en bobina $I_{\text{rms}} = 13.77\text{ mA}$ ($19.47\text{ mA}$ pico); potencia activa disipada en $R_{\text{limit}}$ ($28.43\text{ mW}$) y bobina ($1.90\text{ mW}$); potencia reactiva magnética $Q_{\text{mag}} = 148.89\text{ mVAR}$; consumo total TX $\approx 162.74\text{ mW}$ ($0.163\text{ W}$).
  * Receptor (RX): Consumo de circuitería analógica ($3.45\text{ mW}$) y ESP32 en recepción ($99.0\text{ mW}$); consumo total RX $\approx 102.45\text{ mW}$ ($0.102\text{ W}$).
* **Presupuesto Temporal de Enlace y Rendimiento:**
  * Modulación Manchester a $250\text{ bps}$ ($T_{\text{bit}} = 4.0\text{ ms}$, $T_{\text{half}} = 2.0\text{ ms}$).
  * Relación $T_{\text{half}} / \tau_{\text{env}} = 2.0 \implies$ descarga residual de $13.53\%$ ($>86\%$ de descarga), garantizando inmunidad a ISI.
  * Throughput útil: $166.7\text{ bps}$ para payload de 8 bytes ($384\text{ ms}$ por trama) y hasta $222.2\text{ bps}$ para 32 bytes ($1152\text{ ms}$).
  * Sensibilidad límite: Umbral del comparador $V_{\text{th}} = 0.30\text{ V}$ más barrera Schottky $V_F = 0.25\text{ V} \implies$ tensión mínima en tanque de $34.4\text{ a }59.0\text{ mV}_{\text{pp}}$, prediciendo el corte de enlace a partir de $7.0\text{ cm}$ (en concordancia con la medición experimental).

---

## 8. Informe Técnico en Formato IEEE (`informe_ieee/`)

El informe académico se encuentra estructurado en 6 secciones canónicas:
1. **Carátula:** Portada/Title block formal IEEE en [`informe_ieee/main.tex`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/main.tex) con marcadores de posición configurables para autores, filiación y asignatura.
2. **Introducción:** [`informe_ieee/secciones/01_introduccion.tex`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/secciones/01_introduccion.tex).
3. **Metodología:** [`informe_ieee/secciones/02_metodologia.tex`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/secciones/02_metodologia.tex) (Modelo físico, diseño de circuitos, simulación computacional, firmware/protocolo y banco experimental).
4. **Resultados:** [`informe_ieee/secciones/03_resultados.tex`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/secciones/03_resultados.tex) (Cálculos de ganancia, potencia y enlace, simulaciones, oscilogramas y tabla comparativa de mediciones).
5. **Discusión y Conclusiones:** [`informe_ieee/secciones/04_discusion_conclusiones.tex`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/secciones/04_discusion_conclusiones.tex) (Discusión técnica, sensibilidad del detector, conclusiones y trabajo futuro).
6. **Referencias:** Bibliografía normalizada en [`informe_ieee/referencias.bib`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/referencias.bib).

Documento unificado listo para visualización y lectura directa: [`informe_ieee/BORRADOR.md`](file:///home/paul/Documents/workfiles/ProyectoCAMPOS/informe_ieee/BORRADOR.md).
