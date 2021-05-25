# TeamRocket
Temperature and humidity probes with WiFi connectivity using ESP8266 and DHT11

# Instructions (for NodeMCU 1.0 ESP8266-12E)

## Connection with DHT11

```
NodeMCU GND ---------- DHT11 GND (pin 4)
NodeMCU 3V3 ---------- DHT11 Vcc (pin 1)
                    |  4k7 pull up resistor between 3V3 and D5
NodeMCU D5  ---------- DHT11 Data Out (pin 2)
```

## Flashing

- For each device, change the `WHOAMI`, `STASSID` and `STAPSK` macros to match
your device and network.
- Compile and flash the Arduino code for `NodeMCU 1.0 ESP-12E` device

## Debug

The devices expose a COM port with baud rate `115200` for debug, including a
pre-scan of available networks. The devices will wait `10 seconds` before
initializing so the user have time to properly establish the connection.

If you don't have the Arduino serial monitor, an alternative is minicom 
(the TTY device may vary):

```
sudo minicom -D /dev/ttyUSB0 -b 115200 -8
```

# Setting up Wi-Fi

The SSID and password for the network are saved in a flash memory present 
on the devices. To connect for the first time or to change credentials, 
open de serial connection to the device and send the new credentials in 
the following format:

```
SSID/password\n
```

Where `\n` is a simple new-line without the `\r` carriage return. **This must 
be done during the 10 second grace period before initialization.**

# Accessing the unit

After a successfull initialization, the devices will be accessible via `http`
at their addresses. An automation script can use `curl` for getting the data:

```
curl 'http://IP_OF_THE_DEVICE/'
```

The `WHOAMI` variable also enables `mDNS` lookup when using the `.local`
suffix.

The data is a string in the format:

```
TEMPERATURE_WITH_DECIMALS,TEMPERATURE_UNIT,HUMIDITY_WITH_DECIMALS,HUMIDITY_UNIT
```

For instance:

```
21.7,oC,40,%
```

BuT wIlL It hAvE FaHrEnHeIt sUpPoRt? **No**.
