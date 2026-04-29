# Scene Assets

`cornell-box.gltf` is generated for this repository and is self-contained. It includes:

- base-color textures on the floor and objects;
- a metallic-roughness texture test object;
- an emissive ceiling mesh light.

Recommended external classroom assets from Khronos glTF Sample Assets:

- Water Bottle: CC0, compact, and focused on core metallic-roughness materials.
- Flight Helmet: CC0 and visually richer geometry/textures, but usually used as a multi-file glTF folder.
- Toy Car: CC0 and visually rich, but uses extensions such as transmission, clearcoat, and sheen that this renderer does not support yet.

Avoid bundling Damaged Helmet by default because current Khronos listings include CC BY-NC source credit.

External object-only models may render dark unless placed in a lit scene, because the renderer currently imports mesh materials but not glTF punctual lights or environment lighting.
