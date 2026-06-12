# Ball Sorter

Firmware repository for the Seeeduino Nano ball-sorting subsystem.

## What lives here

- the sorter sketch that drives the motor, servo, NeoPixels, and local ToF sensors
- the embedded AS7341 color classifier used during sorting
- the reference feature order and class list used by the calibration workflow

## Calibration link

The embedded centroids in `classification.cpp` are regenerated outside this repo:

- samples are captured with the Uno Q sketch in `M2614_LaFaceCacheeDeLaLune`
- labeled rows are written by `M2614_LaFaceCacheeDeLaLune-Python`
- the Uno Q SBC web app can now run the centroid analysis locally, save the generated plots, and display the final C array to paste back into this repo
- `ball-analyzer/analysis.ipynb` remains the offline reference implementation and fallback tool

## Target hardware

The firmware is currently aimed at a Seeeduino Nano class board and the sorter hardware used in the M2614 project.
