# ⚙️ Stepper Sequence Runner — PIC16F877A

A programmable stepper motor control system built using the **PIC16F877A microcontroller**, featuring **4×4 keypad input** and **SSD1306 OLED display**.  
This system enables users to configure, store, and execute multi-step motor sequences with precision and flexibility.

---

## 📌 Overview

This project focuses on designing a **real-time embedded control system** for stepper motors using interrupt-driven architecture.  
It supports multiple motion patterns and provides an interactive user interface for sequence programming.

---

## 🛠️ Hardware Used

| Component                | Purpose                               |
|------------------------|-------------------------------------|
| PIC16F877A             | Main microcontroller (20 MHz crystal) |
| 28BYJ-48 Stepper Motor | Load                                |
| ULN2003A Driver Board  | Motor driver                        |
| 4×4 Matrix Keypad      | User input interface                |
| SSD1306 OLED Display   | Real-time display                   |

---

## ⚙️ Features

- Supports multiple step types: Angle, Rotations, Intermittent, Timed  
- Real-time control using keypad interface  
- OLED display for user feedback  
- Timer1 interrupt-based motor control  
- Non-blocking execution  
- Emergency stop functionality  
- Sequence save and reload feature  
- Smooth operation using half-step control  

---

## 🧠 Technical Concepts

- Embedded C Programming  
- Interrupt Handling  
- Timer-based Control  
- Software I2C Communication  
- Real-time Embedded Systems  

---

## 📊 Key Achievements

- Achieved precise control using **4096 half-steps per revolution**  
- Implemented efficient memory usage within 8-bit constraints  
- Developed a responsive and interactive embedded system  
- Enabled dynamic sequence execution with repeat control  

---

## 📅 Project Details

- Course: Microcontroller and its Applications (4 Credits)  
- Duration: March 2026  

---

## 👨‍💻 Team Members

- Navin Rubag V P  
- V R Bhirugudev  
- Harie Goutaym  
- Shreyas Vijay  

---

## 🔗 Original Repository

This project is based on a collaborative implementation.  
Original source: https://github.com/HarieGoutaym/pic16f877a-motor-control
