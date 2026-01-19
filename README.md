# 3 DOF Robot Manipulator – Embedded Control System

## Overview
This project consists of an embedded control system for a 3 DOF robotic manipulator.
The system was developed focusing on firmware design, hardware–software integration,
and closed-loop control.

## System Architecture
- PIC microcontrollers as slave nodes for motor control
- Arduino as master controller
- I2C communication between devices
- Serial communication for monitoring and debugging

## Hardware
- PIC microcontrollers
- Arduino board
- DC motors
- INA219 current sensors
- Motor drivers

## Software
- Firmware developed in C for PIC microcontrollers
- PID control algorithms for position control
- I2C communication drivers
- Python interface for sending coordinates and monitoring data

## Key Features
- Independent PID control for each joint
- Multi-device I2C bus management
- Real-time current measurement using INA219 sensors
- Forward and inverse kinematics implementation

## Testing & Validation
- Functional testing for each joint
- PID tuning and stability analysis
- Positioning error below 5% after tuning

## Notes
This project was developed as an academic project focused on embedded systems,
control algorithms, and hardware–software integration.
