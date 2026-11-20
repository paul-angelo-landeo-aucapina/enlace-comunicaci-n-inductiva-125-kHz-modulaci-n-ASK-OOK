#!/usr/bin/env python3
"""
simular_enlace_datos.py
Simulador extremo a extremo (End-to-End) del Enlace Inductivo a 125 kHz:
1. Capa Digital TX: Trama con Preámbulo (2x 0xAA), SFD (0x7E), Longitud, Payload y CRC-8 (0x07).
2. Codificación Manchester: IEEE 802.3 a 250 bps.
3. Modulación RF: Portadora senoidal a 125 kHz conmutada por OOK.
4. Canal Inductivo: Atenuación física vs distancia axial z (calibrada con mediciones de laboratorio).
5. Etapa Analógica RX:
   - Tanque LC resonante paralelo.
   - Preamplificador MCP6002 (límite rail-to-rail a 3.3V con masa virtual a 1.65V).
   - Desacoplo AC (remoción de 1.65V).
   - Detector de pico no lineal: Diodo Schottky BAT85 (Vf = 0.18V en baja corriente) + Filtro RC (R7=25k, C6=10nF).
   - Conformador LM393 con histéresis (Vth = 0.30 V +- 25 mV).
6. Firmware Receptor ESP32:
   - Detección de flancos por interrupción GPIO.
   - Filtro anti-glitch (< 600 us).
   - Máquina de estados Manchester idéntica a manchester_rx.cpp.
   - Parser de paquetes por registro de desplazamiento bit a bit (packet_parser.cpp) y convalidación CRC-8.

Uso:
    python3 simular_enlace_datos.py                    # Simula a 4.0 cm (Rango válido)
    python3 simular_enlace_datos.py --distancia 7.0   # Simula a 7.0 cm (Límite operativo experimental)
    python3 simular_enlace_datos.py --distancia 8.0   # Simula a 8.0 cm (Fallo físico por corte)
    python3 simular_enlace_datos.py --graficar         # Guarda oscilograma de simulación en figuras/
"""

import sys
import os
import argparse
import numpy as np

# -----------------------------------------------------------------------------
# 1. PARÁMETROS DE HARDWARE Y PROTOCOLO
# -----------------------------------------------------------------------------
F_CARRIER = 125.04e3    # Hz (125 kHz)
BAUD_RATE = 250.0       # bps
T_BIT = 1.0 / BAUD_RATE # 4.0 ms (4000 us)
T_HALF = T_BIT / 2.0    # 2.0 ms (2000 us)

# Parámetros analógicos
V_CC_TX = 5.0           # V (alimentación TX)
V_DD_RX = 3.3           # V (alimentación RX monofuente)
V_BIAS_RX = 1.65        # V (masa virtual simétrica)
GAIN_ACTIVE_AC = 32.0   # V/V (ganancia del preamplificador MCP6002)
V_DIODE_F = 0.18        # V (caída Schottky BAT85 para corrientes de microamperios)
TAU_CHARGE = 2.0e-6     # s (carga rápida del diodo en conducción, 2 us)
TAU_DISCHARGE = 0.25e-3 # s (descarga RC a través de R7=25k, C6=10nF)
V_TH_COMP = 0.30        # V (umbral de decisión LM393)
V_HYST = 0.025          # V (histéresis positiva +-25 mV)

# Muestreo temporal continuo para simular portadora y transitorios analógicos
FS = 1.25e6             # 1.25 MHz (10 muestras por ciclo de 125 kHz)

# -----------------------------------------------------------------------------
# 2. CAPA DIGITAL TX: CONSTRUCCIÓN DE TRAMA Y CRC-8
# -----------------------------------------------------------------------------
def calc_crc8(data_bytes):
    """Calcula CRC-8 con polinomio estándar 0x07 (x^8 + x^2 + x + 1)."""
    crc = 0x00
    for byte in data_bytes:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ 0x07) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
    return crc

