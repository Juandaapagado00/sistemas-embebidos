# -*- coding: utf-8 -*-
"""
HMI en tiempo real - Actividad de Sistemas Embebidos (ATmega1284P + Proteus)

Lee por puerto serie la trama que envia el microcontrolador:

    A1:922,A2:625,D1:0,D2:0

y la muestra como voltajes, indicadores y una grafica en tiempo real.

Requisitos:
    pip install dearpygui pyserial

Uso:
    1. Inicia la simulacion en Proteus.
    2. Cierra Termite (un puerto COM solo lo puede abrir un programa a la vez).
    3. Ejecuta:  python hmi_serial.py
    4. Elige el puerto (COM3) y pulsa "Conectar".
"""

import os
import re
import threading
import time
import unicodedata
from collections import deque
from queue import Empty, Queue

import dearpygui.dearpygui as dpg
import serial
import serial.tools.list_ports

# ----------------------------------------------------------------------
# Configuracion (lo unico que normalmente tendrias que tocar)
# ----------------------------------------------------------------------
PUERTO_POR_DEFECTO = "COM3"   # Extremo del par virtual donde escucha la HMI
BAUDIOS_POR_DEFECTO = 9600
VREF = 5.0                    # Referencia del ADC (AVCC = 5 V)
ADC_MAX = 1023                # ADC de 10 bits: valores de 0 a 1023
VENTANA_S = 60                # Segundos visibles en la grafica
MAX_PUNTOS = 1200             # Puntos que se guardan en memoria
SIN_DATOS_S = 2.0             # Segundos sin tramas antes de avisar

# Trama esperada: A1:922,A2:625,D1:0,D2:0
PATRON_TRAMA = re.compile(r"^A1:(\d{1,4}),A2:(\d{1,4}),D1:([01]),D2:([01])$")

# Paleta: gris azulado de banco de laboratorio, trazos cian y ambar
COLOR_A1 = (76, 201, 240)
COLOR_A2 = (255, 180, 84)
COLOR_OK = (86, 211, 130)
COLOR_ERROR = (240, 98, 98)
COLOR_AVISO = (245, 200, 80)
COLOR_TENUE = (140, 150, 166)
COLOR_LED_ON = (86, 211, 130)
COLOR_LED_OFF = (58, 64, 76)


def parsear_trama(linea):
    """Devuelve (a1, a2, d1, d2) o None si la linea no es una trama valida."""
    m = PATRON_TRAMA.match(linea.strip())
    if not m:
        return None
    a1, a2, d1, d2 = (int(g) for g in m.groups())
    if a1 > ADC_MAX or a2 > ADC_MAX:
        return None
    return a1, a2, d1, d2


# ----------------------------------------------------------------------
# Lectura del puerto serie en un hilo aparte (para no congelar la ventana)
# ----------------------------------------------------------------------
class Lector(threading.Thread):
    def __init__(self, puerto, baudios, cola):
        super().__init__(daemon=True)
        self.puerto = puerto
        self.baudios = baudios
        self.cola = cola
        self._parar = threading.Event()

    def detener(self):
        self._parar.set()

    def run(self):
        try:
            ser = serial.serial_for_url(self.puerto, baudrate=self.baudios, timeout=0.3)
        except (serial.SerialException, OSError, ValueError) as e:
            self.cola.put((self, "error", str(e)))
            return

        self.cola.put((self, "conectado", None))
        buffer = bytearray()
        try:
            ser.reset_input_buffer()
            while not self._parar.is_set():
                pendientes = ser.in_waiting
                bloque = ser.read(pendientes if pendientes else 1)
                if not bloque:
                    continue
                buffer.extend(bloque)
                # Entregamos solo lineas completas (terminan en \n)
                while b"\n" in buffer:
                    linea, _, buffer = buffer.partition(b"\n")
                    self.cola.put((self, "linea", bytes(linea)))
                if len(buffer) > 256:      # basura sin salto de linea
                    buffer.clear()
        except (serial.SerialException, OSError) as e:
            self.cola.put((self, "error", str(e)))
        finally:
            try:
                ser.close()
            except Exception:
                pass


