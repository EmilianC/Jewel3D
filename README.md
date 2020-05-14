![Gemcutter Logo](docs/Logo.png)

# What is Gemcutter?
Gemcutter is a free C++ game development framework for Windows. It features a powerful Entity-Component-System
and fine-grain control of both 2D and 3D rendering.

# Why use Gemcutter?
The framework facilitates expressive and succinct code that is both easy to read, and to use.
With a thin API and C++20 features, Gemcutter provides powerful features without feeling clunky.

Highlight features include:
* [A rich shader workflow](docs/Shader.md)
* [Efficient entity queries](docs/Entity.md)
* [Extendable asset management](docs/AssetManager.md)
* A modular and extendable particle system
* Audio powered by OpenAL
* Mesh and Texture loading
* [GUI support](docs/GUI.md)
* [FreeType text rendering](docs/Text.md)
* Safe function delegates and event management
* Robust fixed-timestep updates
* A complete math library
* Keyboard/Mouse/Xbox-Controller support
* Local networking

# Quick Start
If you're looking to start a new project using Gemcutter, then you can use [this repository template](https://github.com/EmilianC/Gemcutter-Project-Template) to get started quickly.

If you just want to download the framework, then simply clone the repo and configure the project with CMake from the root directory:
```
> mkdir build
> cd build
> cmake ..                       # Default configuration, or
> cmake .. -DENABLE_SAMPLES=OFF  # Default configuration without downloading samples
```
And that's it! You'll find your generated projects under the `build/` folder. Code samples can be found under `samples/` if they were enabled.

Any git submodules which are required by your configuration are automatically pulled, so feel free to omit them when cloning.

# Upcoming Features
Gemcutter is always being updated and improved. See the [Trello board](https://trello.com/b/Oc2GFT2A/gemcutter) to follow development.

Future highlight features include:
* A fully featured world/UI editor
* Bullet physics integration
* Configurable memory management
* C++ reflection driven features:
	* Animation of arbitrary values
	* Serialization and deserialization
	* Replication over networks
	* Seamless editor integration of user data-types

# Dependencies
All Dependencies used under their respective licenses. Copyright is held by their respective owners.
* [GLEW 2.0.0](http://glew.sourceforge.net/)
* [FreeType 2](https://www.freetype.org/)
* [Catch 2.7.2](https://github.com/philsquared/Catch)
* [OpenAL Soft 1.21.1](https://github.com/kcat/openal-soft)
* [Scaffold](https://github.com/EmilianC/scaffold)
* [SOIL ext](https://github.com/fenbf/SOIL_ext)
* [dirent 1.23.1](https://github.com/tronkko/dirent)
* [XInput 1.4](https://msdn.microsoft.com/en-us/library/windows/desktop/ee417001(v=vs.85).aspx)

# Notes
* Gemcutter is built and tested on Windows with the latest version of Visual Studio.
* Gemcutter requires at least OpenGL 3.3.
* Gemcutter is provided under the MIT License.