def construir_trama(payload_str):
    """
    Construye la trama binaria conforme al firmware:
    [Preámbulo (0xAA, 0xAA)] [SFD (0x7E)] [Longitud N] [Payload N bytes] [CRC-8]
    """
    payload_bytes = [ord(c) for c in payload_str]
    length_byte = len(payload_bytes)
    crc = calc_crc8([length_byte] + payload_bytes)
    return [0xAA, 0xAA, 0x7E, length_byte] + payload_bytes + [crc]

def bytes_a_bits(byte_list):
    """Convierte bytes a array de bits (MSB first)."""
    bits = []
    for b in byte_list:
        for i in range(7, -1, -1):
            bits.append((b >> i) & 1)
    return np.array(bits, dtype=int)

# -----------------------------------------------------------------------------
# 3. CODIFICACIÓN MANCHESTER Y MODULACIÓN OOK
# -----------------------------------------------------------------------------
def codificar_manchester(bits):
    """
    Manchester IEEE 802.3:
    Bit '0' -> Flanco ascendente (0 en primer semi-período, 1 en segundo).
    Bit '1' -> Flanco descendente (1 en primer semi-período, 0 en segundo).
    """
    symbols = []
    for bit in bits:
        if bit == 0:
            symbols.extend([0, 1])
        else:
            symbols.extend([1, 0])
    return np.array(symbols, dtype=int)

def generar_senal_tx(manchester_symbols, fs=FS):
    """Genera la señal OOK a 125 kHz en la bobina transmisora."""
    samples_per_half = int(fs * T_HALF)
    total_samples = len(manchester_symbols) * samples_per_half
    t = np.arange(total_samples) / fs
    
    envolvente = np.repeat(manchester_symbols, samples_per_half)
    portadora = np.sin(2 * np.pi * F_CARRIER * t)
    tx_signal = (2.0 * V_CC_TX / np.pi) * envolvente * portadora
    return t, envolvente, tx_signal

# -----------------------------------------------------------------------------
# 4. CANAL INDUCTIVO Y ETAPA ANALÓGICA RECEPTORA
# -----------------------------------------------------------------------------
def atenuacion_canal_experimental(z_cm):
    """
    Tensión inducida pico en el tanque receptor según la distancia axial z (cm),
    calibrada con los datos experimentales y modelo dipolar 1/z^3.
    """
    puntos_ref = [
        (1.0, 3051.7),
        (2.0, 1024.7),
        (3.0, 361.7),
        (4.0, 158.9),
        (5.0, 82.5),
        (6.0, 47.9),
        (7.0, 30.2),
        (8.0, 20.3),
        (10.0, 10.4)
    ]
    z_vals = [p[0] for p in puntos_ref]
    v_vals = [p[1] for p in puntos_ref]
    
    log_z = np.log10(z_vals)
    log_v = np.log10(v_vals)
    
    v_pp_mV = 10.0 ** np.interp(np.log10(max(0.5, z_cm)), log_z, log_v)
    v_peak = (v_pp_mV * 1e-3) / 2.0
    return v_peak, v_pp_mV

def simular_etapa_rx(t, tx_signal, z_cm, ruido_sigma=0.001, fs=FS):
    """
    Simula el tanque receptor, amplificador MCP6002, diodo BAT85, filtro RC y LM393.
    """
    v_tank_peak, v_tank_pp_mV = atenuacion_canal_experimental(z_cm)
    
    # Señal normalizada en el tanque receptor
    v_tx_max = np.max(np.abs(tx_signal))
    v_tank = (tx_signal / v_tx_max) * v_tank_peak
    
    if ruido_sigma > 0:
        v_tank += np.random.normal(0, ruido_sigma * v_tank_peak, size=len(t))
        
    # Preamplificador MCP6002 (Rail-to-Rail a 3.3V, reposo en 1.65V):
    v_amp_lineal = V_BIAS_RX + (v_tank * GAIN_ACTIVE_AC)
    v_amp_clipped = np.clip(v_amp_lineal, 0.05, 3.25)
    
    # Desacoplo AC (remoción de 1.65V):
    v_ac = v_amp_clipped - V_BIAS_RX
    
    # Diodo Schottky BAT85:
    v_rect = np.maximum(0.0, v_ac - V_DIODE_F)
    
    # Detector de envolvente RC:
    dt = 1.0 / fs
    alpha_charge = dt / (TAU_CHARGE + dt)
    alpha_discharge = dt / (TAU_DISCHARGE + dt)
    
    v_env = np.zeros_like(v_rect)
    for k in range(1, len(v_rect)):
        vin = v_rect[k]
        if vin > v_env[k - 1]:
            v_env[k] = alpha_charge * vin + (1.0 - alpha_charge) * v_env[k - 1]
        else:
            v_env[k] = (1.0 - alpha_discharge) * v_env[k - 1]
            
    # Comparador LM393 con histéresis:
    v_digital = np.zeros_like(v_env)
    estado = 0
    th_high = V_TH_COMP + V_HYST
    th_low = V_TH_COMP - V_HYST
    
    for k in range(len(v_env)):
        if estado == 0 and v_env[k] > th_high:
            estado = 1
        elif estado == 1 and v_env[k] < th_low:
            estado = 0
        v_digital[k] = 3.3 if estado == 1 else 0.0
        
    return v_tank, v_amp_clipped, v_env, v_digital, v_tank_peak, v_tank_pp_mV

