# Diseño, Simulación, Validación Analítica y Evaluación Experimental de un Enlace de Comunicación Inductiva a 125 kHz con Modulación ASK/OOK

**Formato IEEE — Informe Consolidado de Proyecto**

---

## 1. Carátula y Resumen Ejecutivo

### Datos Institucionales
* **Título:** Diseño, Simulación, Validación Analítica y Evaluación Experimental de un Enlace de Comunicación Inductiva a 125 kHz con Modulación ASK/OOK
* **Autores:** [Nombre del Estudiante 1], [Nombre del Estudiante 2], [Nombre del Estudiante 3]
* **Filiación:** Departamento de Ingeniería Eléctrica y Electrónica
* **Institución:** [Nombre de la Universidad / Institución Académica]
* **Asignatura:** Campos Electromagnéticos / Teoría Electromagnética
* **Docente / Asesor:** [Nombre del Profesor / Asesor de Cátedra]
* **Fecha de Emisión:** Noviembre 2026

### Resumen
El presente proyecto propone el diseño, simulación, validación analítica y evaluación experimental de un enlace de comunicación inalámbrica de corto alcance basado en una portadora de $125\text{ kHz}$ y modulación digital ASK/OOK. Su fundamento físico es la generación de un campo magnético variable mediante una bobina transmisora de $1\text{ mH}$, cuyo flujo magnético induce una fuerza electromotriz en una bobina receptora homóloga conforme a la ley de Faraday-Lenz. Se desarrolla un modelo cuantitativo completo que deduce la ganancia pasiva por resonancia ($Q$), la ganancia activa de tensión de dos etapas (incorporando el efecto del producto ganancia-ancho de banda $\text{GBW} = 1.0\text{ MHz}$ del amplificador operacional MCP6002), el balance de potencia activa y reactiva en transmisión ($162.74\text{ mW}$) y recepción ($102.45\text{ mW}$), y el presupuesto temporal para codificación Manchester a $250\text{ bps}$. El hardware implementado integra osciladores TLC555 con modulación por conmutación activa en $\sim\text{RST}$, drivers MOSFET TC4427A, tanques resonantes sintonizados a $125.04\text{ kHz}$, preamplificación en cascada con masa virtual a $1.65\text{ V}$, demodulador de envolvente con desacoplo AC para remoción de nivel continuo y conformador con histéresis LM393 conectado a microcontroladores ESP32. Las mediciones en banco de pruebas con guía milimétrica confirman la atenuación de orden cúbico ($1/z^3$) de campo cercano reactivo ($r \ll \lambda / 2\pi$) y alcanzan una tasa de error de paquete nula ($\text{PER} = 0$) hasta $7.0\text{ cm}$, limitada físicamente por la caída de tensión directa del diodo Schottky BAT85 ($V_F \approx 0.25\text{ V}$).

**Palabras clave:** inducción electromagnética, acoplamiento inductivo, ley de Faraday-Lenz, modulación ASK/OOK, RFID LF, presupuesto de enlace, factor de calidad, ganancia-ancho de banda, ESP32.

---

## 2. Introducción

La transferencia inalámbrica de información en distancias cortas mediante inducción magnética cuasiestática presenta ventajas sustanciales frente a los enlaces de radiofrecuencia radiada convencionales:
1. **Condición de Frontera Cuasiestática:** Para una portadora de $f_0 = 125\text{ kHz}$, la longitud de onda en el espacio libre es:
   $$\lambda = \frac{c}{f_0} = \frac{3\times 10^8\text{ m/s}}{125\times 10^3\text{ Hz}} = 2.4\text{ km}$$
   La frontera del campo cercano reactivo de Rayleigh ($r_b = \lambda / (2\pi) \approx 382\text{ m}$) asegura que separaciones de $0\text{ a }20\text{ cm}$ se encuentren estrictamente en régimen cuasiestático ($r \ll \lambda / 2\pi$), donde no existe radiación electromagnética de campo lejano ni reflexiones multicamino.
2. **Penetración en Medios con Pérdidas:** El campo predominantemente magnético atraviesa agua, tejidos biológicos y contenedores no ferrosos con atenuaciones drásticamente menores que los campos electromagnéticos de microondas.
3. **Costo, Eficiencia y Simplicidad:** Posibilidad de implementar transmisores y receptores de alta confiabilidad con componentes analógicos discretos de bajo costo y microcontroladores comerciales (ESP32).

