# gliewer

## What is gliewer ?

gliewer is my attempt at writing a small 3D engine using as little dependencies as possible.
The current dependencies are :
- OpenGL
- SDL2 (for window and input management)
- [ImGui](https://github.com/ocornut/imgui) (for GUI)
- [stb] (https://github.com/nothings/stb) (for image loading / planned to get rid of this dependency)
- [tinyobjloader](https://github.com/syoyo/tinyobjloader) (for mesh loading / planned to get rid of this dependency)

This project is written using C++ but I am using only a very small subset of C++ features so that the code is as close to C as possible. The code is written and tested on Windows and Linux.

## PBGI

In September 2016, I began working on a project with [Telecom ParisTech's Computer Graphics group](http://www.tsi.telecom-paristech.fr/cg/). The goal is to implement basic global illumination methods and analyze how they work and perform to see where most of the bottlenecks are.
I am using this very engine to implement these algorithms as it already provides the main features needed. 

## Current features

- OBJ / OFF file loading
- Arcball camera to rotate and zoom around mesh
- Basic GUI display to tweak rendering parameters
- GPU implementation of several BRDF (Lambert, Blinn-Phong, GGX)
- Shadow mapping with PCF
- Cubemap rendering
- Texture GPU post-processing (Gaussian blur, ...)
- G-buffer implementation for defered shading
- Framebuffer / depth-buffer reading to unproject pixel to world coordinates
- Instancing

## To do features

- Cascading shadow maps
- Texture mapping
- Profiler
- Bloom effects
- Gamma correction
- Automatic shader reloading
- Texture packing
- Mesh simplification (see my other project [meshrekt](https://github.com/rivten/meshrekt))
- Surface segmentation
- Screen-space ambient occlusion
- Motion blur
