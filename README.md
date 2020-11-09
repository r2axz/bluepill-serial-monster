# bluepill-serial-monster

_bluepill-serial-monster_ is a firmware for the _STM32 Blue Pill_ board
that turns it into triple _USB-to-Serial_ adapters. The firmware implements
a USB 2.0 full-speed composite device that consists of 3 independent serial
ports. Operating systems recognize it as three serial interfaces that
can be used simultaneously by different applications.

_STM32 Blue Pill_ is a ridiculously cheap _STM32_ development board which
is available in many stores around the globe. The board contains decent
hardware that supports _USB2.0 Full-Speed_, has 3 independent _USARTs_
and enough processing power to handle high-speed _UART_ communications.

**Note**: some Chinese-made _Blue Pill_ boards have an incorrect pull-up
resistor soldered to the _USB D+_ line (_PA12_) which prevents them from being
successfully detected as a USB device by the host. There is an existing
software workaround for this issue, but it does not deliver reliable results.

This firmware does not contain the workaround for faulty Blue Pill boards and
requires the incorrect resistor to be replaced with the right one. Please refer
to the section [Fixing USB on Blue Pill Boards](#fixing-usb-on-blue-pill-boards)
for more information.

## Features

* 3 independent _UART_ ports;
* Hardware flow control (_RTS_/_CTS_) support<sup>1</sup>;
* _DSR_/_DTR_/_DCD_ signals support;
* 7 or 8 bit word length;
* None, even, odd parity;
* 1, 1.5, and 2 stop bits;
* Works with _CDC Class_ drives on _Linux_, _OS X_, and _Windows_;
* Supports all standard baud rates;
* Supports non-standard baud rates<sup>2</sup>;
* _DMA_ _RX_ / _TX_ for high-speed communication;
* _IDLE line_ detection for short response time;

(1) _UART1_ does not support hardware flow control because _RTS_/_CTS_ pins
(_PA12_, _PA11_) are used for _USB_ communication and cannot be remapped.
If you need hardware flow control, use _UART2_ or _UART3_.

(2) As long as your CDC driver does not hesitate to ask.

## UART Signal Levels

Although _STM32F103C8T6_ installed on the Blue Pill board is a **3.3 V**
device, a number of its inputs are actually **5 V** tolerant.
This means you can safely use the selected inputs with **3.3** and **5 V**
TTL devices.

**Do not use non 5 V tolerant inputs with 5 V devices as doing that will
result in permanent damage of MCU inputs or MCU itself.**

**When configured as an output, none of the _STM32F103C8T6_ pins is 5 V
tolerant. Make sure you don't accidentally get more than 4.0 V on such pin
or damage may occur.**

**5 V** tolerant pins are shown **in bold** in the next section.

## UART Pinout

| Signal |   Direction   |     UART1     |     UART2     |     UART3     |
|:-------|:-------------:|:--------------|:--------------|:--------------|
|   RX   |      IN       |    **PA10**   |      PA3      |    **PB11**   |
|   TX   |      OUT      |      PA9      |      PA2      |      PB10     |
|   RTS  |      OUT      |      N/A      |      PA1      |      PB14     |
|   CTS  |      IN       |      N/A      |      PA0      |    **PB13**   |
|   DSR  |      IN       |    **PB7**    |    **PB4**    |    **PB6**    |
|   DTR  |      OUT      |      PA4      |      PA5      |      PA6      |
|   DCD  |      IN       |    **PB15**   |    **PB8**    |    **PB9**    |

Note: **5 V** tolerant input pins are shown **in bold**.

## Fixing USB on Blue Pill Boards

_STM32 Blue Pill_ boards come in slightly different form-factors. Nevertheless,
their schematic is very similar. Below you will find the instructions on how to
identify and replace the incorrect USB pull-up resistor on any _Blue Pill_ board.

With a digital multimeter, measure the resistance between **PA12** and **3.3 V**
pads on the board. If the resistance reads close to **1.5k** (**1500 ohms**),
then your board is either non-faulty or faulty for some other reason,
and this section does not apply.

If the resistance if far away from **1.5k** (such as **4.7k** or **10k**),
you will have to locate the incorrect resistor on the board and replace it
with a **1.5k** or **1.8k** resistor.

If your board has component names on it, locate **R10**. Otherwise, trace the
board to see where the incorrect resistor is located.

Once you identified the incorrect resistor, replace it with a **1.5k** or
**1.8k** resistor.

## Flashing Firmware

```bash
st-flash --format ihex write bluepill-serial-monster.hex
```

## Building Firmware

### Prerequisites

Install the following software:

* [GNU make](https://www.gnu.org/software/make/)
* [arm-none-eabi toolchain](
    https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)
* [open source st-link](<https://github.com/texane/stlink>)
* [STM32CubeF1](
    <https://www.st.com/en/embedded-software/stm32cubef1.html>)

Here is an example assuming everything is installed in ~/stm32/,
and we use _bash_:

ARM toolchain and st-link must be added to PATH.

```bash
# add ARM toolchain path
export PATH=~/stm32/gcc-arm-none-eabi/bin:$PATH
# add stlink path
export PATH=~/stm32/stlink-install/bin:$PATH
```

Path to STM32CubeF1 should be also exported (use ~/.bash_profile):

```bash
# export STM32Cube
export STM32CUBE_PATH=~/stm32/stm32cube
```

## Building

To build the firmware, **cd** to the project directory and run

```bash
make
```

To flash the MCU using st-link, run

```bash
make flash
```

To remove object and dependency files, run

```bash
make clean
```

To remove object, dependency, and firmware files, run

```bash
make distclean
```
