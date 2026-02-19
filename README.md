# ROD

## Summary
- [Description](#description)
- [Documentation](#documentation)

## Description
As part of the [Eurobot](https://www.eurobot.org/) competition and the [French Robotics Cup](https://www.coupederobotique.fr/), robots compete against each other in matches on a game table. The competition rules allow the use of a Remote Observation Device (ROD): a system equipped with a camera that captures images of the game table and transmits information to the robot.

The objectives of this ROD project are:
- Recognition of ArUco TAGs on game elements using a camera mounted above the table
- Ability to precisely locate game elements on the playing field 
- Transmission of game element positions to a robot

This repository is a simple proof of concept of such a ROD using a Raspberry Pi and a Pi Camera. It includes code for camera calibration, image processing, ArUco tag detection, and position calculation. The proof of concept is written in Python and in C and use openCV libraries.

## Documentation
- [How to set up the raspberry pi environment](docs/how-to-set-up.md)
- [How to connect to the raspberry pi](docs/how-to-connect.md)
- [How to test the camera quickly](docs/how-to-test-the-camera.md)
- [How to calibrate the camera](docs/how-to-calibrate-the-camera.md)
- [Eurobot 2026 rules summary](docs/2026-eurobot-rules.md)
- [Eurobot general rules summary](docs/general-eurobot-rules.md)
- [ROD C](rod_c/README.md)
- [ROD Python](rod_python/README.md)