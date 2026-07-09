# Design and Implementation of a Mobile Robot for Search, Rescue, and Structural Diagnostics in Disaster Scenarios
>[!WARNING]
>This is Work in Progress Project at the Tecnológico de Monterrey Campus Puebla.

![banner-RescueBot](https://github.com/user-attachments/assets/00efe0d9-bb26-4452-a0ac-ce617ab37186)



![GitHub last commit](https://img.shields.io/github/last-commit/AlanS1N/Rescue-Bot-FJ2026--Earthquakes?style=for-the-badge)
![GitHub forks](https://img.shields.io/github/forks/AlanS1N/Rescue-Bot-FJ2026--Earthquakes?style=for-the-badge)
![GitHub](https://img.shields.io/github/license/AlanS1N/Rescue-Bot-FJ2026--Earthquakes?style=for-the-badge)

 Table of Contents

- [Project Overview](#project-overview)
- [Screenshots and Progress](#screenshots-and-progress)
- [Circuit Connections](#circuit-connections)
- [Materials](#materials)
- [Future Goals](#future-goals)

---

# Project Overview

This project focuses on the analysis of structural damages and failures caused by disasters (earthquakes, landslides, etc.) to identify operational needs for search and rescue in high-risk zones. Based on these findings, we designed and built a terrestrial mobile robot that _will serve both as a machine_ for entering and exploring without endangering first responders, _and as a tool to diagnose estructural damages_ using artificial computer vision.

The system **will be** validated in a simulated disaster scenario to evaluate technical performance, exploration efficiency, and its utility in supporting rescue missions and structural diagnostics.

📜 Click here to <a href="https://www.overleaf.com/read/qfjzchcjjjqq#3e95bf"> VIEW the paper developed alongside the documentation in Github.</a> (WORK IN PROGRESS)

📷 Click here to<a href="https://1024terabox.com/s/1G64Ih9HwfPtlZVFDbBaQSw"> DOWNLOAD the the DATASET used to train the vision system</a>, provided by Dr. Romeo Ballinas González.

👁️ Click here to<a href="https://1024terabox.com/s/1AKwh-wiAku7Wo5QE4bYNWA"> DOWNLOAD the the VISION SYSTEM LOCAL FILES used to train the vision system.</a> (40.6 GiB)

**Key Features:**
  1. Teleoperated movement.
  2. Video transmission through RF.
  3. Camara movement throguh head tracking.
  4. Trained algorithm to detect and diagnose structural damages.
  5. Rocker bogie suspension.


# Screenshots and Progress

<table>
  <tr>
    <td>
      <img src="https://github.com/user-attachments/assets/5e2d9122-1db5-46ef-8b54-dcb831fe1ec0" width="500"/>
    </td>
    <td>
      <img src="https://github.com/user-attachments/assets/11282c00-a45e-43a1-ad42-39a22b7481df" width="500"/>
    </td>
  </tr>
  <tr>
    <td>
      <img src="https://github.com/user-attachments/assets/7738de3f-24bc-4caf-8920-57d0997f2702" width="500"/>
    </td>
    <td>
      <img src="https://github.com/user-attachments/assets/686f413e-1127-4cba-b7e1-c308003bfae1" width="500"/>
    </td>
  </tr>
</table>

👉 [Click here to see development logs and check out the current progress](./.docs/progress.md)

# Circuit Connections
The complete circuit diagram is shown below:

<img width="1045" height="965" alt="Captura desde 2026-06-09 13-00-33" src="https://github.com/user-attachments/assets/50d19934-df45-4401-9e7f-ec7f88c1c012" />

# Materials

The material list used is listed below:

|          Name           | Units |
|-------------------------|-------|
| JGB37-520B              |   6   |
| Servomotor MG995        |   1   |
| Servomotor MG996        |   1   |
| H Bridge BTS7960 IBT_2  |   2   |
| GY-BNO085               |   1   |
| ESP32 DEVKIT 30 Pines   |   2   |
| Jetson Nano Ori         |   1   |
| FPV Set TS5823Pro       |   1   |
| Logitech C920 Webcam    |   1   |
| EMAX Transporter 2      |   1   |
| FlySky FS-i6x Control   |   1   |
| FlySky FS-iA6 Rx        |   1   |
| PLA Filament            |   1   |
| TPU Filament            |   1   |
| Perf. phenolic board    |   2   |
| Screws(various lengths) |  ±14  |

# Future Goals
This project is planned to be worked on further more with other students at Tecnológico de Monterrey Campus Puebla, under the supervision of Dr. Roberto R. Flores Quintero.

Some current future goals involve:
- 🧠 Deeper training and testing of the computer vision system.
- 🔧 Redisign the structure to for increased stability and access to components
- 🌡️ Adding thermal vision for detecting gases and possible life signals.
- 🕶️ Developing a VR simulated environment.
