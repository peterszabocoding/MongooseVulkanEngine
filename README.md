# MongooseVK - simple Vulkan renderer

This project is a real-time 3D renderer built from scratch using modern C++ and Vulkan. 
The goal is to build an independent renderer that could be integrated into larger game engine project.

![Mongoose VK Demo application!](/docs/images/mongoose_vk_demo.jpg "San Juan Mountains")

## Implemented Features

* **GLTF Scene Loading:** Ability to load 3D scenes from GLTF files, including meshes, submeshes, and associated materials.
* **Physically Based Rendering (PBR):** Implemented using the metallic-roughness workflow for realistic material representation.
* **Image-Based Lighting (IBL):** Scenes are lit using an HDR environment texture for global illumination.
* **Single Directional Light:** Support for a single, configurable directional light source.
* **Infinite Grid:** A visual aid for the ground plane that extends infinitely.
* **Cascaded Shadow Maps**
* **Simple Screen Space Ambient Occlusion (SSAO)**
* **Bindless Textures:** Utilizes bindless resources for flexible and efficient texture access in shaders.
* **SPIR-V runtime shader compilation**


## Planned Features and Changes

* **Render Pass redesign and Frame Graph Implementation:** A major overhaul of the rendering pipeline to introduce a more flexible and efficient render pass system, potentially using a frame graph for explicit resource dependencies.
* **Point and Spot light support:** Addition of various light types beyond the current directional light.
* **Shadow Map atlas:** Optimization for shadow rendering by packing multiple shadow maps into a single texture atlas.
* **MSAA - Multi-Sampling Anti Aliasing**
* **Tone mapping**
* **Post processing effects like SSR and motion blur**


## Libraries Used
* **GLFW:** Open Source, multi-platform library for OpenGL, OpenGL ES and Vulkan development on the desktop. It provides a simple API for creating windows, contexts and surfaces, receiving input and events.
* **ImGui:** A bloat-free immediate mode graphical user interface for C++ with minimal dependencies. Used for debugging and tool development.
* **spdlog:** A very fast, header-only C++ logging library.
* **GLM:** OpenGL Mathematics library for vector and matrix operations.
* **stb_image:** A single-file public domain library for loading images.
* **tinyobjloader:** A C++ single file wavefront obj loader.
* **tinygltf:** Single file GLTF loader to load complete scenes with materials.
* **VulkanMemoryAllocator (VMA):** A library for simplifying memory management in Vulkan.

## Resources Used for Development

* [**Vulkan-Tutorial**](https://vulkan-tutorial.com/)
* [**Vulkan examples from Sascha Willems**](https://github.com/saschawillems)
* [**LearnOpenGL.com**](https://learnopengl.com/)
* [**TheCherno's Hazel Engine project**](https://github.com/TheCherno/Hazel)

* [**Physically Based Rendering: From Theory To Implementation**]()<br>
* [**Mastering Graphics Programming with Vulkan**](https://www.amazon.com/Mastering-Graphics-Programming-Vulkan-state/dp/1803244798)<br>
* [**Vulkan 3D Graphics Rendering Cookbook**](https://www.amazon.com/Vulkan-Graphics-Rendering-Cookbook-high-performance/dp/1803248114)<br>