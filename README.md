# Movement Simulator
GPU accelerated human movement simulator.

## Building
### Requirements

#### Fedora
```
sudo dnf install gtkmm40-devel libcurl-devel gcc cmake git
sudo dnf install mesa-libEGL-devel vulkan-devel glslc
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