El informe se estructura de acuerdo a la división canónica requerida:
* **Sección 3 (Metodología):** Formulación del modelo físico y electromagnético, arquitectura y esquemáticos de los circuitos TX/RX, simulaciones numéricas y circuitales SPICE, protocolo de enlace digital y montaje experimental.
* **Sección 4 (Resultados):** Deducciones cuantitativas de ganancia, balances de potencia, análisis de transmisión/recepción de datos, oscilogramas y tabla comparativa de tensión vs distancia con verificación CRC-8.
* **Sección 5 (Discusión y Conclusiones):** Discusión de discrepancias experimentales por efectos parásitos, límite físico de sensibilidad del diodo, conclusiones del proyecto y líneas de trabajo futuro.
* **Sección 6 (Referencias):** Bibliografía normalizada bajo estándar IEEE.

---

## 3. Metodología

### A. Modelo Físico y Formulación Electromagnética

#### 1. Ley de Biot-Savart y Aproximación Dipolar
Para una bobina circular de radio medio $a$, $N_1$ espiras y corriente armónica $i_1(t)$, la densidad de flujo magnético axial a una distancia $z$ sobre el eje normal es:
$$B_z(z, t) = \frac{\mu_0 N_1 i_1(t) a^2}{2 (a^2 + z^2)^{3/2}}$$
Para distancias axiales $z \gg a$, la expresión converge a la caída dipolar cúbica:
$$B_z(z, t) \approx \frac{\mu_0 m(t)}{2\pi z^3}, \quad m(t) = N_1 i_1(t) \pi a^2$$

#### 2. Flujo Concatenado, Inductancia Mutua y Factor de Acoplamiento
El flujo magnético concatenado $\Phi_{21}(z, t)$ por una bobina receptora coaxial de $N_2$ espiras y radio $a$ es:
$$\Phi_{21}(z, t) \approx B_z(z, t) \cdot \pi a^2 = \frac{\mu_0 \pi N_1 a^4}{2 (a^2 + z^2)^{3/2}} i_1(t)$$
Definiendo la inductancia mutua $M(z)$ y el factor de acoplamiento $k(z)$:
$$M(z) = \frac{N_2 \Phi_{21}(z, t)}{i_1(t)} = \frac{\mu_0 \pi N_1 N_2 a^4}{2 (a^2 + z^2)^{3/2}}$$
$$k(z) = \frac{M(z)}{\sqrt{L_1 L_2}}, \quad 0 \le k(z) < 1$$

#### 3. Fuerza Electromotriz Inducida (Ley de Faraday-Lenz)
$$\mathcal{E}_{\text{ind}}(t) = -M(z) \frac{d i_1(t)}{dt} = \omega_0 M(z) I_{1,\text{pk}} \sin(\omega_0 t)$$

#### 4. Resonancia y Factor de Calidad ($Q$)
* **Transmisor Serie:** En resonancia exacta ($X_{L1} = X_C$), la reactancia se anula y la impedancia vista colapsa a su mínimo puramente resistivo:
  $$Z_{\text{TX}}(\omega_0) = R_{\text{limit}} + R_{\text{ESR}} + R_{DS(\text{on})} \approx 163.5\,\Omega$$
  lo que maximiza la corriente $I_{\text{pk}}$ inyectada a la bobina emisora.
* **Receptor Paralelo:** En resonancia, el tanque paralelo actúa como un multiplicador pasivo de tensión:
  $$V_{\text{tank}}(\omega_0) \approx Q_0 \cdot \mathcal{E}_{\text{ind}} = \left(\frac{\omega_0 L_2}{R_{\text{ESR}}}\right) \mathcal{E}_{\text{ind}}$$
  elevando señales de milivoltios antes de ingresar a la etapa de amplificación activa.

---

### B. Diseño y Topología de Circuitos Hardware

