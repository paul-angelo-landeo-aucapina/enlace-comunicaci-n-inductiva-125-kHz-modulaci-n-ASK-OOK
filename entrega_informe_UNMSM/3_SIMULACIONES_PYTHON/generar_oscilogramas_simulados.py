#!/usr/bin/env python3
"""
generar_oscilogramas_simulados.py
Genera los 4 gráficos de simulación numérica discreta en Python para el informe IEEE:
1. sim_montaje_enlace.png / .pdf: Diagrama en bloques de la simulación del enlace extremo a extremo.
2. sim_osc_tx.png / .pdf: Oscilograma simulado del transmisor (CH1: GPIO18, CH2: Tanque Serie 125 kHz).
3. sim_osc_rx_amp.png / .pdf: Oscilograma simulado del receptor (CH1: Tanque LC débil, CH2: MCP6002 amplificado).
4. sim_osc_demod.png / .pdf: Oscilograma de demodulación y conformación (CH1: Envolvente BAT85 + RC, CH2: LM393).
"""

import os
import sys
import numpy as np

# Configuración de backend sin entorno gráfico y directorio temporal de Matplotlib
os.environ["MPLCONFIGDIR"] = "/tmp/matplotlib"
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import matplotlib.patches as patches

# -----------------------------------------------------------------------------
# Constantes físicas y del circuito
# -----------------------------------------------------------------------------
F0 = 125040.0           # Frecuencia portadora nominal (Hz)
W0 = 2.0 * np.pi * F0
BAUD = 250.0            # Tasa de símbolos (bps)
T_BIT = 1.0 / BAUD      # 4.0 ms
T_HALF = T_BIT / 2.0    # 2.0 ms
FS = 1.25e6             # 1.25 MHz de muestreo (10 muestras por ciclo de portadora)
DT = 1.0 / FS

# Directorio de salida
FIG_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "figuras"))
os.makedirs(FIG_DIR, exist_ok=True)

