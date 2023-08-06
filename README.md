# Simple publish-subscribe protocol

SPSP connects low power IoT clients to MQTT[^mqtt] or other protocols.

It's an extensible framework for publish-subscribe pattern.

### Reliability

As reliability is important, the whole system aims to be a highly reliable
platform while taking into account protocol-specific and performance
restrictions. There's not a 100 % guarantee of message delivery from client
to the MQTT[^mqtt] broker, but with correct settings, you can get extremely
close.

No retrasmission logic between *nodes* is implemented, because it's highly
application-specific, but the *node* always knows whether data have or have not
beed delivered, so you can implement it yourself.


## Usage

### Platform

Implemenation is based on **ESP-IDF**[^espidf] framework, so all ESP32[^esp32]
devices are supported and they are a key target of this project.

Future plans include port to **OpenWrt**[^openwrt] (or other Linux based
systems) – primarily for the *bridge* nodes as it's convenient to have
an access point serving "standard" devices as well as all of IoT.

### Dependencies

- C++ 17 or newer (C++ 20 is going to be required soon)
- ESP-IDF[^espidf] 5.x
- optionally PlatformIO[^platformio]

### Setup

TODO

### Examples

You can find usage examples in `examples/` directory.

### Known limitations

- Subscription to topic which includes wildcards[^mqtt_wildcard] (`+`, `#`) is
  not yet supported.


## Network architecture

### Nodes

Currently, there are 2 node types: client and bridge.

#### Client

Client is generic node type for **low power sensors**.

The client typically makes a measurement as quickly as possible
(in order of 100s of milliseconds) and goes to deep sleep afterwards
(for a longer time period).
Always-on clients and any combinations are supported as well.

Upstream connectivity (to the *bridge*) is provided by *local layer*
(for example ESP-NOW[^espnow]), which allows for this extremly quick wake-up
periods.
The client somehow (protocol dependent) establishes connection to near-by
*bridge* (think of it as router) processing all messages from client.

The client can *publish* data or *subscribe* to upstream data.

##### Reporting

For debugging purposes, client also publishes signal strength when it receives
[probe response](#clients-using-esp-now) to `_report/rssi/{BRIDGE_ADDR}` topic
(+ prefix depending on far layer).

#### Bridge

Bridge **connects *clients* on *local layer* and *far layer***.
You can think of bridge as a IP network router.

All received *publish* messages are forwarded to upstream *far layer*
(typically MQTT[^mqtt]). When *subscribe* request is received from a client, topic of
the subscription gets stored in the database and registered on the *far layer*.
Any number of *clients* can subscribe to the same topic and even the bridge
itself can be one of them.

The bridge can also *publish* data, so it can function as both bridge and
client at the same time.

##### Reporting

Bridge reports (topics don't include far layer prefix):
- SPSP version on startup to `_report/version` topic
- signal strength to *client* nodes to `_report/rssi/{CLIENT_ADDR}` topic
- probe request payload to `_report/probe_payload/{CLIENT_ADDR}` topic
  (should include SPSP version of client)

Reporting can be configured or completely disabled in bridge configuration.

### Local layer protocols

Local layer protocols are used for local communication between nodes
(*clients* and *bridges*).

There's just one local layer protocol: ESP-NOW.
More will come later.

#### ESP-NOW

ESP-NOW local layer protocol is a wrapper around Espressif's ESP-NOW[^espnow].
It's basically a WiFi without any state management (connecting to AP,...)
whatsoever. Just a quick setup, send of single packet, receive of delivery
confirmation – all over WiFi MAC.

This implemenation includes built-in encryption using **ChaCha20** with 256-bit
password and 64-bit nonce.
To allow multiple separate networks without much interference, 32-bit SSID
must be set.

Each packet is checked for delivery status.
Internally, Espressif's implemenation does some sort of retransmission, but
generally, it's not guaranteed that data will be delivered, so custom
implementation of retransmission may be needed if required by your use-case.

##### Clients using ESP-NOW

*Clients* search for *bridges* to connect to using **probes**.
During this process, the *client* iterates through all WiFi channels allowed by
country restrictions (see `WiFi::setCountryRestrictions`) and broadcasts probe
requests to the broadcast address. *Bridges* on the same SSID with the correct
password will decrypt the request and send back probe response. The *client*
then chooses *bridge* with the best signal strength.

### Far layer protocols

Far layer protocols are used by *bridges* as concentrators. They collect all
data published by clients and provide data from other sources.

There's just one far layer protocol: MQTT.
More will come later.

#### MQTT

Wrapper for most well known publish-subscribe IoT protocol – MQTT[^mqtt].

All standard authentication methods are supported:
- unauthenticated over TCP
- username and password
- TLS
- unauthenticated over WebSockets
- WebSockets secure

Internally ensures retransmission and reconnection to MQTT broker if needed.

Default topic structure for *publishing* is `{PREFIX}/{ADDR}/{TOPIC}` where:
- `PREFIX` is configured topic prefix (`spsp` by default)
- `ADDR` is node's address as reasonably-formatted string (in case of
ESP-NOW[^espnow] it's lowercase MAC address without separators)
- `TOPIC` is topic received in message (e.g. `temperature`)

Topics for *subscribing* are not prepended or modified in any way.


[^mqtt]: MQTT https://mqtt.org
[^mqtt_wildcard]: MQTT wildcard: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718107
[^espidf]: ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/
[^espnow]: ESP-NOW: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
[^esp32]: ESP32: https://en.wikipedia.org/wiki/ESP32
[^openwrt]: OpenWrt: https://openwrt.org
[^platformio]: PlatformIO: https://platformio.org
