PROGRAM = esp-gizmo-ir-remote
OTA = 1
DEVICE_IP = 192.168.0.107
EXTRA_COMPONENTS = extras/rboot-ota extras/pwm ./midea-ir extras/paho_mqtt_c
PROGRAM_SRC_DIR = ./app
ESPPORT = /dev/tty.SLAB_USBtoUART

include ../esp-open-rtos/common.mk

upload: all
	@-echo "mode binary\nput firmware/$(PROGRAM).bin firmware.bin\nquit"\
		| tftp $(DEVICE_IP)
	@echo "Upload finished"

console:
	picocom -b 115200 /dev/ttyUSB0
