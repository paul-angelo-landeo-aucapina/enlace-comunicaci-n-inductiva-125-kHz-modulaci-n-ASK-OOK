#!/usr/bin/env python3
"""
crear_placeholders.py
Genera imágenes placeholder profesionales con etiquetas para las figuras experimentales
que el usuario reemplazará posteriormente con fotos reales y capturas de osciloscopio.
"""

import os
import matplotlib.pyplot as plt

FIG_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "figuras")
os.makedirs(FIG_DIR, exist_ok=True)

placeholders = [
    ("placeholder_montaje.png", "FOTOGRAFÍA DEL MONTAJE EXPERIMENTAL\n[Banco de pruebas, bobinas coaxiales, ESP32 TX/RX y fuentes]", "Cargue aquí foto del banco de pruebas"),
    ("placeholder_osc_tx.png", "OSCILOGRAMA TX (125 kHz)\nCH1: Ráfaga de portadora modulada ASK/OOK\nCH2: Señal de control GPIO18 (ESP32)", "Cargue aquí captura de osciloscopio TX"),
    ("placeholder_osc_rx_amp.png", "OSCILOGRAMA RX (PREAMPLIFICACIÓN)\nCH1: Tensión inducida en tanque LC (mVpp)\nCH2: Salida preamplificada MCP6002 (~32x)", "Cargue aquí captura de osciloscopio RX"),
    ("placeholder_osc_demod.png", "OSCILOGRAMA DEMODULACIÓN Y CONFORMACIÓN\nCH1: Salida detector de envolvente (BAT85)\nCH2: Pulso digital cuadrado (LM393 -> ESP32 GPIO19)", "Cargue aquí captura de demodulación")
]

for filename, title, subtitle in placeholders:
    filepath = os.path.join(FIG_DIR, filename)
    fig, ax = plt.subplots(figsize=(5, 3), dpi=200)
    ax.set_facecolor("#f4f6f8")
    ax.plot([0, 1], [0, 1], color="#cbd5e1", linestyle="--")
    ax.plot([0, 1], [1, 0], color="#cbd5e1", linestyle="--")
    ax.text(0.5, 0.6, title, ha="center", va="center", fontsize=9, fontweight="bold", color="#1e293b", family="sans-serif")
    ax.text(0.5, 0.35, f"({subtitle})", ha="center", va="center", fontsize=8, color="#64748b", family="sans-serif")
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.set_xticks([])
    ax.set_yticks([])
    for spine in ax.spines.values():
        spine.set_color("#94a3b8")
        spine.set_linestyle(":")
    plt.tight_layout()
    fig.savefig(filepath)
    plt.close(fig)
    print(f"Creado placeholder: {filename}")