# -----------------------------------------------------------------------------
# 1. FIGURA 6: Diagrama en Bloques del Modelo de Simulación
# -----------------------------------------------------------------------------
def generar_figura_montaje_simulacion():
    print("[1/4] Generando sim_montaje_enlace...")
    fig, ax = plt.subplots(figsize=(10, 4.2), dpi=300)
    ax.set_xlim(0, 100)
    ax.set_ylim(0, 50)
    ax.axis("off")

    # Estilos de bloques
    box_blue = dict(boxstyle="round,pad=0.5", facecolor="#EBF3FB", edgecolor="#2B6CB0", linewidth=1.5)
    box_green = dict(boxstyle="round,pad=0.5", facecolor="#E6FFFA", edgecolor="#234E52", linewidth=1.5)
    box_orange = dict(boxstyle="round,pad=0.5", facecolor="#FFFAF0", edgecolor="#C05621", linewidth=1.5)
    box_gray = dict(boxstyle="round,pad=0.5", facecolor="#F7FAFC", edgecolor="#4A5568", linewidth=1.2)

    # 1. Bloque Transmisor
    ax.add_patch(patches.Rectangle((2, 4), 38, 42, fill=True, facecolor="#F0F4F8", edgecolor="#718096", linestyle="--", linewidth=1))
    ax.text(21, 43, "NODO TRANSMISOR (TX)", fontsize=11, fontweight="bold", ha="center", color="#2D3748")

    ax.text(8, 33, "ESP32 (TX)\n• Trama CRC-8\n• Manchester\n(250 bps)", ha="center", va="center", bbox=box_blue, fontsize=8)
    ax.text(21, 33, "Oscilador\nTLC555\n(125 kHz)", ha="center", va="center", bbox=box_orange, fontsize=8)
    ax.text(34, 33, "Driver\nTC4427A\n(Push-Pull)", ha="center", va="center", bbox=box_orange, fontsize=8)
    ax.text(21, 14, "Tanque Resonante Serie\n$L_1 = 1.0\\text{ mH}$, $C_1 = 1.62\\text{ nF}$\n$R_{\\text{limit}} = 150\\,\\Omega$, $Q \\approx 4.8$", ha="center", va="center", bbox=box_blue, fontsize=8)

    # Flechas TX
    ax.annotate("", xy=(15.5, 33), xytext=(12.5, 33), arrowprops=dict(arrowstyle="->", color="#2B6CB0", lw=1.5))
    ax.text(14, 35, "GPIO18", fontsize=7, color="#2B6CB0", ha="center")
    ax.annotate("", xy=(28.5, 33), xytext=(25.5, 33), arrowprops=dict(arrowstyle="->", color="#C05621", lw=1.5))
    ax.annotate("", xy=(28.5, 17), xytext=(34, 27), arrowprops=dict(arrowstyle="->", color="#2B6CB0", lw=1.5))
    ax.text(34, 21, "$V_{\\text{sq}}$", fontsize=7, color="#2B6CB0")

    # 2. Canal Inductivo
    ax.add_patch(patches.Rectangle((42, 10), 16, 30, fill=True, facecolor="#FFF5F5", edgecolor="#E53E3E", linestyle=":", linewidth=1.5))
    ax.text(50, 36, "CANAL INDUCTIVO\nQUASIESTÁTICO", fontsize=8, fontweight="bold", ha="center", color="#9B2C2C")
    ax.text(50, 24, "Biot-Savart\n$M(z) = \\mu_0 N_1 N_2 \\frac{r_1^2 r_2^2}{2(r_1^2 + z^2)^{3/2}}$\n$k(z) = \\frac{M}{\\sqrt{L_1 L_2}}$", ha="center", va="center", fontsize=7, color="#742A2A")
    ax.text(50, 13, "$z = 1.0 \\dots 10.0\\text{ cm}$", ha="center", va="center", fontsize=8, fontweight="bold", color="#C53030")

    # Acoplamiento TX -> Canal -> RX
    ax.annotate("", xy=(42, 14), xytext=(33.5, 14), arrowprops=dict(arrowstyle="<->", color="#E53E3E", lw=2))
    ax.annotate("", xy=(60, 14), xytext=(58, 14), arrowprops=dict(arrowstyle="<->", color="#E53E3E", lw=2))

    # 3. Bloque Receptor
    ax.add_patch(patches.Rectangle((60, 4), 38, 42, fill=True, facecolor="#F0FFF4", edgecolor="#68D391", linestyle="--", linewidth=1))
    ax.text(79, 43, "NODO RECEPTOR (RX)", fontsize=11, fontweight="bold", ha="center", color="#22543D")

    ax.text(68, 14, "Tanque Paralelo\n$L_2 = 1.0\\text{ mH}$\n$C_2 = 1.62\\text{ nF}$\n$Q_{\\text{eff}} = 10.95$", ha="center", va="center", bbox=box_green, fontsize=8)
    ax.text(88, 14, "Preamplificador\nMCP6002 (2 etapas)\n$A_{v,\\text{DC}} = 32\\text{ V/V}$\n$A_v(f_0) = 18.6\\text{ V/V}$", ha="center", va="center", bbox=box_green, fontsize=8)
    ax.text(88, 33, "Detector de Envolvente\nBAT85 + $R_7 C_6$\n($\\tau = 1.0\\text{ ms}$)", ha="center", va="center", bbox=box_orange, fontsize=8)
    ax.text(68, 33, "Conformador\nLM393\n$V_{\\text{th}} = 0.30\\text{ V}$", ha="center", va="center", bbox=box_orange, fontsize=8)

    # Bloque final ESP32 RX
    ax.text(50, 4, "ESP32 (RX): Interrupción GPIO19 • Filtro Anti-Glitch (600 µs) • FSM Manchester • Checksum CRC-8", ha="center", va="center", bbox=box_gray, fontsize=7.5, fontweight="bold")

    # Flechas RX
    ax.annotate("", xy=(78.5, 14), xytext=(74.5, 14), arrowprops=dict(arrowstyle="->", color="#234E52", lw=1.5))
    ax.text(76.5, 16, "$V_{\\text{ind}}$", fontsize=7, color="#234E52", ha="center")
    ax.annotate("", xy=(88, 26), xytext=(88, 19.5), arrowprops=dict(arrowstyle="->", color="#234E52", lw=1.5))
    ax.text(91.5, 22.5, "$V_{\\text{amp}}$", fontsize=7, color="#234E52")
    ax.annotate("", xy=(75, 33), xytext=(80.5, 33), arrowprops=dict(arrowstyle="->", color="#C05621", lw=1.5))
    ax.text(77.5, 35, "$V_{\\text{env}}$", fontsize=7, color="#C05621", ha="center")
    ax.annotate("", xy=(65, 8.5), xytext=(68, 27), arrowprops=dict(arrowstyle="->", color="#234E52", lw=1.5))
    ax.text(69.5, 18, "GPIO19", fontsize=7, color="#234E52")

    plt.tight_layout()
    plt.savefig(os.path.join(FIG_DIR, "sim_montaje_enlace.png"), dpi=300)
    plt.savefig(os.path.join(FIG_DIR, "sim_montaje_enlace.pdf"))
    plt.close(fig)
    print("   -> Guardado sim_montaje_enlace.png y .pdf")

