import smbus
import time
import csv
import matplotlib.pyplot as plt
import math

# Dirección I2C del ADXL345
ADXL345_ADDRESS = 0x53

# Umbral de vibración (en g)
UMBRAL_VIBRACION = 1.5

# Inicializar I2C
bus = smbus.SMBus(1)

def configurar_adxl345():
    # Coloca el sensor en modo de medición
    bus.write_byte_data(ADXL345_ADDRESS, 0x2D, 0x08)

def leer_eje(eje):
    registro = {
        'x': 0x32,
        'y': 0x34,
        'z': 0x36
    }[eje]

    datos = bus.read_i2c_block_data(ADXL345_ADDRESS, registro, 2)
    valor = datos[0] | (datos[1] << 8)
    if valor & (1 << 15):
        valor -= (1 << 16)
    return valor * 0.004  # Cada bit = 4mg

def leer_vibracion():
    x = leer_eje('x')
    y = leer_eje('y')
    z = leer_eje('z')
    # Magnitud total de vibración
    return round(math.sqrt(x**2 + y**2 + z**2), 2)

def guardar_datos_csv(datos, archivo='vibraciones_adxl345.csv'):
    with open(archivo, mode='w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["Tiempo (s)", "Vibración (g)"])
        writer.writerows(datos)

def monitorear_vibraciones():
    configurar_adxl345()
    datos = []
    print("Iniciando monitoreo con ADXL345...\n")

    for i in range(100):
        vibracion = leer_vibracion()
        tiempo = round(i * 0.1, 1)
        datos.append((tiempo, vibracion))

        estado = "ALERTA" if vibracion > UMBRAL_VIBRACION else "OK"
        print(f"Tiempo: {tiempo}s | Vibración: {vibracion}g | Estado: {estado}")

        time.sleep(0.1)

    guardar_datos_csv(datos)
    print("\nDatos guardados en 'vibraciones_adxl345.csv'")

    tiempos, vibraciones = zip(*datos)
    plt.plot(tiempos, vibraciones, label='Vibración (g)')
    plt.axhline(UMBRAL_VIBRACION, color='red', linestyle='--', label='Umbral')
    plt.xlabel("Tiempo (s)")
    plt.ylabel("Vibración (g)")
    plt.title("Monitoreo de Vibraciones con ADXL345")
    plt.legend()
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    monitorear_vibraciones()