# -----------------------------------------------------------------------------
# 5. MÁQUINA DE ESTADOS Y DECODIFICADOR ESP32 (FIRMWARE RX)
# -----------------------------------------------------------------------------
class SimuladorManchesterESP32:
    """Implementa la lógica exacta de firmware_rx/manchester_rx.cpp."""
    
    def __init__(self):
        self.state = "UNSYNCED"
        self.sync_count = 0
        self.last_edge_us = 0
        self.bits = []
        
        # Umbrales temporales (us)
        self.T_HALF_MIN = 1400
        self.T_HALF_MAX = 2600
        self.T_BIT_MIN = 2800
        self.T_BIT_MAX = 5200
        self.T_TIMEOUT = 14000
        self.T_GLITCH_MIN = 600

    def procesar_flanco(self, now_us, pin_level):
        if self.last_edge_us == 0:
            self.last_edge_us = now_us
            return

        dt = now_us - self.last_edge_us

        # 1. Timeout por silencio
        if dt > self.T_TIMEOUT:
            self.state = "UNSYNCED"
            self.sync_count = 0
            self.last_edge_us = now_us
            return

        # 2. Filtro anti-glitch
        if dt < self.T_GLITCH_MIN:
            return

        self.last_edge_us = now_us

        is_half = (self.T_HALF_MIN <= dt <= self.T_HALF_MAX)
        is_full = (self.T_BIT_MIN <= dt <= self.T_BIT_MAX)

        decoded_bit = 0 if pin_level else 1

        if self.state == "UNSYNCED":
            if is_full:
                self.sync_count += 1
                if self.sync_count >= 2:
                    self.state = "EXPECT_ANY"
                    self.bits.append(decoded_bit)
            else:
                self.sync_count = 0

        elif self.state == "EXPECT_ANY":
            if is_full:
                self.bits.append(decoded_bit)
                self.state = "EXPECT_ANY"
            elif is_half:
                self.state = "EXPECT_MID"
            else:
                self.state = "UNSYNCED"
                self.sync_count = 0

        elif self.state == "EXPECT_MID":
            if is_half:
                self.bits.append(decoded_bit)
                self.state = "EXPECT_ANY"
            else:
                self.state = "UNSYNCED"
                self.sync_count = 0

