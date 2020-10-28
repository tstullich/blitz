# blitz
A deferred renderer written in C++ and Vulkan. This will be the repository for a deferred
renderer that I am currently writing. Follow along with the development by
starring this repository or on on the blog of my [website](https://tstullich.github.io/posts).

## Installation
### Dependencies

To install you will need the following tools:

- CMake 3.16
- A C++ compiler toolchain with support for C++14

Make sure to have the following libraries installed and visible to CMake

- Vulkan SDK 1.2
- GLM 0.9.9.7
- GLFW 3.3

### Building

To build execute the following commands from the root of the repository

```
mkdir build
cd build
cmake ..
make && make install
```

The `blitz` binary should then be found in the `bin` folder from the repository root which
can then be executed to run the renderer.

## Features
- [x] Indexed draw mode
- [x] Texture support w/ mip map creation at runtime
- [x] Depth stencil testing
- [x] Multisampled anti-aliasing
- [x] Blinn-Phong shading model

## Future Work 
- [ ] UI improvements
- [ ] GLTF scene loading
- [ ] Physically-based lighting
- [ ] Deferred shading

## Progress Screenshots
![An image of a model with diffuse shading and a single point light](https://i.imgur.com/FMQnDi1.png)
An image of a model with diffuse shading and a single point light

![An image of a cube rendered with a texture](https://i.imgur.com/5OazHcA.png)
The first cube rendered with texture support

![An image of a quad being rendered through vertex indices](https://i.imgur.com/aqgoO4Q.png)
The very first quad that was rendered