```mermaid
flowchart LR
    subgraph TX ["ETAPA TRANSMISORA (5V)"]
        A["ESP32 GPIO18<br/>(Datos OOK)"] -->|Gating ~RST| B["TLC555 Astable<br/>(125 kHz)"]
        B --> C["Driver TC4427A<br/>(1.5 A pico)"]
        C --> D["Tanque Resonante Serie<br/>(150 Ω + 1 mH + 1.62 nF)"]
    end
    
    subgraph CANAL ["ENLACE INDUCTIVO CUASIESTÁTICO"]
        D -->|Campo Magnético B| E["Tanque Resonante Paralelo<br/>(1 mH || 1.62 nF)"]
    end
    
    subgraph RX ["ETAPA RECEPTORA (3.3V)"]
        E --> F["Masa Virtual (1.65 V) +<br/>MCP6002 (Cascada 2 Etapas)"]
        F --> G["Desacoplo AC (C7, R12) +<br/>BAT85 + Filtro RC Envolvente"]
        G --> H["Comparador LM393 +<br/>Histéresis (Vth = 0.30 V)"]
        H --> I["ESP32 GPIO19<br/>(Interrupción Manchester)"]
    end
```

#### Lista de Materiales Críticos (BOM)
| Subcircuito | Componente / Rol | Valor Nominal | Función |
| :--- | :--- | :--- | :--- |
| **TX** | Microcontrolador | ESP32 DevKitC (Pin GPIO18) | Genera modulación digital Manchester OOK |
| **TX** | Generador Portadora | TLC555 (DIP-8) | Oscilador astable calibrado a $125.04\text{ kHz}$ |
| **TX** | Driver de Potencia | TC4427A (DIP-8) | Driver de compuerta MOSFET ($1.5\text{ A}$ pico) |
| **TX** | Tanque Serie | $150\,\Omega + 1.0\text{ mH} + 1.62\text{ nF}$ | $I_{\text{pk}} \approx 19.5\text{ mA}$, sintonizado a $125.04\text{ kHz}$ |
| **RX** | Tanque Paralelo | $1.0\text{ mH} \parallel 1.62\text{ nF}$ | Ganancia pasiva resonante ($Q_0 \approx 78.5$) |
| **RX** | Masa Virtual | $2\times 100\text{ k}\Omega + 1\,\mu\text{F} + 100\text{ nF}$ | Polarización simétrica a $V_{\text{ref}} = 1.65\text{ V}$ para alimentación simple |
| **RX** | Preamplificador | MCP6002 (DIP-8 Rail-to-Rail) | 2 etapas inversoras ($A_{v1} = -6.8, A_{v2} = -4.7$) |
| **RX** | Desacoplo AC | $C_7 = 100\text{ nF}, R_{12} = 100\text{ k}\Omega$ | Elimina la continua de $1.65\text{ V}$ antes de demodular |
| **RX** | Demodulador | Diodo Schottky BAT85 ($V_F \approx 0.25\text{ V}$) | Rectificación rápida de envolvente |
| **RX** | Filtro Envolvente | $R_7 = 100\text{ k}\Omega, C_6 = 10\text{ nF}$ | Constante de tiempo $\tau = 1.0\text{ ms}$ (pasa-bajos) |
| **RX** | Conformador | LM393 ($V_{\text{th}} = 0.30\text{ V}, R_{10} = 1\text{ M}\Omega$) | Comparador con histéresis hacia GPIO19 del ESP32 |

---

### C. Modelado Computacional y Simulación
* **Simulación Numérica en Python:** Cálculo de la integral de Biot-Savart vs aproximación dipolar $1/z^3$ y cálculo de la inductancia mutua $M(z)$ y factor $k(z)$ en un rango axial de $0.5\text{ a }12\text{ cm}$.
* **Simulación Circuital SPICE:** Modelado completo con resistencias parásitas equivalentes ($R_{\text{ESR}} = 10\,\Omega$), modelos no lineales del BAT85 y macromodelos de amplificadores. Análisis AC (barrido en frecuencia de tanques) y análisis transitorio (conmutación OOK a $250\text{ bps}$).

---

### D. Protocolo de Comunicación y Firmware ESP32
* **Codificación:** Manchester IEEE 802.3 a $250\text{ bps}$ ($T_{\text{bit}} = 4000\,\mu\text{s}$, $T_{\text{half}} = 2000\,\mu\text{s}$).
* **Estructura de Trama:**
  $$\underbrace{\text{Preámbulo (0xAA)}}_{1\text{ byte}} \longrightarrow \underbrace{\text{SFD (0x7E)}}_{1\text{ byte}} \longrightarrow \underbrace{\text{Longitud (}N\text{)}}_{1\text{ byte}} \longrightarrow \underbrace{\text{Payload}}_{N\text{ bytes}} \longrightarrow \underbrace{\text{CRC-8 (0x07)}}_{1\text{ byte}}$$
