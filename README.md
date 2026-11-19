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
