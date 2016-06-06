# esp-gizmo-ir-remote
IR remote control for Air Conditioner

## MQTT communication (not implemented)

Each device has a location, a type and a name.

For example:
Location: 'kitchen', type: 'switch', name: 'main'
The device topic will be '/kitchen/switch/main'

### Commands
The command topic consists of the device topic plus '/cmd'.
So the device will subscribe to a topic '/location/type/name/cmd' and listens
for commands.

#### Commands specification

Supported commands:
 * "on" - Turn on the Air conditioner.
 * "off" - Turn off the Air conditioner.
 * "temp T" - Set temperature, where T is value 17-30.
 * "auto" - Set mode to auto.
 * "cool" - Set mode to cool.
 * "heat" - Set mode to heat.
 * "fan" -  Set mode to fan.
 * "fan_level N" -  Set fan level, where N is value 0-3.
 * "move" - Move the deflector one position.

### Status
The status topic consists of the device topic plus '/status'.
The device will publish its status changes using topic
'/location/type/name/status'

#### Status message specification

The status message format:
    "SSS mode TT FF"

 * SSS - "on"/"off"
 * mode - one of "auto","cool","heat","fan"
 * TT - temperature in Celsius
 * FF - fan level

The status message is published each time a command is received.

## Configuration (idea, not implemented yet)

There are three option to configure a device:
  * Press a config button during power on.
The device will start a WiFi Config AP with distinctive name.
  * If device hasn't been previously configured it will start a WiFi Config AP
automatically.
  * If device sees a WiFi AP with distinctive name during power on it will
connect to it.

In each case when WiFi connection is established the configuration web server
starts on the device and can be accessed with IP address of the device.
For example: 192.168.0.2/config
On the web page various configuration options can be entered and submitted.
After submit the device will restart and configuration will be applied.