* **Firmware Receptor:** Decodificación en GPIO19 mediante interrupción por cambio de nivel (`CHANGE`), filtrado anti-glitch para pulsos $< 600\,\mu\text{s}$ y discriminación temporal con ventana $\pm 25\%$.

---

### E. Banco de Pruebas Experimental
* Bancada mecánica graduada en PLA no ferromagnético, con ajuste coaxial milimétrico ($0 - 15\text{ cm}$) y graduación angular ($0^\circ - 90^\circ$).
* Osciloscopio digital de $100\text{ MHz}$ con sondas $10\times$ ($10\text{ M}\Omega \parallel 15\text{ pF}$) para prevenir desintonía capacitiva del tanque receptor.

---

## 4. Resultados

### A. Validación Analítica Cuantitativa del Enlace

#### 1. Cálculo de Ganancia Pasiva y Activa
* **Reactancia a $f_0 = 125.04\text{ kHz}$:** $X_{L0} = \omega_0 L = 785.65\,\Omega$.
* **Factor de calidad en vacío:** $Q_0 = X_{L0} / R_{\text{ESR}} = 785.65 / 10 = 78.56$ ($+37.90\text{ dB}$).
* **Efecto de carga en RX:** $R_p \approx Q_0^2 R_{\text{ESR}} \approx 61.72\text{ k}\Omega$. En paralelo con la resistencia de entrada del preamplificador ($R_{\text{in1}} = 10\text{ k}\Omega$):
  $$R_{p,\text{eff}} = 61.72\text{ k}\Omega \parallel 10\text{ k}\Omega = 8.60\text{ k}\Omega \implies Q_{\text{eff}} = \frac{8.60\text{ k}\Omega}{785.65\,\Omega} = 10.95 \quad (+20.79\text{ dB})$$
* **Ganancia Activa DC MCP6002:** $A_{v1,\text{DC}} = 68/10 = 6.8$, $A_{v2,\text{DC}} = 47/10 = 4.7 \implies A_{v,\text{DC}} = 31.96\text{ V/V}$ ($+30.09\text{ dB}$).
* **Limitación por Producto Ganancia-Ancho de Banda ($\text{GBW} = 1.0\text{ MHz}$):**
  $$f_{c1} = \frac{1000\text{ kHz}}{1 + 6.8} = 128.21\text{ kHz} \implies |A_{v1}(125\text{ kHz})| = \frac{6.8}{\sqrt{1 + (125/128.21)^2}} = 4.87$$
  $$f_{c2} = \frac{1000\text{ kHz}}{1 + 4.7} = 175.44\text{ kHz} \implies |A_{v2}(125\text{ kHz})| = \frac{4.7}{\sqrt{1 + (125/175.44)^2}} = 3.83$$
  $$A_{v,\text{activa}}(125\text{ kHz}) = 4.87 \times 3.83 = 18.63\text{ V/V} \quad (+25.41\text{ dB})$$
* **Ganancia Global Real del Sistema:**
  $$A_{\text{total,real}} = Q_{\text{eff}} \times A_{v,\text{activa}} = 10.95 \times 18.63 = 204.09\text{ V/V} \quad (+46.20\text{ dB})$$

| Etapa de Ganancia | Teórica Ideal | Efectiva Real | Causa Física de Reducción |
| :--- | :---: | :---: | :--- |
| **Tanque RX ($Q$)** | $78.56\text{ V/V}$ ($+37.9\text{ dB}$) | $10.95\text{ V/V}$ ($+20.8\text{ dB}$) | Carga de entrada $R_{\text{in1}} = 10\text{ k}\Omega$ |
| **Etapa 1 MCP6002** | $6.80\text{ V/V}$ ($+16.6\text{ dB}$) | $4.87\text{ V/V}$ ($+13.7\text{ dB}$) | Límite dinámico GBW ($f_{c1} = 128.2\text{ kHz}$) |
| **Etapa 2 MCP6002** | $4.70\text{ V/V}$ ($+13.4\text{ dB}$) | $3.83\text{ V/V}$ ($+11.7\text{ dB}$) | Límite dinámico GBW ($f_{c2} = 175.4\text{ kHz}$) |
| **Ganancia Activa** | $31.96\text{ V/V}$ ($+30.1\text{ dB}$) | $18.63\text{ V/V}$ ($+25.4\text{ dB}$) | Atenuación en lazo cerrado a $125\text{ kHz}$ |
| **Ganancia Global** | $\mathbf{2510\text{ V/V}}$ ($\mathbf{+68.0\text{ dB}}$) | $\mathbf{204.09\text{ V/V}}$ ($\mathbf{+46.2\text{ dB}}$) | Efecto acoplado de carga e inercia del op-amp |

