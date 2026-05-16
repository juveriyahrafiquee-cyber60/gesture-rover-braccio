# ═══════════════════════════════════════════════════════════════
#  GESTURE CAR — CAR SIDE
#  Raspberry Pi Pico + NRF24L01 + 2x L298N + 4x BO Motors
#
#  Libraries needed (copy to Pico via Thonny):
#    - micropython-nrf24l01 (nrf24l01.py)
#    Download: https://github.com/micropython/micropython-lib/blob/master/micropython/drivers/radio/nrf24l01/nrf24l01.py
#
#  NRF24L01 Pins:
#    CE   → GP0
#    CSN  → GP1
#    SCK  → GP2  (SPI0)
#    MOSI → GP3  (SPI0)
#    MISO → GP4  (SPI0)
#
#  L298N #1 (Left Motors):
#    IN1→GP6  IN2→GP7  IN3→GP8  IN4→GP9
#    ENA→GP10(PWM)  ENB→GP11(PWM)
#
#  L298N #2 (Right Motors):
#    ENA→GP16(PWM)  ENB→GP17(PWM)
#    IN1→GP18  IN2→GP19  IN3→GP20  IN4→GP21
# ═══════════════════════════════════════════════════════════════

import machine
import utime
import struct
from nrf24l01 import NRF24L01

# ─── Status LED (onboard LED = GP25) ───────────────────────────
led = machine.Pin(25, machine.Pin.OUT)

def blink(times=1, ms=100):
    for _ in range(times):
        led.value(1)
        utime.sleep_ms(ms)
        led.value(0)
        utime.sleep_ms(ms)

# ─── NRF24L01 Setup ────────────────────────────────────────────
spi = machine.SPI(0,
    baudrate=4000000,
    polarity=0,
    phase=0,
    sck=machine.Pin(2),
    mosi=machine.Pin(3),
    miso=machine.Pin(4)
)

ce  = machine.Pin(0, machine.Pin.OUT)
csn = machine.Pin(1, machine.Pin.OUT)

# Same address as glove
ADDRESS = b"GCAR1"

nrf = NRF24L01(spi, csn, ce, payload_size=5)  # 1 byte + 2x int16 = 5 bytes
nrf.open_rx_pipe(1, ADDRESS)
nrf.set_channel(108)       # Must match glove
nrf.set_power_speed(NRF24L01.POWER_3, NRF24L01.SPEED_250K)
nrf.start_listening()      # Car = receiver

print("NRF24L01 ready — listening")
blink(3, 200)  # 3 blinks = ready

# ─── Motor Pins ────────────────────────────────────────────────
# L298N #1 — Left Motors
IN1 = machine.Pin(6,  machine.Pin.OUT)
IN2 = machine.Pin(7,  machine.Pin.OUT)
IN3 = machine.Pin(8,  machine.Pin.OUT)
IN4 = machine.Pin(9,  machine.Pin.OUT)
ENA = machine.PWM(machine.Pin(10))  # Left Front speed
ENB = machine.PWM(machine.Pin(11))  # Left Rear  speed

# L298N #2 — Right Motors
ENC = machine.PWM(machine.Pin(16))  # Right Front speed
END = machine.PWM(machine.Pin(17))  # Right Rear  speed
IN5 = machine.Pin(18, machine.Pin.OUT)
IN6 = machine.Pin(19, machine.Pin.OUT)
IN7 = machine.Pin(20, machine.Pin.OUT)
IN8 = machine.Pin(21, machine.Pin.OUT)

# PWM frequency 1kHz
for pwm in [ENA, ENB, ENC, END]:
    pwm.freq(1000)

# ─── Motor Helpers ─────────────────────────────────────────────
def set_pwm(pwm_pin, value):
    # 0–255 → 0–65535
    duty = int((value / 255) * 65535)
    pwm_pin.duty_u16(duty)

def left_forward(spd):
    IN1.value(1); IN2.value(0)
    IN3.value(1); IN4.value(0)
    set_pwm(ENA, spd)
    set_pwm(ENB, spd)

def left_backward(spd):
    IN1.value(0); IN2.value(1)
    IN3.value(0); IN4.value(1)
    set_pwm(ENA, spd)
    set_pwm(ENB, spd)

def left_stop():
    IN1.value(0); IN2.value(0)
    IN3.value(0); IN4.value(0)
    set_pwm(ENA, 0)
    set_pwm(ENB, 0)

def right_forward(spd):
    IN5.value(1); IN6.value(0)
    IN7.value(1); IN8.value(0)
    set_pwm(ENC, spd)
    set_pwm(END, spd)

def right_backward(spd):
    IN5.value(0); IN6.value(1)
    IN7.value(0); IN8.value(1)
    set_pwm(ENC, spd)
    set_pwm(END, spd)

def right_stop():
    IN5.value(0); IN6.value(0)
    IN7.value(0); IN8.value(0)
    set_pwm(ENC, 0)
    set_pwm(END, 0)

def stop_all():
    left_stop()
    right_stop()

# ─── Differential Steering ─────────────────────────────────────
# steer: -255 = full left, +255 = full right
# Slows down the turning side proportionally

def drive(direction, speed, steer):
    left_spd  = speed
    right_spd = speed

    if steer > 0:    # turning right → slow right side
        right_spd = int(speed * (1.0 - abs(steer) / 255.0))
    elif steer < 0:  # turning left  → slow left side
        left_spd  = int(speed * (1.0 - abs(steer) / 255.0))

    left_spd  = max(0, min(255, left_spd))
    right_spd = max(0, min(255, right_spd))

    if direction == ord('F'):
        left_forward(left_spd)
        right_forward(right_spd)
        led.value(1)
    elif direction == ord('B'):
        left_backward(left_spd)
        right_backward(right_spd)
        led.value(1)
    else:
        stop_all()
        led.value(0)

# ─── Main Loop ─────────────────────────────────────────────────
last_packet  = utime.ticks_ms()
TIMEOUT_MS   = 500  # Safety stop if no packet for 500ms

print("Waiting for packets...")

while True:
    # Check if data available
    if nrf.any():
        buf = nrf.recv()

        try:
            # Unpack: 1 uint8 + 2x int16 = 5 bytes
            # 'B' = unsigned byte, 'h' = signed 16-bit int (little-endian)
            dir_byte, speed, steer = struct.unpack('<Bhh', buf)

            drive(dir_byte, speed, steer)
            last_packet = utime.ticks_ms()

            print("Recv: dir={} spd={} steer={}".format(
                chr(dir_byte), speed, steer))

        except Exception as e:
            print("Parse error:", e)

    # Safety stop — no packet received for too long
    if utime.ticks_diff(utime.ticks_ms(), last_packet) > TIMEOUT_MS:
        stop_all()
        led.value(0)

    utime.sleep_ms(5)  # 200Hz loop — very responsive
