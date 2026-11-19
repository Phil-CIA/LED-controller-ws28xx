# Rev B: Get your lights working

**Already running WLED?** Skip the install. For direct control, join the board's
Wi-Fi and open **[http://4.3.2.1](http://4.3.2.1)**. No router or app required.

## 1. Install WLED (new board only)

1. Leave the LED strip disconnected for first setup. Connect the Rev B ESP32
   USB programming connection to your computer with a data cable. Unplug other
   ESP32s so you can't select the wrong one.
2. On the computer, open the [official WLED installer](https://install.wled.me)
   in Chrome or Edge. Select **ESP32** (classic ESP32, not S2/S3/C3), select
   the board's serial port, and choose **Install**. Skip router setup if you
   want to use the board directly.
3. Join the board's Wi-Fi as described below, then load the starter files.
   Stock WLED is enough for the strip, buttons and phone control. The AHT20
   and independent status LEDs need the optional Rev B firmware.

## 2. Connect your phone or computer

| What you want | What to do |
| --- | --- |
| **No router** | In Wi-Fi settings, join **WLED-AP** (or the board's assigned hotspot name). The factory password is `wled1234`. Choose **Stay connected** if warned there is no internet. Open **[http://4.3.2.1](http://4.3.2.1)** in a browser. |
| **On your router** | From the board's hotspot page, go to **Config > WiFi & Network**, enter your 2.4 GHz network name and password, and save. Put your phone back on that network and use the WLED app to discover the board, or open its router-assigned IP in a browser. |

The board serves the same WLED controls either way. When directly connected,
use the browser, or manually add `4.3.2.1` if your WLED app offers manual
add. If the app can't find it, use the browser.

For an existing board, **Config > WiFi & Network > AP opens** controls when the
hotspot is available. Choose **Disconnected** for fallback when the router
isn't reachable, or **Always** to keep the hotspot available even when the
board is on the router. **Always** doesn't stop it joining a saved router.
For hotspot-only use, clear the saved router network name(s), select
**Always**, save, and reconnect to the board's hotspot. Set your own AP
password before leaving the hotspot on. If the board isn't reachable and
its AP is set to **No connection after boot**, power-cycle it away from the
saved router and give it a moment to start the hotspot.

## 3. Load the Rev B settings (new board only)

Download these files **before** switching to the board's Wi-Fi. On GitHub,
open each file and use **Download raw file** to save the JSON.

| File | What it sets up |
| --- | --- |
| [`cfg.json`](wled/setup/cfg.json) | Main strip on GPIO16 (50 RGB WS281x pixels as a starting point), button pins, I2C pins, and hotspot access. The strip starts off and optional usermods stay disabled. |
| [`presets.json`](wled/setup/presets.json) | Button actions: dim, brighten, next effect, and toggle power. |

1. Open WLED and go to **Config > Security & Updates**.
2. Under **Restore configuration**, choose `cfg.json` and press **Upload**.
   WLED reboots. Rejoin the board's hotspot and reopen `http://4.3.2.1`.
3. Under **Restore presets**, choose `presets.json` and press **Upload**.
4. In **Config > LED & Hardware**, check the strip's pixel count, color order
   and current limit against your setup. Connect the strip with power off.
   Power it back up and try WLED's **Power** button at low brightness.

These are starter files without personal network settings, not a backup of
the working board. They overwrite existing settings and presets. On an
already-configured board, use **Backup configuration** and **Backup presets**
on the same page before restoring anything, and keep the passwords separately.
Set up the router connection after the restore if you want one.

## Optional: Rev B sensor and status LEDs

If **Config > Usermods** already shows **RevBController**, you have the custom
firmware. If not, get the Rev B application `.bin` from the board maintainer
or [build it here](WLED_REV_B_PLAN.md#build-the-combined-firmware-no-upload).
There is no published Rev B firmware download yet. With compatible ESP32
WLED already installed, back up your settings and use **Config > Security &
Updates > Manual OTA Update** to upload that `.bin`. It is an OTA application
image, not a blank-board USB installer. Leave firmware validation enabled.

With the starter config loaded, enable **Sensor Enabled** and **Indicators
Enabled** in **Config > Usermods** and save. GPIO12 is reserved for the three
status LEDs; don't add it as a strip output. Leave **Fan Enabled** off.

---

**PCB QR code:** Point it at the [public repository README](https://github.com/Phil-CIA/LED-controller-ws28xx)
once this guide is on the default branch. The link needs internet, so scan
and save the guide/files before joining the hotspot. Print
**WLED: `http://4.3.2.1`** beside the QR for offline use.
