# MCU settings
MCU_SERIES = f7
CMSIS_MCU = STM32F767xx
MICROPY_FLOAT_IMPL = double
AF_FILE = boards/stm32f767_af.csv
LD_FILES = boards/PYBD_SF6/f767.ld
TEXT0_ADDR = 0x08008000

# MicroPython settings
MICROPY_PY_LWIP = 1
