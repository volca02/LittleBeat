# LittleBeats

This is a standalone drum sound synthesizer project, based on the code from Mutable Peaks drum section.

The project is licensed under MIT license, in accordance to the Peaks original source code license.

The project runs on ESP32, with MIDI in on Serial2 RX (GPIO16), I2S connection to PCM5102 on BCK:GPIO26, LCRK:GPIO25, DATA:GPIO22.

The display is SPI, based on SH1106, with RST:GPIO4, DC:GPIO2 and CS pulled permanently low (GND). SCK:GPIO18, SDA:GPIO23.

The rotational encoder is on KEY:GPIO13, S1:GPIO12, S2:GPIO14.

The plan is to finalize the project, then model a 3D printed box for the project to reside in.