# ----------------------------------------------------------------------
# Interfaz
# ----------------------------------------------------------------------
class HMI:
    def __init__(self):
        self.cola = Queue()
        self.lector = None
        self.conectado = False
        self.puerto_actual = ""
        self.baudios_actuales = 0

        self.t0 = time.monotonic()
        self.t_conexion = None
        self.ultimo_dato = None

        self.tiempos = deque(maxlen=MAX_PUNTOS)
        self.volt1 = deque(maxlen=MAX_PUNTOS)
        self.volt2 = deque(maxlen=MAX_PUNTOS)

        self.validas = 0
        self.descartadas = 0
        self._estado = None

        self.fuente_ok = False
        self.fuente_grande = None

    # ---- utilidades -------------------------------------------------
    def T(self, texto):
        """Si no hay fuente con acentos, los quita para no ver '?' en pantalla."""
        if self.fuente_ok:
            return texto
        sin = unicodedata.normalize("NFD", texto)
        return "".join(c for c in sin if unicodedata.category(c) != "Mn")

    def texto(self, tag, contenido):
        dpg.set_value(tag, self.T(contenido))

    def poner_estado(self, mensaje, color):
        if self._estado == (mensaje, color):
            return
        self._estado = (mensaje, color)
        self.texto("txt_estado", mensaje)
        dpg.configure_item("txt_estado", color=color)

    def cargar_fuentes(self):
        ruta = os.path.join(os.environ.get("WINDIR", "C:/Windows"), "Fonts", "segoeui.ttf")
        if not os.path.exists(ruta):
            return
        with dpg.font_registry():
            normal = dpg.add_font(ruta, 18)
            self.fuente_grande = dpg.add_font(ruta, 40)
        dpg.bind_font(normal)
        self.fuente_ok = True

    def aplicar_tema(self):
        core = dpg.mvThemeCat_Core
        with dpg.theme() as tema:
            with dpg.theme_component(dpg.mvAll):
                dpg.add_theme_color(dpg.mvThemeCol_WindowBg, (20, 24, 31), category=core)
                dpg.add_theme_color(dpg.mvThemeCol_ChildBg, (28, 33, 42), category=core)
                dpg.add_theme_color(dpg.mvThemeCol_FrameBg, (38, 45, 57), category=core)
                dpg.add_theme_color(dpg.mvThemeCol_Button, (48, 58, 74), category=core)
                dpg.add_theme_color(dpg.mvThemeCol_ButtonHovered, (64, 78, 99), category=core)
                dpg.add_theme_color(dpg.mvThemeCol_Border, (52, 60, 74), category=core)
                dpg.add_theme_style(dpg.mvStyleVar_FrameRounding, 5, category=core)
                dpg.add_theme_style(dpg.mvStyleVar_ChildRounding, 6, category=core)
                dpg.add_theme_style(dpg.mvStyleVar_WindowPadding, 16, 14, category=core)
                dpg.add_theme_style(dpg.mvStyleVar_ItemSpacing, 10, 9, category=core)
        dpg.bind_theme(tema)

    def tema_barra(self, color):
        with dpg.theme() as tema:
            with dpg.theme_component(dpg.mvProgressBar):
                dpg.add_theme_color(dpg.mvThemeCol_PlotHistogram, color,
                                    category=dpg.mvThemeCat_Core)
        return tema

    def tema_linea(self, color):
        with dpg.theme() as tema:
            with dpg.theme_component(dpg.mvLineSeries):
                dpg.add_theme_color(dpg.mvPlotCol_Line, color, category=dpg.mvThemeCat_Plots)
                dpg.add_theme_style(dpg.mvPlotStyleVar_LineWeight, 2.2,
                                    category=dpg.mvThemeCat_Plots)
        return tema

    # ---- construccion de la ventana ---------------------------------
    def listar_puertos(self):
        try:
            puertos = [p.device for p in serial.tools.list_ports.comports()]
        except Exception:
            puertos = []
        if PUERTO_POR_DEFECTO not in puertos:
            puertos.append(PUERTO_POR_DEFECTO)
        return sorted(puertos, key=lambda s: (len(s), s))

    def tarjeta_analogica(self, prefijo, titulo, color):
        with dpg.child_window(height=186, width=-1, border=True, no_scrollbar=True):
            dpg.add_text(self.T(titulo), color=color)
            dpg.add_text("-- V", tag=f"{prefijo}_volt")
            if self.fuente_grande:
                dpg.bind_item_font(f"{prefijo}_volt", self.fuente_grande)
            dpg.add_progress_bar(default_value=0.0, width=-1, tag=f"{prefijo}_barra")
            dpg.bind_item_theme(f"{prefijo}_barra", self.tema_barra(color))
            dpg.add_text(f"ADC: -- de {ADC_MAX}", tag=f"{prefijo}_adc", color=COLOR_TENUE)

    def tarjeta_digital(self, prefijo, titulo):
        with dpg.child_window(height=186, width=-1, border=True, no_scrollbar=True):
            dpg.add_text(self.T(titulo), color=COLOR_TENUE)
            with dpg.group(horizontal=True):
                with dpg.drawlist(width=64, height=64):
                    dpg.draw_circle((32, 32), 26, color=(90, 98, 112), fill=COLOR_LED_OFF,
                                    thickness=2, tag=f"{prefijo}_led")
                with dpg.group():
                    dpg.add_text("--", tag=f"{prefijo}_txt")
                    if self.fuente_grande:
                        dpg.bind_item_font(f"{prefijo}_txt", self.fuente_grande)
                    dpg.add_text(self.T("nivel lógico --"), tag=f"{prefijo}_nivel",
                                 color=COLOR_TENUE)

    def construir(self):
        puertos = self.listar_puertos()
        with dpg.window(tag="principal"):
            # Barra de conexion
            with dpg.group(horizontal=True):
                dpg.add_text("Puerto")
                dpg.add_combo(puertos, default_value=PUERTO_POR_DEFECTO, width=110,
                              tag="combo_puerto")
                dpg.add_button(label="Actualizar", callback=self.actualizar_puertos)
                dpg.add_text("Baudios")
                dpg.add_combo(["1200", "2400", "4800", "9600", "19200", "38400",
                               "57600", "115200"],
                              default_value=str(BAUDIOS_POR_DEFECTO), width=100,
                              tag="combo_baudios")
                dpg.add_button(label="Conectar", width=120, callback=self.alternar_conexion,
                               tag="btn_conectar")
                dpg.add_text("Desconectado", tag="txt_estado", color=COLOR_ERROR)

            dpg.add_spacer(height=4)

            # Lecturas: dos analogicas y dos digitales en una sola fila
            with dpg.table(header_row=False, policy=dpg.mvTable_SizingStretchSame,
                           borders_innerV=False, borders_outerV=False,
                           borders_innerH=False, borders_outerH=False):
                for _ in range(4):
                    dpg.add_table_column()
                with dpg.table_row():
                    with dpg.table_cell():
                        self.tarjeta_analogica("a1", "Analógico 1 (PA1)", COLOR_A1)
                    with dpg.table_cell():
                        self.tarjeta_analogica("a2", "Analógico 2 (PA2)", COLOR_A2)
                    with dpg.table_cell():
                        self.tarjeta_digital("d1", "Digital 1 (PB0)")
                    with dpg.table_cell():
                        self.tarjeta_digital("d2", "Digital 2 (PB1)")

            # Grafica en tiempo real
            with dpg.plot(label=self.T("Voltaje de las entradas analógicas"),
                          height=-84, width=-1, tag="grafica"):
                dpg.add_plot_legend(location=dpg.mvPlot_Location_NorthEast)
                dpg.add_plot_axis(dpg.mvXAxis, label="Tiempo (s)", tag="eje_x")
                with dpg.plot_axis(dpg.mvYAxis, label="Voltaje (V)", tag="eje_y"):
                    dpg.add_line_series([], [], label="A1 (PA1)", tag="serie_a1")
                    dpg.add_line_series([], [], label="A2 (PA2)", tag="serie_a2")
            dpg.bind_item_theme("serie_a1", self.tema_linea(COLOR_A1))
            dpg.bind_item_theme("serie_a2", self.tema_linea(COLOR_A2))
            dpg.set_axis_limits("eje_x", 0, VENTANA_S)
            dpg.set_axis_limits("eje_y", 0, VREF * 1.05)

            # Pie: contadores, ultima trama y limpiar
            with dpg.group(horizontal=True):
                dpg.add_button(label="Limpiar gráfica", callback=self.limpiar_grafica)
                dpg.add_text("", tag="txt_stats", color=COLOR_TENUE)
            dpg.add_text("", tag="txt_ultima", color=COLOR_TENUE)

        self.actualizar_contadores()
        self.texto("txt_ultima", "Última trama: --")

    # ---- acciones del usuario ---------------------------------------
    def actualizar_puertos(self, sender=None, app_data=None, user_data=None):
        dpg.configure_item("combo_puerto", items=self.listar_puertos())

    def alternar_conexion(self, sender=None, app_data=None, user_data=None):
        if self.lector is not None:
            self.desconectar()
        else:
            self.conectar()

    def conectar(self):
        puerto = dpg.get_value("combo_puerto").strip()
        baudios = int(dpg.get_value("combo_baudios"))
        self.limpiar_grafica()
        self.validas = 0
        self.descartadas = 0
        self.actualizar_contadores()
        self.puerto_actual = puerto
        self.baudios_actuales = baudios
        self.ultimo_dato = None
        self.t_conexion = time.monotonic()
        self.lector = Lector(puerto, baudios, self.cola)
        self.lector.start()
        dpg.configure_item("btn_conectar", label="Desconectar")
        self.poner_estado(f"Conectando a {puerto}...", COLOR_AVISO)

    def desconectar(self):
        if self.lector is not None:
            self.lector.detener()
            self.lector.join(timeout=1.0)   # espera a que cierre el puerto
            self.lector = None
        self.conectado = False
        dpg.configure_item("btn_conectar", label="Conectar")
        self.poner_estado("Desconectado", COLOR_ERROR)

    def limpiar_grafica(self, sender=None, app_data=None, user_data=None):
        self.tiempos.clear()
        self.volt1.clear()
        self.volt2.clear()
        self.t0 = time.monotonic()
        dpg.set_value("serie_a1", [[], []])
        dpg.set_value("serie_a2", [[], []])
        dpg.set_axis_limits("eje_x", 0, VENTANA_S)

    # ---- procesamiento de datos (se llama en cada cuadro) -----------
    def procesar_cola(self):
        hubo_datos = False
        while True:
            try:
                origen, tipo, dato = self.cola.get_nowait()
            except Empty:
                break
            if origen is not self.lector:
                continue                    # mensaje de una conexion anterior

            if tipo == "conectado":
                self.conectado = True
                self.t_conexion = time.monotonic()
            elif tipo == "linea":
                if self.procesar_linea(dato):
                    hubo_datos = True
            elif tipo == "error":
                self.lector = None
                self.conectado = False
                dpg.configure_item("btn_conectar", label="Conectar")
                self.poner_estado(self.explicar_error(dato), COLOR_ERROR)

        if hubo_datos:
            self.refrescar_grafica()
        self.vigilar_datos()

    def explicar_error(self, mensaje):
        bajo = mensaje.lower()
        if "permission" in bajo or "denied" in bajo or "busy" in bajo:
            return "Puerto ocupado: cierra Termite u otro programa que lo use"
        if "could not open" in bajo or "filenotfound" in bajo or "cannot find" in bajo:
            return "El puerto no existe: revisa que el par virtual esté activo"
        return "Error de puerto: " + mensaje[:70]

    def procesar_linea(self, cruda):
        texto = cruda.decode("ascii", errors="ignore").strip()
        if not texto:
            return False
        trama = parsear_trama(texto)
        if trama is None:
            self.descartadas += 1
            self.actualizar_contadores()
            return False

        a1, a2, d1, d2 = trama
        self.validas += 1
        self.ultimo_dato = time.monotonic()
        v1 = a1 * VREF / ADC_MAX
        v2 = a2 * VREF / ADC_MAX

        self.tiempos.append(self.ultimo_dato - self.t0)
        self.volt1.append(v1)
        self.volt2.append(v2)

        for pref, adc, volt in (("a1", a1, v1), ("a2", a2, v2)):
            fraccion = adc / ADC_MAX
            dpg.set_value(f"{pref}_volt", f"{volt:.2f} V")
            dpg.set_value(f"{pref}_barra", fraccion)
            dpg.set_value(f"{pref}_adc", f"ADC: {adc} de {ADC_MAX}  ({fraccion * 100:.0f} %)")

        self.poner_led("d1", d1)
        self.poner_led("d2", d2)
        self.texto("txt_ultima", f"Última trama: {texto}")
        self.actualizar_contadores()
        return True

    def poner_led(self, prefijo, valor):
        dpg.configure_item(f"{prefijo}_led", fill=COLOR_LED_ON if valor else COLOR_LED_OFF)
        self.texto(f"{prefijo}_txt", "ALTO" if valor else "BAJO")
        self.texto(f"{prefijo}_nivel", f"nivel lógico {valor}")
        dpg.configure_item(f"{prefijo}_txt", color=COLOR_LED_ON if valor else COLOR_TENUE)

    def refrescar_grafica(self):
        xs = list(self.tiempos)
        dpg.set_value("serie_a1", [xs, list(self.volt1)])
        dpg.set_value("serie_a2", [xs, list(self.volt2)])
        t = xs[-1]
        dpg.set_axis_limits("eje_x", max(0.0, t - VENTANA_S), max(float(VENTANA_S), t))

    def actualizar_contadores(self):
        self.texto("txt_stats",
                   f"Tramas válidas: {self.validas}   descartadas: {self.descartadas}")

    def vigilar_datos(self):
        if not self.conectado:
            return
        referencia = self.ultimo_dato or self.t_conexion
        if time.monotonic() - referencia > SIN_DATOS_S:
            self.poner_estado(f"Conectado a {self.puerto_actual}, pero no llegan datos",
                              COLOR_AVISO)
        else:
            self.poner_estado(
                f"Conectado a {self.puerto_actual} ({self.baudios_actuales} baudios)",
                COLOR_OK)


def main():
    app = HMI()
    dpg.create_context()
    app.cargar_fuentes()
    app.aplicar_tema()
    app.construir()

    dpg.create_viewport(title="HMI - Sistemas Embebidos (ATmega1284P)",
                        width=1080, height=800, min_width=900, min_height=640)
    dpg.setup_dearpygui()
    dpg.show_viewport()
    dpg.set_primary_window("principal", True)

    while dpg.is_dearpygui_running():
        app.procesar_cola()
        dpg.render_dearpygui_frame()

    app.desconectar()
    dpg.destroy_context()


if __name__ == "__main__":
    main()
