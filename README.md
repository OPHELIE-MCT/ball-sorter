# Ball Sorter

Arduino-based ball sorting project using color classification and distance sensors.

## Overview

This repository contains the firmware for a ball sorter that:

- detects ball presence with VL6180X time-of-flight sensors,
- reads color data from an AS7341 sensor,
- classifies the ball color with the embedded classifier,
- drives a servo to route the ball,
- runs a DC motor and shows status on a NeoPixel ring.

## Files

- `ball-sorter.ino`: main Arduino sketch.
- `classification.h`: classifier interface and prediction types.
- `classification.cpp`: embedded color classification implementation.

## Microcontroller

The firmware is designed for an Arduino Nano based board (currently using a Seeeduino Nano), but it should work on any AVR board supported by the Adafruit TiCoServo library.
