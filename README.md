# Ethernet Dead Canary
This is based off of [another GitHub project by rockettpunk.][1]

That project was intended for homelab use with multiple servers that may not work with typical UPS software, so it provided a standard solution that any linux machine could use to verify the state of power to the building. The idea is to program an ESP32-based dev board to serve a simple webserver that always responds the same to an HTTP request.

The problem for me was that using WiFi seemed counterproductive to an actual rack environment, where I can instead directly wire it with ethernet. Thus, [a W5500 ethernet module][3] and one of my [favorite ESP32 form factors later][2], I built a simple device and wrote a new arduino sketch while using pretty much the same linux script.

## Working principle
An ESP32 hosts a web server and is connected to the network via a network cable. Computers ping the server every minute and check to see if they get a response. If no response is recieved for too long, the server initiates a shutdown command, as that is an indication of power loss behind the UPS.

The specific timings and network details are designed to be user-configurable.

## Required Hardware
- An ESP32-based development board, I used the [Adafruit model 5700][2]
- W5500 Ethernet module, I used [this one from Amazon][3]
- A breadboard/protoboard/perfboard to put it all together.
- Bonus points if you 3D print a nice enclosure.

## Installation
### ESP32 Setup
Simply connect your ESP32 board to your computer and upload the sketch `canary-ethernet.ino` with the Arduino IDE. Be sure to adjust the user-defined variables at the top and adjust it to your needs. The ethernet module uses the SPI interface, follow the instructions to hook up your specific model.

Some of these variables are:
- Network information
- Ethernet chip CS pin
- MAC Addresses for magic packets/wake time (optional)
- CPU clock speed (optional)
- Disable WiFi/Bluetooth flag (optional)

By default, all radio communication is disabled as it is not needed, and the CPU is clocked down as it saves power. You must set the appropriate network details though, as well as the CS pin that you have connected via your breadboard/protoboard solution. I hard-soldered my parts together on a protoboard.

Once it's all set up, connect it to the MAINS POWER (not the UPS) to ensure proper detection. The board should lose power when the building does, and the servers must live past that for a little while!

Then, connect the network cable.

### Server setup
Set up the script `canary-watchdog.sh` on your linux server and then set up a cron job to run it. Make sure you modify the canary IP and other user variables at the top to match your setup. As described by the [original project:][1]

---
Place the script:

```bash
sudo nano /usr/local/bin/canary-watchdog.sh
```

Make it executable:

```bash
sudo chmod +x /usr/local/bin/canary-watchdog.sh
```

Add it to root’s crontab to run every minute:

```bash
sudo crontab -e
```

Then add this line:

```cron
* * * * * /usr/local/bin/canary-watchdog.sh >> /var/log/canary-watchdog.log 2>&1
```
---
I also set up a rotating log with mine to save disk space over time. If you also want Wake-on-LAN, you will need to set up your server to accept magic packets.

### Testing
You can simulate power loss by either unplugging the network cable or the power cable to the ESP board. After the specified threshold time (default is 5 minutes) your servers should safely shut down.

You can view the logs with `tail -f /var/log/canary-watchdog.log`.

## Contributing
Feel free to submit issues or pull requests to fix issues with the code, or if you feel this guide is incomplete in some way.

## License
Following on the open-source nature of the original project, this is licensed under the [MIT License][4]

[1]: https://github.com/rockettpunk/dead-canary
[2]: https://www.adafruit.com/product/5700
[3]: https://www.amazon.com/HiLetgo-Ethernet-Network-Interface-WIZ820io/dp/B08KXM8TKJ?sr=8-3
[4]: https://en.wikipedia.org/wiki/MIT_License
