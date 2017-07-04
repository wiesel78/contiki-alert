# contiki-alert
Contiki-Programm zum senden von Status und Alarm Nachrichten.

## Installation
Installiere einen MQTT-Broker wie [Mosquitto](http://www.eclipse.org/mosquitto/download/) oder [RSMB](https://github.com/MichalFoksa/rsmb) und die [Contiki-Toolchain](https://github.com/contiki-os/contiki/wiki)
und klone das Contiki-Repository. Gib den Pfad des Contiki-Repositories in der Variable CONTIKI in dem Makefile dieses Projektes an.
```
CONTIKI = ../contiki
```
Setze symbolische Links zum Apps-Verzeichnis von Contiki
```
ln -s $HOME/repos/contiki-alert/apps/mqtt-service $HOME/repos/contiki/apps/mqtt-service
ln -s $HOME/repos/contiki-alert/apps/ping-service $HOME/repos/contiki/apps/ping-service
```

oder kopiere die Apps in das Contiki Apps Verzeichnis.

```
cp -r $HOME/repos/contiki-alert/apps/* $HOME/repos/contiki/apps/
```
Setze die Broker-IP in der Datei project-conf.h
```
#define MQTT_BROKER_IP_ADDR "fd00::1"
```
Kompiliere das Programm und Tausche ggf. die Target-Platform aus
```
make TARGET=cc2538dk
```
Verbinde den Sensorknoten mit dem Computer und aktiviere ggf. den Flash-Mode.
Nun Kopiere das Programm auf den Sensorknoten.

```
make TARGET=cc2538dk contiki-alert.upload PORT=/dev/ttyUSB1
```

## Job erstellen
Das Programm arbeitet mit einer Job-Liste in der steht, wann und unter welcher
Bedingung Daten versandt werden.
### Status-Job
Um einen Job zu erstellen der regelmäßig
Statuswerte versendet, schicke das JSON-Objekt
```
{
    "topic":"status/allvalues", // MQTT-Topic
    "status":127,               // All Sensor-Values
    "interval":30,              // Every 30 second
    "type":1                    // Status-Job-Type
}
```
an das Topic "job/add" für alle Knoten und an "job/add/{clientId}" für einen
Speziellen Knoten.

```
mosquitto_pub -h localhost -m "{\"topic\":\"status/allvalues\",\"status\":127,\"interval\":30,\"type\":1}" -t job/add
```

### Alarm-Job
Um einen Job zu senden, der immer dann Alarm schlägt, wenn der Lichtwert länger
als 10 Sekunden unter 10000 sinkt, sende das Objekt
```
{
    "topic":"alert/light",      // MQTT-Topic
    "status":1,                 // Light-Sensor-Value
    "interval":10,              // If value more than 10 seconds lower 10000
    "type":2,                   // Alert type
    "op":1,                     // Compare-Operator : lower
    "value":10000               // Border-Value
}
```
an das Topic "job/add" für alle Knoten und an "job/add/{clientId}" für einen
speziellen Knoten.

```
mosquitto_pub -h localhost -m "{\"topic\":\"alert/light\",\"status\":1,\"interval\":10,\"type\":2,\"op\":1,\"value\":10000}" -t job/add
```

### Werte des Statusfeldes
Das Statusfeld trägt die Information, auf welche Sensorknoten Werte sich der
Job bezieht. Bei Status-Jobs können es mehrere Werte sein, da sie nur versendet
werden. Bei Alarm-Jobs kann nur ein Wert angegeben werden, da darauf die
Operation im "op" Feld durchgeführt wird.

```
DEVICE_STATUS_LIGHT         = 1,
DEVICE_STATUS_TEMPERATURE   = 2,
DEVICE_STATUS_UPTIME        = 4,
DEVICE_STATUS_POWER         = 8,
DEVICE_STATUS_IPV6          = 16,
DEVICE_STATUS_RSSI          = 32,
DEVICE_STATUS_CLIENT_ID     = 64,
DEVICE_STATUS_ALL           =   DEVICE_STATUS_LIGHT |
                                DEVICE_STATUS_TEMPERATURE |
                                DEVICE_STATUS_UPTIME |
                                DEVICE_STATUS_POWER |
                                DEVICE_STATUS_IPV6 |
                                DEVICE_STATUS_RSSI |
                                DEVICE_STATUS_CLIENT_ID,
```

### Werte des Op Feldes
Im Op Feld (Operation) steht, um welche Operation es sich beim Alarm-Job
handelt. Es wirkt sich nur auf Alarm-Jobs aus. Ein Alarm Job prüft, ob ein
bestimmter Wert für eine gewisse Zeit einen Grenzwert über oder unterschritten
hat. Ob auf über oder unterschreiten geprüft wird, gibt der "op" Wert an.

```
COMPARE_OPERATOR_LOWER = 0x1,
COMPARE_OPERATOR_GREATER = 0x2,
COMPARE_OPERATOR_EQUAL= 0x4,
COMPARE_OPERATOR_LOWER_EQUAL =
    COMPARE_OPERATOR_LOWER |
    COMPARE_OPERATOR_EQUAL,
COMPARE_OPERATOR_GREATE_EQUAL =
    COMPARE_OPERATOR_GREATER |
    COMPARE_OPERATOR_EQUAL,
```

## Daten empfangen
Um Daten zu empfangen abonniert man einen MQTT-Topic. Um zum Beispiel
mit zu bekommen, dass ein Sensorknoten sich am Broker anmeldet, reicht es
den topic "clients/#" zu abonnieren.
```
mosquitto_sub -t "clients/#"
```

ähnliches kann man für die topic einrichten, die man bei der Joberstellung
vergeben hat.

## JS-Client
Eine Referenzimplementierung in Javascript bzw. Nodejs liegt im Verzeichnis
clients/js.

### Installation
Es wird NodeJs und der dazugehörige Packetmanager npm benötigt. Sobal dies
installiert ist, gehe in das Verzeichnis

```
cd clients/js
```

Installiere npm-Pakete

```
npm install
```

Starte server (evtl. mit root rechten oder geändertem Port)

```
npm run server
```

Öffne einen Browser und besuche "localhost"