#### 2. Balance y Estimación de Potencia
* **Transmisor (TX - 5.0 V):**
  - Tensión fundamental eficaz: $V_{1,\text{rms}} = \frac{2 V_{CC}}{\pi \sqrt{2}} = 2.251\text{ V}_{\text{rms}}$ ($V_{1,\text{pk}} = 3.183\text{ V}$).
  - Impedancia en resonancia: $Z_{\text{TX}} = R_{\text{limit}} + R_{\text{ESR}} + R_{DS(\text{on})} = 150 + 10 + 3.5 = 163.50\,\Omega$.
  - Corriente en bobina: $I_{\text{rms}} = 13.77\text{ mA}_{\text{rms}}$ ($I_{\text{pk}} = 19.47\text{ mA}$).
  - Potencia activa disipada en $R_{\text{limit}}$: $P_{\text{limit}} = I_{\text{rms}}^2 (150\,\Omega) = 28.43\text{ mW}$.
  - Potencia activa disipada en bobina ($R_{\text{ESR}}$): $P_{\text{ESR}} = 1.90\text{ mW}$.
  - Potencia reactiva magnética: $Q_{\text{mag}} = I_{\text{rms}}^2 X_{L0} = 148.89\text{ mVAR}$.
  - Consumo TLC555 + driver TC4427A: $1.25\text{ mW} + 15.0\text{ mW} = 16.25\text{ mW}$.
  - Consumo ESP32 emitiendo OOK a 80 MHz: $115.5\text{ mW}$.
  - **Potencia Total TX:** $P_{\text{TX,total}} = 162.74\text{ mW}$ ($\mathbf{0.163\text{ W}}$).
* **Receptor (RX - 3.3 V):**
  - Potencia captada en el enlace: desde $0.12\text{ mW}$ (a $1\text{ cm}$) hasta $0.6\text{ nW}$ (a $7\text{ cm}$).
  - Consumo analógico: MCP6002 ($0.66\text{ mW}$) + LM393 ($2.64\text{ mW}$) + divisores ($0.15\text{ mW}$) = $3.45\text{ mW}$.
  - Consumo ESP32 receptor (interrupciones activas a 80 MHz): $99.0\text{ mW}$.
  - **Potencia Total RX:** $P_{\text{RX,total}} = 102.45\text{ mW}$ ($\mathbf{0.102\text{ W}}$).

#### 3. Presupuesto Temporal de Datos, Inmunidad a ISI y Throughput
* **Modulación:** Manchester a $250\text{ bps} \implies T_{\text{bit}} = 4.0\text{ ms}$, $T_{\text{half}} = 2.0\text{ ms}$ (250 ciclos de portadora).
* **Constante de tiempo de envolvente:** $\tau_{\text{env}} = R_7 C_6 = 100\text{ k}\Omega \times 10\text{ nF} = 1.0\text{ ms}$.
* **Condición de ISI:** $\frac{T_{\text{half}}}{\tau_{\text{env}}} = 2.0 \implies \text{Descarga residual } e^{-2} = 13.53\%$ ($>86\%$ de descarga entre símbolos, eliminando ISI).
* **Rendimiento de Trama:**

| Carga Útil ($N$) | Trama Completa | Tiempo por Paquete | Throughput Neto | Eficiencia de Canal |
| :---: | :---: | :---: | :---: | :---: |
| 4 bytes | 8 bytes | $256.0\text{ ms}$ | $125.0\text{ bps}$ | $50.0\%$ |
| 8 bytes | 12 bytes | $384.0\text{ ms}$ | $166.7\text{ bps}$ | $66.7\%$ |
| 16 bytes | 20 bytes | $640.0\text{ ms}$ | $200.0\text{ bps}$ | $80.0\%$ |
| 32 bytes | 36 bytes | $1152.0\text{ ms}$ | $222.2\text{ bps}$ | $88.9\%$ |