def decodificar_paquete_bit_a_bit(bits):
    """
    Simula packet_parser.cpp: Registro de desplazamiento para búsqueda de SFD (0x7E)
    y lectura alineada de longitud, carga útil y checksum CRC-8.
    """
    shift_reg = 0
    state = "SEARCH_SFD"
    bit_acc = 0
    bits_collected = 0
    payload = []
    length = 0
    received_crc = 0
    success = False

    for bit in bits:
        if state == "SEARCH_SFD":
            shift_reg = ((shift_reg << 1) | bit) & 0xFF
            if shift_reg == 0x7E:
                state = "READ_LEN"
                bit_acc = 0
                bits_collected = 0
        elif state == "READ_LEN":
            bit_acc = (bit_acc << 1) | bit
            bits_collected += 1
            if bits_collected == 8:
                length = bit_acc
                state = "READ_PAYLOAD"
                bit_acc = 0
                bits_collected = 0
                payload = []
        elif state == "READ_PAYLOAD":
            bit_acc = (bit_acc << 1) | bit
            bits_collected += 1
            if bits_collected == 8:
                payload.append(bit_acc)
                bit_acc = 0
                bits_collected = 0
                if len(payload) >= length:
                    state = "READ_CRC"
        elif state == "READ_CRC":
            bit_acc = (bit_acc << 1) | bit
            bits_collected += 1
            if bits_collected == 8:
                received_crc = bit_acc
                success = True
                break

    if success:
        calc_crc = calc_crc8([length] + payload)
        valid = (received_crc == calc_crc)
        payload_str = "".join([chr(b) if 32 <= b <= 126 else '?' for b in payload])
        return {
            "exito": True,
            "valido": valid,
            "payload": payload_str,
            "crc_rx": hex(received_crc),
            "crc_calc": hex(calc_crc)
        }
    return {
        "exito": False,
        "valido": False,
        "payload": "",
        "crc_rx": "None",
        "crc_calc": "None"
    }

def extraer_flancos_y_decodificar(t, v_digital):
    """Detecta transiciones analógicas y alimenta la máquina de estados del ESP32."""
    decodificador = SimuladorManchesterESP32()
    
    flancos = np.diff((v_digital > 1.65).astype(int))
    indices_flancos = np.where(flancos != 0)[0]
    
    for idx in indices_flancos:
        t_us = int(t[idx] * 1e6)
        nivel_alto = bool(v_digital[idx + 1] > 1.65)
        decodificador.procesar_flanco(t_us, nivel_alto)
        
    resultado_parser = decodificar_paquete_bit_a_bit(decodificador.bits)
    resultado_parser["total_flancos"] = len(indices_flancos)
    resultado_parser["bits"] = len(decodificador.bits)
    return resultado_parser

