# Wi-Se Remote UART Terminal - C++ implementation

> *Wireless Serial*

![Demo](https://i.postimg.cc/PqRRcwX1/ezgif-com-optimize-1.gif)

---

This software allows for using an ESP8266 board as a remote UART terminal. It is very fast, reaching (with [caveats](#caveats)) up to
1500000bps rates.

It is intended as a firmware for the [Wi-Se](https://github.com/Depau/wi-se-hw/)
boards (which you can order and build at your favorite PCB manufacturer), though it will work just fine with a normal ESP8266 breakout
board.

With some changes it may work with ESP32 but there's no interest for that at the moment. Pull requests are welcome.

Communication occurs over WebSockets: it is compatible with
[ttyd](https://github.com/tsl0922/ttyd/). In fact, the web UI is the same. CLI clients compatible with ttyd, such
as [ttyc](https://github.com/Depau/ttyc)
should also work with Wi-Se.

Wi-Se uses a superset of ttyd's protocol. Non Wi-Se-aware clients won't be able to use all features.

Wistty can be used to control remote terminal configuration parameters. It is part of ttyc:
https://github.com/Depau/ttyc

## Features

- Web-based terminal based on Xterm.js
- Very low latency (~10ms on average, depending on your Wi-Fi)
- Relatively high baud rates are supported (up to ~1500000bps, with
  [caveats](#caveats))
- Zmodem support on Web UI
- Native *nix client: [`ttyc`](https://github.com/Depau/ttyc)
- OTA firmware updates
- Automatic baud detection (ymmv)
- Remote terminal parameters can be changed on the fly

## Building and flashing

Building is only supported and tested on GNU/Linux x86_64.

Building on Windows is not supported. WSL may work but is not tested. Building on macOS may work but it is not tested. Pull requests are
welcome.

Requirements:

- PlatformIO Core ([installation instructions](https://docs.platformio.org/en/latest/core/installation.html#super-quick-mac-linux))
- Python 3.8+

The build script also has some additional Python dependencies: `pyjq`, `jinja2`,
`pyyaml`. You may install them through your distro package manager or use a virtualenv as described here:

```bash
python3 -m venv venv
source venv/bin/activate
pip install -r requirements.txt
```

To build and flash the project:

- Inside `configs/`, copy `config.yml.example` to `device_name.yml` and change configuration parameters as needed.
    - The serial port or IP address for OTA updates must be specified in the config file.
- Activate Python virtualenv:
  ```bash
  source venv/bin/activate
  ```
- Build and upload:
  ```bash
  ./builder.py upload              # To flash all configured devices
  ./builder.py upload device_name  # To flash only using "device_name.yml"
  ```

The `builder.py` script makes it easy to keep a number of devices up-to-date, without having to swap config files.

The `platformio.ini` and `include/config.h` files are automatically generated by the build script based on each single human-readable YAML
config.

For development you can change config options in `configs/config.yml.example`, then run `./builder.py devconf --example` to generate the
headers in-place.

## Configuration

Configuration is located under `configs/`. An example config `config.yml.example` is provided, with comments and all options set to their
default values.

Runtime configuration changes are not supported and will not be supported. This helps keep the code simple and reduce security and memory
corruption issues.

Pushing a OTA firmware update to change the configuration is simple enough.

UART parameters can be changed at runtime, but the original configuration will be restored in case of restart.

## Changing UART parameters at runtime

Ideally it should be implemented in the web page, but I don't know ReactJS (pull-requests welcome). `wistty` (part of `ttyc`) can be used
to set them. `ttyc` supports setting the UART parameters directly.

If you don't want to use `wistty` you can change the baud rate and the other parameters by sending an HTTP request:

```bash
curl -X POST IP_ADDRESS/stty -H 'Content-Type: application/json' \
-d '{"baudrate":1500000,"bits":8,"parity":null,"stop":1}'

# To fetch current setting:
curl IP_ADDRESS/stty

# For authentication, add:
--digest --user username:password
```

Bits (data bits) can be `5`, `6`, `7` or `8`, and it must not be `8` if parity is not none.

Parity can be `null` (none), `0` (even), `1` (odd).

Stop (stop bits) can be `0`, `1`, `2`.

Defaults (`8`, `null`, `1`) will work for most setups.

You don't have to provide all the parameters, you can provide only the parameters you want to change, for example
`{"baudrate": 115200}`.

## Caveats

ESP8266 has incredible capabilities, but fast Wi-Fi isn't one of them.

The UART works fine with baudrates higher than 1.5 mbps (1500000 bps), however average Wi-Fi transfer speed is usually around 900 kbps.

To get best performance:

- Avoid the UART to USB adapter built into most ESP8266 devkits (but rather use a better external adapter such as those based on FTDI chips)
    - The built-in adapter won't go faster than ~500000bps
- Enable software flow control and make sure it is supported by and enabled on the connected device
- Avoid sending constant streams of data at high rates if flow control cannot be enabled

This firmware implements UART software flow control and it is enabled by default.

With software flow control, Wi-Se asks the connected UART device to momentarily suspend the data transfer (IXON/IXOFF) when the Wi-Fi can't
keep up.

This will improve reliability at high speeds by orders of magnitude.

See the next section on how to ensure flow control is enabled on Linux-based devices

## Software flow control on Linux

Linux supports flow control and it is usually enabled by default. Some shells (such as `fish`) disable software flow control on start.

`fish` versions prior to 3.2.0 do not support enabling it. Starting from 3.2.0, `fish` will still disable flow control on start up, but it
will respect your choice if you enable it in your configuration file.

`bash` and `zsh` usually don't mess with it. However, some "plug-ins" may disable it.

Add the following at the end of your shell configuration file to ensure it is enabled when you login from a terminal.

`~/.bashrc`, `~/.zshrc`, etc.

```bash
tty | grep -qE '/dev/tty[A-Za-z]+[0-9]*' && stty ixon ixoff
```

`~/.config/fish/config.fish`

```fish
string match -rq '/dev/tty[A-Za-z]+\d*' (tty) && stty ixon ixoff
```

## Troubleshooting

### Terminal is stuck

You might have pressed `Ctrl`+`S` and triggered flow control by mistake. If you're using `ttyc` you can press `Ctrl`+`Q` to unlock it. If
you're using the web client, this will close the browser. Sorry :/ Again, PRs welcome ;)

Another possible reason is that the firmware crashed. When the firmware crashes, the UART goes out of control until execution restart. This
may result in sending a "break condition", which causes [`agetty`](https://man.archlinux.org/man/agetty.8) to switch to the next baud rate.

If you're using `ttyc` you can attempt to manually send more breaks, until the terminal becomes responsive again, or try to perform an
automatic baud detection. Every time you send a break, `agetty` will try the next baud rate. Note that if the current console is enabled for
kernel messages and [SysRq](https://www.kernel.org/doc/html/latest/admin-guide/sysrq.html#how-do-i-use-the-magic-sysrq-key) is enabled you
have to send a break twice in a row.

As a workaround you can change, on connected device, the options passed to `agetty` and configure it to use a single baudrate. On
systemd-based distributions you can run `sudo systemctl edit serial-getty@ttyXXX.service` (you can retrieve the TTY by running `tty`), then
add

```systemd
[Service]
ExecStart=
ExecStart=-/sbin/agetty -o '-p -- \\u' --keep-baud 115200,57600,38400,9600 %I $TERM
```

Change the baudrate to whatever you like, then `sudo systemctl restart serial-getty@ttyXXX.service`.

### Terminal output is garbled

Software flow control is disabled or not supported/enabled on the connected device. See if running `stty ixon ixoff` on the remote device
makes any difference.

If you can't enable flow control on the connected device, try with low baud rates.

## LEDs meaning

Wi-Se boards come with 4 LEDs:

| LED      | Color        |
| -------- | ------------ |
| Wi-Fi    | Blue         |
| Status   | Yellow/Amber |
| TX       | Red          |
| RX       | Red          |

#### Wi-Fi LED blinking, status LED on

Connecting to Wi-Fi.

#### Wi-Fi LED on

Connected and operating normally.

#### Wi-Fi LED on, status LED blinking fast (or on)

Operating normally, but flow control is currently blocked. If the status LED doesn't turn off within 0.5 sec there might be a bug.

It is normal for flow control to occur regularly when there is a lot of terminal activity. However, if it gets stuck turned on it might be
the symptom of another issue.

#### TX and RX blink very fast

Terminal activity:

- TX blinks ⇒ WebSocket to UART
- RX blinks ⇒ UART to WebSocket

#### TX and RX blink one at a time, slowly

Device error:

- OTA update failed (ensure the OTA host port is enabled in your firewall - OTA requires the devices to be mutually reachable)
- When in Wi-Fi station (client) mode: disconnected from the wireless network

The device will restart after around 1 second.

#### OTA LED animations

When performing OTA, the device will switch to "Christmas tree lights mode" and show a series of animations to report the current status.

- OTA start: LEDs turn on one at a time, twice, in sequence RX, TX, Status, Wi-Fi
- OTA progress: the LEDs will act as a firmware download progress bar
- OTA error: RX/TX LEDs blink slowly
- OTA success: RX, TX and status LEDs will turn off in sequence, Wi-Fi LED will stay on. The device will restart after ~3 sec into the new
  firmware.

## License

This project is licensed under the GNU General Public License v3.0.

All content under `/html` was originally written for
[ttyd](https://github.com/tsl0922/ttyd/), and it has been slightly modified.
[ttyd](https://github.com/tsl0922/ttyd/) is licensed under MIT license.

See the git commit history for the `/html` directory for original authors credits.

See `fakeesp/README.md` for licensing info for the ESP SDK and libraries mocks.




