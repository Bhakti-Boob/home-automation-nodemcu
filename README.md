# Home Automation Using NodeMCU (SinricPro + Local Switches)
Wi‑Fi based home automation system using NodeMCU, SinricPro cloud and relay modules to control a light and a fan via Google Assistant as well as local manual switches.

# Overview
This project implements an IoT home automation that exposes two smart switches (Light and Fan) to SinricPro and maps them to physical relay channels and local rocker switches. The NodeMCU connects to Wi‑Fi, maintains a persistent SinricPro connection and keeps the state of each appliance synchronized between cloud commands and local switch actions. Local switches are debounced in software and any manual toggle is reported back to SinricPro so voice assistants always see the correct ON/OFF state.

<div align="center">
  <video src="https://github.com/user-attachments/assets/380e7c70-8a74-4858-a4ee-e6b9dbafe219" width="400" controls></video>
</div>

# Features
- Controls two AC loads (Light and Fan) via relay module using NodeMCU 
- Integration with SinricPro for Google Assistant voice control 
- Dual control path:  
  - Cloud → SinricPro → NodeMCU → Relay 
  - Local flip switch → NodeMCU → Relay  
- Debounced local switches with 250 ms window to avoid contact bounce  
- Wi‑Fi status LED indicating successful network connection

# System Architecture
- Google Assistant sends voice commands via internet to SinricPro  
- SinricPro forwards switch state changes to the Wi‑Fi‑enabled NodeMCU 
- NodeMCU drives the relay module, which switches AC power to the home appliances (light and fan) 
- Local switches connected to GPIOs allow manual override and their state is pushed back to SinricPro
<div align="center">
  <img width="600" height="300" alt="Block_diagram" src="https://github.com/user-attachments/assets/87158269-77df-438f-b177-3baec6b13026"><img/>
</div>

# Hardware and components used
- NodeMCU powered from a 5 V adapter 
- 2x Relay module 
- 2 x Local switches 
- LED (for Wi-Fi indication)
- AC mains supply  

# Getting Started

1. SinricPro setup  
   - Create an account on SinricPro and add two Switch devices (Light and Fan)  
   - Copy their device IDs into `device_ID_1` and `device_ID_2` 
   - Obtain `APP_KEY` and `APP_SECRET` from SinricPro and update the sketch

2. Arduino IDE 
   - Install the SinricPro and ESP8266 board packages  
   - Open the `.ino` file from this repository  
   - Set board to “NodeMCU” and correct COM port  

3. Flashing and test  
   - Upload the sketch to NodeMCU  
   - Verify that the Wi‑Fi LED turns off once connected and that the serial monitor prints the obtained IP address  
   - From SinricPro or Google Assistant, issue commands like “Turn on Room Light” and confirm the corresponding relay and appliance react  
   - Toggle the local switches and confirm the relays change state and the SinricPro app reflects the new states

# How to Use This Repository
- `/src/` – Sketch for NodeMCU + SinricPro + local switches  
- `/circuit_diagrams/` – Schematics, wiring diagrams
- `/docs/` – Detailed technical report, photos and videos of the assembled prototype