# -----------------------------------------------------------------------------
# 6. PROGRAMA PRINCIPAL
# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description="Simulador del Enlace Inductivo de Comunicación a 125 kHz")
    parser.add_argument("--distancia", type=float, default=4.0, help="Distancia axial en centímetros (defecto: 4.0 cm)")
    parser.add_argument("--mensaje", type=str, default="CAMPOS", help="Mensaje de prueba a transmitir (defecto: 'CAMPOS')")
    parser.add_argument("--graficar", action="store_true", help="Guardar oscilograma de simulación en figuras/")
    args = parser.parse_args()

    print("=" * 72)
    print("  SIMULADOR FÍSICO-DIGITAL DE ENLACE INDUCTIVO A 125 kHz (OOK / MANCHESTER)")
    print("=" * 72)
    print(f"Texto a transmitir:          '{args.mensaje}'")
    print(f"Distancia axial seleccionada: {args.distancia:.1f} cm")
    
    # 1. Transmisor Digital y RF
    trama = construir_trama(args.mensaje)
    bits = bytes_a_bits(trama)
    manchester = codificar_manchester(bits)
    t, env_tx, s_tx = generar_senal_tx(manchester)
    
    print(f"Bytes de trama estructurada:  {[hex(b) for b in trama]}")
    print(f"Bits Manchester generados:    {len(bits)} bits ({len(manchester)} semi-períodos)")
    print(f"Duración total del paquete:   {t[-1]*1e3:.1f} ms")

    # 2. Canal Físico y Cadena Analógica RX
    s_tank, s_amp, s_env, s_dig, v_pk, v_pp_mV = simular_etapa_rx(t, s_tx, args.distancia)
    print(f"\n[CANAL FÍSICO Y CADENA ANALÓGICA RX]")
    print(f"Tensión inducida en tanque RX: {v_pk*1e3:.2f} mV pico ({v_pp_mV:.1f} mVpp)")
    print(f"Tensión pico tras MCP6002:     {min(v_pk * GAIN_ACTIVE_AC, 1.60):.2f} V (sobre masa virtual)")
    print(f"Umbral de rectificación BAT85: {V_DIODE_F:.2f} V")
    print(f"Umbral de decisión LM393:     {V_TH_COMP:.2f} V")

    # 3. Decodificación por Firmware
    res = extraer_flancos_y_decodificar(t, s_dig)
    print(f"\n[DECODIFICACIÓN DIGITAL EN ESP32]")
    print(f"Transiciones GPIO detectadas:  {res['total_flancos']}")
    print(f"Bits conmutados decodificados: {res['bits']}")
    
    if res["exito"] and res["valido"]:
        print(f"Estado de Trama:               ¡VÁLIDO! (Integridad CRC-8 verificada)")
        print(f"Mensaje recuperado con éxito:  '{res['payload']}'")
        print(f"CRC Recibido: {res['crc_rx']} | CRC Esperado: {res['crc_calc']}")
    elif res["exito"] and not res["valido"]:
        print(f"Estado de Trama:               CORRUPTO (Discrepancia en checksum CRC-8)")
        print(f"CRC Recibido: {res['crc_rx']} | CRC Calculado: {res['crc_calc']}")
    else:
        print(f"Estado de Trama:               CORTE DE ENLACE / SIN DETECCIÓN")
        print(f"Diagnóstico: A {args.distancia:.1f} cm la señal es insuficiente para vencer la")
        print(f"             barrera Schottky ({V_DIODE_F} V) y el umbral del comparador ({V_TH_COMP} V).")

    # 4. Generación opcional de gráficos
    if args.graficar:
        import matplotlib
        matplotlib.use('Agg')
        import matplotlib.pyplot as plt
        
        output_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "figuras")
        os.makedirs(output_dir, exist_ok=True)
        out_path = os.path.join(output_dir, "simulacion_enlace_completo.png")
        
        fig, axs = plt.subplots(4, 1, figsize=(10, 8), sharex=True, dpi=200)
        t_ms = t * 1e3
        lim_ms = min(t_ms[-1], 36.0) # Primeros 36 ms (Preámbulo + SFD + Len)
        mask = t_ms <= lim_ms
        
        axs[0].plot(t_ms[mask], env_tx[mask], 'tab:blue', linewidth=1.5)
        axs[0].set_ylabel("Datos [V]")
        axs[0].set_title("1. Señal Moduladora en Transmisor (ESP32 GPIO18 Manchester @ 250 bps)")
        axs[0].grid(True, linestyle="--", alpha=0.5)
        
        axs[1].plot(t_ms[mask], s_tx[mask], 'tab:purple', linewidth=0.8)
        axs[1].set_ylabel("Portadora [V]")
        axs[1].set_title("2. Ráfagas de Portadora a 125 kHz en Tanque Serie TX")
        axs[1].grid(True, linestyle="--", alpha=0.5)
        
        axs[2].plot(t_ms[mask], s_amp[mask] - V_BIAS_RX, 'tab:gray', alpha=0.4, label="MCP6002 (sin DC)")
        axs[2].plot(t_ms[mask], s_env[mask], 'tab:red', linewidth=1.8, label=r"Envolvente BAT85 ($R_7 C_6$)")
        axs[2].axhline(V_TH_COMP, color='black', linestyle='--', label=f"Umbral LM393 ({V_TH_COMP}V)")
        axs[2].set_ylabel("Tensión [V]")
        axs[2].set_title(f"3. Detección de Envolvente en Receptor (Distancia z = {args.distancia:.1f} cm)")
        axs[2].legend(loc="upper right", fontsize=8)
        axs[2].grid(True, linestyle="--", alpha=0.5)
        
        axs[3].plot(t_ms[mask], s_dig[mask], 'tab:green', linewidth=1.5)
        axs[3].set_ylabel("Digital [V]")
        axs[3].set_xlabel("Tiempo [ms]")
        axs[3].set_title("4. Salida del Comparador hacia ESP32 GPIO19")
        axs[3].grid(True, linestyle="--", alpha=0.5)
        
        plt.tight_layout()
        plt.savefig(out_path)
        plt.close(fig)
        print(f"\n[GRÁFICO GENERADO]: {out_path}")

    print("=" * 72)

if __name__ == "__main__":
    main()
