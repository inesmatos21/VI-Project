# VI-RT-2526

To compile this project you need to do the following:

```
# Setup dependencies
git submodule update --init --recursive

mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

## Docs

- [Texture Mapping Practical](docs/texture-mapping-practical.md)
- [NVIDIA FLIP Practical](docs/nvidia-flip-practical.md)
- [glTF and Texture Mapping Diagrams](docs/diagrams.md)
