# Simple publish-subscribe protocol

SPSP connects low power IoT clients to MQTT[^mqtt] or other protocols.

It's an extensible framework for publish-subscribe pattern.

Main goal is to create highly reliable platform for message delivery from and
to IoT devices while taking into account protocol-specific, performance and
power restrictions.

This project is currently in initial development phase,
there's not yet a stable version and API may change.
There shouldn't be drastic changes, however.


## Usage

### Platform

Implemenation is based on **ESP-IDF**[^espidf] framework, so all ESP32[^esp32]
devices are supported and they are a key target of this project.

Future plans include port to **OpenWrt**[^openwrt] (or other Linux based
systems) – primarily for the *bridge* nodes as it's convenient to have
an access point serving "standard" devices as well as all of IoT.

### Dependencies

- C++ 17 or newer (C++ 20 is going to be required soon)
- ESP-IDF[^espidf] 5.1
- optionally PlatformIO[^platformio]

### Setup

1. Create ESP-IDF[^espidf] project.
2. Create `idf_component.yml` file inside `main` project directory:
   ```yml
   dependencies:
     spsp:
       git: https://github.com/DavidB137/spsp.git
   ```
3. Rename `main.c` to `main.cpp` file inside `main` project directory
   and change it's content to:
   ```cpp
   #include "spsp.hpp"

   extern "C" void app_main()
   {
       // ...
   }
   ```

### Examples

You can find usage examples in `examples/` directory.


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
You can think of bridge as a router or gateway.

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

#### ESP-NOW local layer

ESP-NOW local layer protocol is a wrapper around Espressif's ESP-NOW[^espnow].
It's basically a WiFi without any state management (connecting to AP,...)
whatsoever. Just a quick setup, sending of single packet, receiving of delivery
confirmation – all over WiFi MAC.

It's built-in all ESP32 SOCs. You don't need any additional hardware –
drastically saving cost, labour and enabling usage on third-party devices.

This implemenation includes built-in encryption using **ChaCha20** with 256-bit
password and 64-bit nonce.
To allow multiple separate networks without much interference, 32-bit SSID
must be set.

Each packet is checked for delivery status, but not resent when delivery fails!
Internally, Espressif's implemenation does some sort of retransmission, but
generally, it's not guaranteed that data will be delivered, so custom
implementation of retransmission may be needed if required by your use-case
(support is implemented).

##### Clients using ESP-NOW

*Clients* search for *bridges* to connect to using **probes**.
During this process, the *client* iterates through all WiFi channels allowed by
country restrictions (see `WiFi::Station::setChannelRestrictions`) and
broadcasts probe requests to the broadcast address.
*Bridges* on the same SSID with the correct password will decrypt the request
and send back probe response. The *client* then chooses *bridge* with
the strongest signal.

### Far layer protocols

Far layer protocols are used by *bridges* as concentrators. They collect all
data published by clients and provide data from other sources.

There's just one far layer protocol: MQTT.
More will come later.

#### MQTT far layer

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

### Message types

Message types are generic for current and any future protocols.

#### Bridge discovery message types

- **Probe request (`PROBE_REQ`)**: *Client* attempts *bridge* discovery by
  broadcasting `PROBE_REQ` messages on all available channels.
- **Probe response (`PROBE_RES`)**: *Bridge* response to received `PROBE_REQ`
  from a *client*.
  *Client* listens for `PROBE_RES` messages and chooses the one with
  the strongest signal.
  Address of that *bridge* is stored and all communication is routed to
  that address.

#### Publish and subscribe message types

- **Publish (`PUB`)**: *Client* sends `PUB` message to discovered *bridge* to
  publish payload to topic. *Bridge* forwards it to *far layer* (i.e. MQTT).
- **Subscribe request (`SUB_REQ`)**: Client sends `SUB_REQ` message to
  discovered *bridge* to subscribe to topic. This subscription is requested on
  *far layer* and valid for 15 minutes.
  The *client* can extend the lifetime by sending another `SUB_REQ` (it's done
  automatically).
- **Subscribe data (`SUB_DATA`)**: When *bridge* receives data from far layer,
  it sends `SUB_DATA` messages to all *clients* that are subscribed to
  given topic.
- **Unsubscribe request (`UNSUB`)**: *Client* can (and should) notify
  the discovered *bridge* when it doesn't need subscription anymore by sending
  explicit `UNSUB` message.

#### Time message types

Many applications require current time information.
*Bridge* nodes automatically sync time using SNTP and then provide it to
*clients*.

- **Time request (`TIME_REQ`)**: *Client* sends `TIME_REQ` when
  `Client::syncTime()` method is called.
- **Time response (`TIME_RES`)**: *Bridge* responds to `TIME_REQ` by sending
  `TIME_RES` with current timestamp as payload.
  The timestamp has milliseconds precision, but link latency between *bridge*
  and *client* is not compensated.

#### Other message types

Currently unused, but may be in the future.

- **OK (`OK`)**: Generic OK message (typically for success responses).
- **Failure (`FAIL`)**: Generic failure message (typically for failure
  responses).


[^mqtt]: MQTT https://mqtt.org
[^mqtt_wildcard]: MQTT wildcard: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html#_Toc398718107
[^espidf]: ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/
[^espnow]: ESP-NOW: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html
[^esp32]: ESP32: https://en.wikipedia.org/wiki/ESP32
[^openwrt]: OpenWrt: https://openwrt.org
[^platformio]: PlatformIO: https://platformio.org