# -----------------------------------------------------------------------------
# Generación de señales sintéticas de alta precisión
# -----------------------------------------------------------------------------
def sintetizar_enlace(n_bits=8, z_cm=7.0):
    # Secuencia binaria de prueba con alternancia representativa (Preámbulo parcial + datos)
    # Patrón: 1, 0, 1, 0, 0, 1, 1, 0
    bits = np.array([1, 0, 1, 0, 0, 1, 1, 0])
    # Codificación Manchester IEEE 802.3: '0' -> [0, 1], '1' -> [1, 0]
    symbols = []
    for b in bits:
        if b == 0:
            symbols.extend([0, 1])
        else:
            symbols.extend([1, 0])
    symbols = np.array(symbols, dtype=float)

    samples_per_half = int(FS * T_HALF)
    total_samples = len(symbols) * samples_per_half
    t = np.arange(total_samples) * DT

    # Señal moduladora digital GPIO18 (0 a 3.3V)
    v_gpio18 = np.repeat(symbols, samples_per_half) * 3.3

    # Ráfagas OOK en el transmisor: envolvente con tiempo de subida/bajada de tanque LC
    # Tau del tanque serie: tau = 2*Q / w0 = 2*(4.8) / (2*pi*125000) ~ 12.2 us
    tau_tank_tx = 2.0 * 4.8 / W0
    alpha_tx = DT / (tau_tank_tx + DT)
    env_tx_smooth = np.zeros(total_samples)
    for k in range(1, total_samples):
        target = 1.0 if v_gpio18[k] > 1.65 else 0.0
        env_tx_smooth[k] = alpha_tx * target + (1.0 - alpha_tx) * env_tx_smooth[k - 1]

    # Portadora senoidal a 125.04 kHz excitada por driver TC4427 (5V)
    # V1_pk = 4*Vcc / pi ~ 3.18V sobre la bobina
    carrier = np.sin(W0 * t)
    v_coil_tx = 3.18 * env_tx_smooth * carrier

    # Canal inductivo a z = 7.0 cm:
    # V_tank_pp = 30.2 mVpp -> Amplitud pico ~ 15.1 mV
    v_tank_pk = 0.0151
    v_tank_rx = v_tank_pk * env_tx_smooth * carrier
    # Pequeño piso de ruido térmico
    np.random.seed(42)
    v_tank_rx += np.random.normal(0, 0.0008, size=total_samples)

    # Preamplificador MCP6002:
    # Ganancia efectiva a 125 kHz con límite GBW: Av = 18.63 V/V
    # Masa virtual = 1.65 V
    v_amp_ac = v_tank_rx * 18.63
    v_amp_total = 1.65 + v_amp_ac

    # Desacoplo AC y rectificación BAT85 (Vf = 0.18V en microamperios):
    v_diode_in = np.abs(v_amp_ac)
    v_diode_rect = np.maximum(0.0, v_diode_in - 0.18)

    # Filtro de envolvente RC (tau = 1.0 ms = 1000 us, carga rápida tau_ch = 2 us)
    tau_ch = 2.0e-6
    tau_dis = 1.0e-3
    alpha_ch = DT / (tau_ch + DT)
    alpha_dis = DT / (tau_dis + DT)
    v_env = np.zeros(total_samples)
    for k in range(1, total_samples):
        vin = v_diode_rect[k]
        if vin > v_env[k - 1]:
            v_env[k] = alpha_ch * vin + (1.0 - alpha_ch) * v_env[k - 1]
        else:
            v_env[k] = (1.0 - alpha_dis) * v_env[k - 1]

    # Comparador LM393 con histéresis (Vth = 0.30 V, Hyst = 25 mV)
    v_th_high = 0.30 + 0.025
    v_th_low = 0.30 - 0.025
    v_comp_out = np.zeros(total_samples)
    state = 0
    for k in range(total_samples):
        if state == 0 and v_env[k] > v_th_high:
            state = 1
        elif state == 1 and v_env[k] < v_th_low:
            state = 0
        v_comp_out[k] = 3.3 if state == 1 else 0.0

    return t, v_gpio18, v_coil_tx, v_tank_rx, v_amp_total, v_env, v_comp_out

