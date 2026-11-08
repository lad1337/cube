# cube

LED cube: three 64x64 FM6126A HUB75 panels chained as one 192x64 canvas, driven by a
Raspberry Pi 2 Model B with an Adafruit RGB Matrix HAT. A GLSL shader renders on the
Pi's GPU, the frame is read back and pushed to the panels via hzeller/rpi-rgb-led-matrix.

## Files

- `cpu-stats-gl.cpp`: renderer. EGL/GLES2 pbuffer (192x64) -> `glReadPixels` ->
  `SetPixel` -> `SwapOnVSync`, ~30 fps. Listens on UDP 1234 for
  `load,download,upload,on,brightness` (positional, negative = ignore field).
  Flags: `--render shader/render.X.glsl`, `-v`/`-vv`/`-vvv` (`-vv` prints fps, `-vvv` shows
  matrix refresh rate), `--print-shader`, plus any `--led-*` library flag.
- `shader/fragment.template.glsl` + `shader/render.*.glsl`: the render function is
  spliced in at `// RENDER ENDS HERE`. Uniforms: `time`, `age` (seconds since last UDP
  stats), `load`, `download`, `upload` (0..1). Default shader: `render.smoke2.glsl`.
- `as-light.py`: HAP-python HomeKit lightbulb "Cube" (port 51826, pairing in
  `/etc/cube/cube.state`). Sends `-1,-1,-1,<on>,<brightness>` to 127.0.0.1:1234.
- `stats.go`: "Cubestatspusher", runs on zeta (192.168.1.201) as `/root/stats`, sends
  `load,downbytes,upbytes` to the cube every ~1s (`STATS_TRAGET_IP`, sic).
- `led.service`, `homekit.service`, `led.cron`, `r8188eu.conf`: deployed system config.
- `Makefile`: `make deploy` (copy + compile on the cube), `make setup` (fresh Pi, untested).

## The Pi (`ssh cube`, 192.168.1.116)

- Raspbian Buster, kernel 5.10, legacy Broadcom GL (`/opt/vc`, `-lbrcmEGL -lbrcmGLESv2`).
  Newer OS images don't ship these.
- Builds happen on the Pi (`make deploy`); the local clang errors on macOS
  (`EGL/egl.h` not found) are expected.
- `/usr/bin/cpu-stats-gl` -> `~/cpu-stats-gl`, `/usr/bin/as-light.py` -> `~/as-light.py`,
  `/etc/cube/shader` -> `~/shader`. Both services run as root with
  `WorkingDirectory=/etc/cube`; units are linked from `~/` via `systemctl enable ~/x.service`.
- Boot: `isolcpus=3` (matrix refresh thread runs on core 3 at RT prio), `dtparam=audio=off`.
- HAP-python 4.9.2 installed as root (`sudo pip3`); the user-site 4.6.0 is not the one used.
  4.6.0 caused "No Response" with the Kitchen HomePod.
- No persistent journal; logs before a reboot are gone. `tcpdump` isn't installed, use a
  Python `AF_PACKET` sniffer (`python3 -u`, or output is lost on timeout).
- `gdb -p <pid> -batch -ex "print t"` reads the animation clock without restarting.
- Networking is all USB: Wi-Fi dongle RTL8188EUS (`r8188eu` staging driver, wext only),
  Ethernet SMSC9514. Wi-Fi MAC `00:e0:2d:ac:69:5d`.
- Power: the old 5V 10A supply caused under-voltage and a crash; `vcgencmd get_throttled`
  must read `0x0`.

## Services and runtime behaviour

- `led.service`: `cpu-stats-gl -v --render shader/render.smoke2.glsl`, `Restart=always`.
  Restarted daily at 04:00 by `/etc/cron.d/led` (replaced the old `RuntimeMaxSec=9h`).
  `ExecStartPost` waits 5s, then `systemctl kill -s USR1 homekit`.
- `homekit.service`: waits for an IPv4 default route before starting. HAP binds to
  whatever address exists at start; after a slow DHCP boot it once bound to 169.254.x and
  the Home app showed "No Response".
- HomeKit on/brightness live only in `as-light.py`'s memory (no SD writes). On SIGUSR1 it
  resends them, so the daily `led` restart keeps the state. Reboot or `homekit` restart
  resets to on/100%. Do NOT resend periodically: a send every 5s made the LEDs flicker.
- Active HomeKit hub is the Living Room Apple TV (192.168.1.211). If the cube shows
  "No Response", restart `homekit` (and the Apple TV if needed).
- Black-and-white display = no stats packets for >10s (`age` -> `greyscale()` in the
  shader). Means a network problem, or zeta's `stats` stopped / targets the wrong IP.

## Renderer pitfalls

- Animation clock `t` is a `double` and advances 0.01 per *rendered* frame only. It used to
  advance while switched off (the off loop spins ~1000/s), reaching 220k in a day; the
  float `time` uniform then stuck/jumped = looked like a very low frame rate.
  Large `time` also degrades GPU `sin()` precision: keep the daily restart.
- Matrix: `pwm_bits=8`, `pwm_dither_bits=1`, `pwm_lsb_nanoseconds=130`, `scan_mode=1`,
  `gpio_slowdown=2`, `limit_refresh_rate_hz=90`. Uncapped, refresh wanders 85-151Hz
  (visible flicker). Don't cap at 100Hz: beats against 2x 50Hz mains (rhythmic pulsing).
- Refresh dips come from memory-bus contention (e.g. `dd`) and network traffic, not CPU
  load. The `r8188eu` watchdog caused a dip exactly every 2.00s; `rtw_power_mgnt=0`
  (`/etc/modprobe.d/r8188eu.conf`) mostly fixed it. Remaining flicker needs hardware
  (Ethernet, or a Pi 3/Zero 2 W with on-SoC Wi-Fi; see below).

## Network

- UniFi network, SSID `1337`, APs: UDM, U6-LR-F, U6-LR-B. "Lock 2.4 GHz to Channel 6
  (All APs)" is enabled for the dongle.
- 2026-10-31: U6-LR-F went offline and the cube never reconnected (associates, AP never
  starts the WPA handshake, deauth reason 2). What brought it back: UniFi *Remove* client,
  channel 6 lock, physically re-plugging the dongle into another USB port. Driver reloads
  and reboots alone didn't help.
- zeta's netdata `udp` module also had a `cube` job sending 0..1 values to port 1234,
  clashing with Cubestatspusher; it's commented out in
  `/mnt/user/Apps/netdata/python.d/udp.conf` (netdata logs `udp check failed`, expected).

## Working on it

- Confirm before restarting `led`/`homekit` or rebooting when the user is relying on it;
  `make deploy` compiles but doesn't restart `led`.
- When killing processes over ssh, don't use `pkill -f` with a pattern that also appears
  in your own command line; pipe the script via `ssh cube sudo bash -s <<'EOF'`.
- Hardware upgrade options discussed: Ethernet; Pi 3B/3B+/Zero 2 W (same SD card works);
  ESP32-S3 Matrix Portal S3 as HUB75 driver with the Pi rendering and sending frames
  over SPI (no custom PCB needed).
