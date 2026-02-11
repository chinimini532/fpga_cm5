# Modern FPGA Configuration on Raspberry Pi 5
A Practical Userspace Bitstream Loader for Spartan-3E

------------------------------------------------------------

1. Overview
------------------------------------------------------------

This project demonstrates how to configure a Xilinx Spartan-3E FPGA
directly from a Raspberry Pi 5 or Compute Module 5 using:

- libgpiod v2 (modern GPIO interface)
- Userspace bit-banging
- INIT/DONE monitoring
- H16 bitstream header validation
- Optional SPI register access

Raspberry Pi 5 uses the RP1 I/O controller.
Legacy GPIO methods such as /dev/mem and sysfs are no longer suitable.
This project provides a modern, reliable approach.

------------------------------------------------------------

2. Tested Hardware
------------------------------------------------------------

Raspberry Pi:
- Raspberry Pi 5
- Raspberry Pi Compute Module 5 (CM5)

FPGA:
- XC3S100E
- XC3S250E
- XC3S500E
- XC3S1200E
- XC3S1600E

Operating System:
- Raspberry Pi OS-lite (64-bit)
- libgpiod v2

------------------------------------------------------------

3. GPIO Wiring
------------------------------------------------------------

The following BCM GPIO mapping is used:

+----------------+----------+
| FPGA Signal    | BCM GPIO |
+----------------+----------+
| CCLK           | 25       |
| INIT           | 27       |
| DONE           | 22       |
| DIN            | 24       |
| PROGRAM        | 23       |
+----------------+----------+

Ensure proper voltage compatibility (3.3V logic).

------------------------------------------------------------

4. H16 Bitstream Format
------------------------------------------------------------

The loader expects an H16 formatted file:

Offset   Size    Description
-----------------------------------------
0x00     4       "FPGA"
0x04     4       Payload size (big-endian)
0x08     4       Checksum (sum of payload bytes)
0x0C     4       Padding + board ID
0x10     N       Raw FPGA bitstream

The program validates:
- Magic header
- File size
- Checksum integrity

------------------------------------------------------------

5. Build Instructions
------------------------------------------------------------

Dependencies:

- gcc
- make
- libgpiod (v2)

Install libgpiod:

sudo apt update
sudo apt install libgpiod-dev

Build the project:

make

After running "make", the compiled binary will be created at:

bin/fpga

You can verify:

ls bin/

------------------------------------------------------------

6. Running the Program
------------------------------------------------------------

Because GPIO and SPI require root privileges,
the program must be executed using sudo.

General format:

sudo ./bin/fpga [option]

------------------------------------------------------------

6.1 Read FPGA Register
------------------------------------------------------------

Read a single register (example: 0x0A):

sudo ./bin/fpga -r 0A

Read all registers:

sudo ./bin/fpga -r all

------------------------------------------------------------

6.2 Write FPGA Register
------------------------------------------------------------

Write value 0x21 to register 0x08:

sudo ./bin/fpga -w 08 21

------------------------------------------------------------

6.3 Program (Configure) the FPGA
------------------------------------------------------------

Place your H16 bitstream file in the current directory.

Then run:

sudo ./bin/fpga -p your_file.fbin

Example:

sudo ./bin/fpga -p example.fbin

Optional debug level (0-4):

sudo ./bin/fpga -p example.fbin 2

If configuration succeeds, the program will print:

SUCCESS

If DONE remains low, configuration has failed.

------------------------------------------------------------

7. Programming Sequence
------------------------------------------------------------

1. PROGRAM is driven low to reset FPGA
2. INIT is monitored
3. Bitstream is shifted LSB-first on DIN
4. CCLK toggled for each bit
5. INIT checked after Device ID
6. DONE checked at completion

If DONE is high, configuration is successful.

------------------------------------------------------------

8. Project Structure
------------------------------------------------------------

src/        Source files
include/    Header files
Makefile    Build system
bitstreams/ Example location for user bitstreams

Note:
Proprietary or company-specific bitstreams are not included.

------------------------------------------------------------


9. Disclaimer
------------------------------------------------------------

Ensure proper electrical connections.
The author is not responsible for hardware damage.