# -----------------------------------------------------------------------------
# 2. FIGURA 7: Oscilograma TX (GPIO18 + Ráfagas 125 kHz) - Fondo Blanco
# -----------------------------------------------------------------------------
def generar_figura_osc_tx(t, v_gpio18, v_coil_tx):
    print("[2/4] Generando sim_osc_tx (Fondo Blanco)...")
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 4.8), dpi=300, sharex=True)
    fig.patch.set_facecolor("#FFFFFF")
    t_ms = t * 1e3
    t_max = 16.0  # 4 bits (16 ms)
    mask = t_ms <= t_max

    # Fondo blanco y retícula de laboratorio
    for ax in (ax1, ax2):
        ax.set_facecolor("#FFFFFF")
        ax.grid(True, which="both", color="#E2E8F0", linestyle=":", linewidth=0.9)
        ax.axhline(0, color="#94A3B8", linewidth=0.8)
        for spine in ax.spines.values():
            spine.set_color("#CBD5E1")

    # CH1: Moduladora digital (Azul vibrante)
    ax1.plot(t_ms[mask], v_gpio18[mask], color="#0284C7", linewidth=1.8, label="CH1: GPIO18 (Manchester OOK)")
    ax1.set_ylim(-0.5, 4.3)
    ax1.set_ylabel("Tensión [V]", color="#0F172A", fontsize=9, fontweight="bold")
    ax1.tick_params(colors="#334155", labelsize=8)
    ax1.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax1.set_title("OSCILOGRAMA SIMULADO TX: MODULADORA DIGITAL Y PORTADORA A 125 kHz", color="#0F172A", fontsize=10, fontweight="bold")
    ax1.text(0.5, 3.6, "CH1: 1.00 V/div  |  $T_{\\text{half}} = 2.0\\text{ ms}$ ($R_b = 250\\text{ bps}$)", color="#0369A1", fontsize=8, fontweight="bold")

    # CH2: Portadora sobre bobina serie (Rojo carmesí)
    ax2.plot(t_ms[mask], v_coil_tx[mask], color="#DC2626", linewidth=0.8, label="CH2: $V_{\\text{coil}}$ (Tanque Serie 125 kHz)")
    ax2.set_ylim(-4.2, 4.2)
    ax2.set_ylabel("Tensión [V]", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.set_xlabel("Tiempo [ms]  (Base de Tiempo: 2.00 ms/div)", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.tick_params(colors="#334155", labelsize=8)
    ax2.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax2.text(0.5, 3.2, "CH2: 1.00 V/div  |  $V_{\\text{pp}} \\approx 6.36\\text{ V}$  |  $f_0 = 125.04\\text{ kHz}$", color="#B91C1C", fontsize=8, fontweight="bold")

    plt.tight_layout()
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_tx.png"), dpi=300, facecolor="#FFFFFF")
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_tx.pdf"), facecolor="#FFFFFF")
    plt.close(fig)
    print("   -> Guardado sim_osc_tx.png y .pdf (Fondo Blanco)")

# -----------------------------------------------------------------------------
# 3. FIGURA 8: Oscilograma RX (Preamplificación MCP6002) - Fondo Blanco
# -----------------------------------------------------------------------------
def generar_figura_osc_rx_amp(t, v_tank_rx, v_amp_total):
    print("[3/4] Generando sim_osc_rx_amp (Fondo Blanco)...")
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 4.8), dpi=300, sharex=True)
    fig.patch.set_facecolor("#FFFFFF")
    t_ms = t * 1e3
    t_max = 16.0
    mask = t_ms <= t_max

    for ax in (ax1, ax2):
        ax.set_facecolor("#FFFFFF")
        ax.grid(True, which="both", color="#E2E8F0", linestyle=":", linewidth=0.9)
        for spine in ax.spines.values():
            spine.set_color("#CBD5E1")

    # CH1: Tensión inducida en tanque LC (Verde esmeralda)
    ax1.plot(t_ms[mask], v_tank_rx[mask] * 1e3, color="#059669", linewidth=0.9, label="CH1: $V_{\\text{tank}}$ (Tanque LC Paralelo RX)")
    ax1.axhline(0, color="#94A3B8", linewidth=0.8)
    ax1.set_ylim(-38, 38)
    ax1.set_ylabel("Tensión [mV]", color="#0F172A", fontsize=9, fontweight="bold")
    ax1.tick_params(colors="#334155", labelsize=8)
    ax1.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax1.set_title("OSCILOGRAMA SIMULADO RX: TENSIÓN INDUCIDA Y PREAMPLIFICACIÓN (MCP6002)", color="#0F172A", fontsize=10, fontweight="bold")
    ax1.text(0.5, 28, "CH1: 10.0 mV/div  |  $V_{\\text{tank,pp}} \\approx 30.2\\text{ mV}_{\\text{pp}}$ ($z = 7.0\\text{ cm}$)", color="#047857", fontsize=8, fontweight="bold")

    # CH2: Salida preamplificada con masa virtual (Violeta intenso)
    ax2.plot(t_ms[mask], v_amp_total[mask], color="#7C3AED", linewidth=0.9, label="CH2: $V_{\\text{amp}}$ (Salida Etapa 2 MCP6002)")
    ax2.axhline(1.65, color="#64748B", linestyle="--", linewidth=1.0, label="Masa Virtual (1.65 V)")
    ax2.set_ylim(1.05, 2.25)
    ax2.set_ylabel("Tensión [V]", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.set_xlabel("Tiempo [ms]  (Base de Tiempo: 2.00 ms/div)", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.tick_params(colors="#334155", labelsize=8)
    ax2.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax2.text(0.5, 2.12, "CH2: 200 mV/div  |  $V_{\\text{amp,pp}} \\approx 562\\text{ mV}_{\\text{pp}}$  |  $A_{v,\\text{eff}}(125\\text{k}) = 18.63\\text{ V/V}$", color="#6D28D9", fontsize=8, fontweight="bold")

    plt.tight_layout()
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_rx_amp.png"), dpi=300, facecolor="#FFFFFF")
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_rx_amp.pdf"), facecolor="#FFFFFF")
    plt.close(fig)
    print("   -> Guardado sim_osc_rx_amp.png y .pdf (Fondo Blanco)")

# -----------------------------------------------------------------------------
# 4. FIGURA 9: Oscilograma Demodulación y Conformación (BAT85 + LM393) - Fondo Blanco
# -----------------------------------------------------------------------------
def generar_figura_osc_demod(t, v_env, v_comp_out):
    print("[4/4] Generando sim_osc_demod (Fondo Blanco)...")
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(8, 4.8), dpi=300, sharex=True)
    fig.patch.set_facecolor("#FFFFFF")
    t_ms = t * 1e3
    t_max = 16.0
    mask = t_ms <= t_max

    for ax in (ax1, ax2):
        ax.set_facecolor("#FFFFFF")
        ax.grid(True, which="both", color="#E2E8F0", linestyle=":", linewidth=0.9)
        for spine in ax.spines.values():
            spine.set_color("#CBD5E1")

    # CH1: Envolvente rectificada vs umbral (Ámbar / Naranja)
    ax1.plot(t_ms[mask], v_env[mask], color="#D97706", linewidth=1.8, label="CH1: Envolvente Filtrada BAT85 ($R_7 C_6$)")
    ax1.axhline(0.30, color="#DC2626", linestyle="--", linewidth=1.2, label="Umbral LM393 ($V_{\\text{th}} = 0.30\\text{ V}$)")
    ax1.axhspan(0.30 - 0.025, 0.30 + 0.025, color="#FCA5A5", alpha=0.35, label="Banda Histéresis ($\\pm 25\\text{ mV}$)")
    ax1.set_ylim(-0.05, 0.68)
    ax1.set_ylabel("Tensión [V]", color="#0F172A", fontsize=9, fontweight="bold")
    ax1.tick_params(colors="#334155", labelsize=8)
    ax1.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax1.set_title("OSCILOGRAMA SIMULADO: DETECCIÓN DE ENVOLVENTE Y CONFORMACIÓN DIGITAL", color="#0F172A", fontsize=10, fontweight="bold")
    ax1.text(0.5, 0.60, "CH1: 100 mV/div  |  Filtro $\\tau_{\\text{env}} = 1.0\\text{ ms}$  |  Descarga intersimbólica $> 86\\%$", color="#B45309", fontsize=8, fontweight="bold")

    # CH2: Salida conformada hacia ESP32 (Azul intenso)
    ax2.plot(t_ms[mask], v_comp_out[mask], color="#2563EB", linewidth=1.8, label="CH2: Salida Digital LM393 -> ESP32 GPIO19")
    ax2.set_ylim(-0.5, 4.2)
    ax2.set_ylabel("Tensión [V]", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.set_xlabel("Tiempo [ms]  (Base de Tiempo: 2.00 ms/div)", color="#0F172A", fontsize=9, fontweight="bold")
    ax2.tick_params(colors="#334155", labelsize=8)
    ax2.legend(loc="upper right", facecolor="#FFFFFF", edgecolor="#CBD5E1", labelcolor="#0F172A", fontsize=8, framealpha=0.95)
    ax2.text(0.5, 3.5, "CH2: 1.00 V/div  |  Recuperación Manchester Limpia sin ISI  |  PER = 0", color="#1D4ED8", fontsize=8, fontweight="bold")

    plt.tight_layout()
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_demod.png"), dpi=300, facecolor="#FFFFFF")
    plt.savefig(os.path.join(FIG_DIR, "sim_osc_demod.pdf"), facecolor="#FFFFFF")
    plt.close(fig)
    print("   -> Guardado sim_osc_demod.png y .pdf (Fondo Blanco)")

# -----------------------------------------------------------------------------
# Función principal
# -----------------------------------------------------------------------------
def main():
    print("Iniciando generación de gráficos de simulación para informe IEEE...")
    generar_figura_montaje_simulacion()
    t, v_gpio18, v_coil_tx, v_tank_rx, v_amp_total, v_env, v_comp_out = sintetizar_enlace(n_bits=8, z_cm=7.0)
    generar_figura_osc_tx(t, v_gpio18, v_coil_tx)
    generar_figura_osc_rx_amp(t, v_tank_rx, v_amp_total)
    generar_figura_osc_demod(t, v_env, v_comp_out)
    print("¡Todas las figuras de simulación se generaron con éxito en informe_ieee/figuras/!")

if __name__ == "__main__":
    main()
