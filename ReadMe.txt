# COMP 371 – Assignment 2: Dynamic Lighting 3D Scene

Authors:  
- Marc El Haddad – ID: 40231208  
- Mostafa Mohamed – ID: 40201893  
- Joseph Keshishian – ID: 40297447  
- Ahmad Al Habbal – ID: 40261029  

---

## Description
This project is an interactive 3D scene featuring:
- A textured 3D model as the main object.
- A moon object orbiting around the main model with hierarchical animation.
- Phong shading with dynamic lighting.
- Shadow mapping for realistic lighting.
- Ability to toggle textures, pause light movement, and change light colors.

---

## Controls
W / A / S / D - Move camera  
Mouse - Look around  
T - Toggle textures on/off  
L - Pause or resume light movement  
M - Change light color  

---

## Features
- Complex 3D models loaded from `.obj` files.
- Phong lighting model with ambient, diffuse, and specular components.
- Dynamic lighting with moving light source.
- Hierarchical animation (moon orbit and spin).
- Shadow mapping with depth framebuffer.
- Texture toggling via keyboard.
- Light color changes in real time.
- Light pause and resume toggle.

---

## How to Run

### Requirements
- Windows with MSYS2 MinGW64 installed.
- GLEW, GLFW, and GLM libraries (included in `include/` and `lib/` folders).
- stb_image.h (included in `src/`).

### Build and Run
1. Open MSYS2 MinGW64.
2. Navigate to the project folder:
   ```bash
   cd /c/Users/mosta/Desktop/COMP371-FinalProject
   assignment2.cpp -o assignment2.exe -Iinclude -Llib -lglfw3 -lglew32 -lopengl32
   ./assignment2.exe

---




