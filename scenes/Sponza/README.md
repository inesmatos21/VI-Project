# Sponza glTF Sample

Source: <https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Sponza/glTF>

The files in `glTF/` mirror the Khronos glTF Sample Models Sponza asset:

- `Sponza.gltf`
- `Sponza.bin`
- referenced texture images

Run it with:

```sh
./build/VI-RT --scene scenes/Sponza/glTF/Sponza.gltf --spp 1
```

Notes:

- The Khronos Sponza package does not include lights in the model.
- It also does not include a glTF camera, so the renderer falls back to the default camera from `main.cpp`.
- The model is much heavier than `cornell-box.gltf`; use low sample counts for classroom demos.
- See the upstream Sponza README for source and licensing details.
