# Lantern glTF Sample

Source: <https://github.com/KhronosGroup/glTF-Sample-Models/tree/main/2.0/Lantern/glTF>

The files in `glTF/` mirror the Khronos glTF Sample Models Lantern asset:

- `Lantern.gltf`
- `Lantern.bin`
- base color, emissive, normal, and metallic-roughness textures

Run it with:

```sh
./build/VI-RT --scene scenes/Lantern/glTF/Lantern.gltf --spp 1
```

Notes:

- This is a compact, core-glTF sample with textured materials and no required extensions.
- The model does not include a glTF camera, so the renderer falls back to the default camera from `main.cpp`.
- It is much lighter than Sponza and better suited for quick classroom demos.
- At the current hard-coded `800x600` render size, a one-sample render can still take a few seconds.