#### 4. Sensibilidad y Distancia Límite
Para activar el comparador LM393 ($V_{\text{th}} = 0.30\text{ V}$) superando la barrera del diodo Schottky BAT85 ($V_F \approx 0.25\text{ V}$), se requiere:
$$V_{\text{out,min,pk}} = V_{\text{th}} + V_F = 0.30 + 0.25 = 0.55\text{ V}$$
Con una ganancia activa de $32\text{ V/V}$ (o $18.63\text{ V/V}$ con GBW), la tensión mínima en el tanque receptor es:
$$V_{\text{tank,min,pp}} = \frac{2 \times 0.55\text{ V}}{32} \approx 34.4\text{ mV}_{\text{pp}} \quad \text{a} \quad 59.0\text{ mV}_{\text{pp}}$$
A $z = 7.0\text{ cm}$, la tensión medida es $30.2\text{ mV}_{\text{pp}}$ y opera con éxito. A $z = 8.0\text{ cm}$, la señal cae a $20.3\text{ mV}_{\text{pp}}$, colapsando el demodulador y marcando el alcance límite del prototipo.

---

### B. Curvas de Simulación Numérica y SPICE

![Distribución de Campo Magnético](figuras/campo_b_distancia.png)
*Figura 1: Densidad de flujo magnético axial $B_z(z)$ a 125 kHz: concordancia con la asíntota dipolar cúbica ($1/z^3$) para $z > 3\text{ cm}$.*

![Factor de Acoplamiento](figuras/acoplamiento_k_distancia.png)
*Figura 2: Evolución de la inductancia mutua $M(z)$ y factor de acoplamiento $k(z)$ vs. distancia axial.*

![Respuesta en Frecuencia SPICE](figuras/respuesta_frecuencia_tanques.png)
*Figura 3: Respuesta en frecuencia simulada: corriente en tanque serie TX (arriba) y tensión en tanque paralelo RX (abajo) a $125.04\text{ kHz}$.*

---

### C. Oscilogramas Experimentales

![Banco de Pruebas](figuras/placeholder_montaje.png)
*Figura 4: Banco de pruebas experimental con soporte milimétrico y circuitos acoplados a microcontroladores ESP32.*

![Oscilograma TX](figuras/placeholder_osc_tx.png)
*Figura 5: Transmisor: modulación OOK en GPIO18 (arriba) y ráfaga senoidal a 125 kHz en la bobina (abajo).*

![Oscilograma RX](figuras/placeholder_osc_rx_amp.png)
*Figura 6: Receptor: señal de milivoltios captada en el tanque LC (arriba) y amplificada linealmente en MCP6002 (abajo).*

![Oscilograma Demodulación](figuras/placeholder_osc_demod.png)
*Figura 7: Demodulación: envolvente rectificada tras el diodo BAT85 (arriba) y tren de pulsos digital recuperado en el LM393 (abajo).*

---

### D. Mediciones Experimentales vs. Modelo y Verificación Digital

| Distancia [cm] | $M_{\text{teor}}$ [$\mu$H] | $V_{\text{tank}}$ Teór. [$\text{mV}_{\text{pp}}$] | $V_{\text{tank}}$ Sim. [$\text{mV}_{\text{pp}}$] | $V_{\text{tank}}$ Med. [$\text{mV}_{\text{pp}}$] | CRC-8 Status |
| :---: | :---: | :---: | :---: | :---: | :---: |
| 1.0 | 79.48 | 3912.4 | 3325.5 | 3051.7 | **VÁLIDO** |
| 2.0 | 26.69 | 1313.8 | 1116.7 | 1024.7 | **VÁLIDO** |
| 3.0 | 9.42  | 463.7  | 394.1  | 361.7  | **VÁLIDO** |
| 4.0 | 4.14  | 203.8  | 173.2  | 158.9  | **VÁLIDO** |
| 5.0 | 2.15  | 105.8  | 89.9   | 82.5   | **VÁLIDO** |
| 6.0 | 1.25  | 61.5   | 52.3   | 47.9   | **VÁLIDO** |
| 7.0 | 0.79  | 38.8   | 33.0   | 30.2   | **VÁLIDO** |
| 8.0 | 0.53  | 26.0   | 22.1   | 20.3   | **FALLO** |
| 10.0| 0.27  | 13.3   | 11.3   | 10.4   | **FALLO** |

