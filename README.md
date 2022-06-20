# Movement Simulator
GPU accelerated human movement simulator.

## Building
### Requirements

#### Fedora
```
sudo dnf install gtkmm4.0-devel libadwaita-devel libcurl-devel g++ clang cmake git
sudo dnf install mesa-libEGL-devel vulkan-devel glslc vulkan-tools glslang
```

If you want to enable support for debugging Vulkan shaders with [RenderDoc](https://renderdoc.org/) via setting the `MOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API` to `ON` during CMake configuration (e.g. `cmake .. -DMOVEMENT_SIMULATOR_ENABLE_RENDERDOC_API=ON`) the following needs to be installed as well:
```
sudo dnf install renderdoc-devel
```

### Compiling
```
git clone https://github.com/COM8/movement-sim.git
cd movement-sim
mkdir build
cd build
cmake ..
cmake --build .
```

### Installing
```
sudo cmake --build . --target install
```