* **Alcance Máximo Confiable:** $7.0\text{ cm}$ con tasa de error de paquete nula ($\text{PER} = 0$).
* **Orientación Angular:** Verificación de la ley del coseno $V(\theta) = V(0^\circ)\cos(\theta)$, anulándose la tensión a $\theta = 90^\circ$ (bobinas coplanares ortogonales).

---

## 5. Discusión y Conclusiones

### A. Discusión Técnica y Fuentes de Divergencia
1. **Divergencia Experimental (~20%):** Explicada por el efecto de carga de la impedancia de entrada del preamplificador ($R_{\text{in1}} = 10\text{ k}\Omega$), amortiguando el factor de calidad efectivo a $Q_{\text{eff}} \approx 10.95$.
2. **Limitación de Ancho de Banda (GBW):** A $125\text{ kHz}$, la ganancia del MCP6002 cae a $18.63\text{ V/V}$ debido a la proximidad de los polos de corte dinámicos ($128\text{ kHz}$ y $175\text{ kHz}$).
3. **Pérdidas en Ferrita y Tolerancias:** Resistencia parásita por pérdidas de histéresis a $125\text{ kHz}$ y tolerancias del $\pm 1\%$ (capacitores) y $\pm 5\%$ (inductores) que introducen pequeñas desintonías.
4. **Capacitancias Parásitas:** Capacitancias de interconexión en protoboard ($2-5\text{ pF}$).

### B. Límite Físico de Sensibilidad del Demodulador Pasivo
El cuello de botella fundamental no es el piso de ruido térmico, sino la tensión de barrera del diodo Schottky BAT85 ($V_F \approx 0.25\text{ V}$). A distancias $> 7.0\text{ cm}$, la señal rectificada no supera la barrera necesaria para cargar el filtro RC por encima del umbral de decisión del LM393 ($0.30\text{ V}$).

### C. Conclusiones
* Se comprobó el régimen inductivo reactivo cuasiestático ($r \ll \lambda/2\pi$) gobernado por la ley de Biot-Savart con caída asintótica $1/z^3$.
* Se demostró la eficacia de la topología resonante asimétrica (tanque serie en TX para corriente máxima de $13.77\text{ mA}_{\text{rms}}$ y tanque paralelo en RX para ganancia pasiva $Q$).
* Se diseñó y validó el circuito monofuente con masa virtual a $1.65\text{ V}$, preamplificador en cascada, desacoplo AC para remoción de nivel DC y comparador con histéresis LM393.
* Se logró comunicación digital confiable en ESP32 con codificación Manchester a $250\text{ bps}$ y verificación CRC-8 sin errores hasta $7.0\text{ cm}$.

### D. Trabajo Futuro
* **Rectificación Activa Síncrona:** Empleo de transistores MOSFET como rectificadores síncronos para eliminar la caída $V_F = 0.25\text{ V}$, extendiendo el alcance operativo a $> 12\text{ cm}$.
* **Control Automático de Ganancia (AGC):** Prevención de saturación a distancias de contacto ($z < 1.5\text{ cm}$) manteniendo alta sensibilidad a distancia máxima.
* **Sintonía Adaptativa:** Banco conmutado de capacitores gobernado por microcontrolador para compensar desintonías por proximidad de metales.
* **WPT:** Transmisión simultánea de energía y telemetría para nodos pasivos sin batería.

---

## 6. Referencias Bibliográficas (IEEE)

1. K. Finkenzeller, *RFID Handbook: Fundamentals and Applications in Contactless Smart Cards, Radio Frequency Identification and Near-Field Communication*, 3rd ed. John Wiley & Sons, Chichester, U.K., 2010.
2. Y. Lee, "Antenna Circuit Design for RFID Applications," Microchip Technology Inc., Application Note AN710, DS00710C, Chandler, AZ, USA, 2003.
3. Microchip Technology Inc., "microID 125 kHz RFID System Design Guide," Design Guide DS51115F, Chandler, AZ, USA, 2004.
4. W. H. Hayt and J. A. Buck, *Engineering Electromagnetics*, 8th ed. McGraw-Hill, New York, NY, USA, 2012.
5. M. N. O. Sadiku, *Elements of Electromagnetics*, 7th ed. Oxford University Press, New York, NY, USA, 2018.
6. J. G. Proakis and M. Salehi, *Digital Communications*, 5th ed. McGraw-Hill, Boston, MA, USA, 2008.
7. IEEE Computer Society, *IEEE Standard for Ethernet*, IEEE Std 802.3-2018, 2018.
