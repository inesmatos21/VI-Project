# Texture Mapping Practical

## Goal

Complete the texture sampling implementation in `src/Primitive/Material.hpp` so imported glTF textures affect the rendered material.

The glTF importer already loads:

- vertex positions from `POSITION`
- normals from `NORMAL`
- texture coordinates from `TEXCOORD_0`
- base color textures
- metallic-roughness textures
- texture wrap and filter modes

The missing part is converting an interpolated UV coordinate into a texture color.

## Where To Work

Implement the TODO inside `TextureSampler::Sample`:

```cpp
RGB Sample(const Vec2& uv) const;
```

The helper functions `ApplyWrap`, `SampleNearest`, and `SampleLinear` are already implemented and commented. Read them first, then use them from `Sample`.

Before working on the sampler, read `Triangle::Intersect` in `src/Primitive/Geometry/Triangle.cpp`. The intersection code computes the UV coordinate at the ray hit point by interpolating the three vertex UVs with barycentric weights:

```cpp
const Vec2 tex_coord = (1.0f - u - v) * m_UV1 + u * m_UV2 + v * m_UV3;
```

That interpolated `tex_coord` is stored in the `Intersection` and later passed to the material.

## Expected Behavior

`Sample` should:

1. Apply the S wrap mode to `uv.x`.
2. Apply the T wrap mode to `uv.y`.
3. Use nearest filtering when `Filter == TextureFilterMode::Nearest`.
4. Use linear filtering otherwise.

The provided `ApplyWrap` supports:

- `Repeat`: keep only the fractional part.
- `ClampToEdge`: clamp to the `0..1` range.
- `MirroredRepeat`: repeat every integer interval, reversing direction every other interval.

The provided `SampleNearest`:

- convert normalized `u, v` coordinates to integer image coordinates
- clamp the result to the valid image range
- return `Data.Get(x, y)`

The provided `SampleLinear`:

- convert normalized `u, v` to floating-point image coordinates
- find the four neighboring texels
- interpolate horizontally, then vertically

## How The Result Is Used

The material functions already call the sampler:

```cpp
return m_Albedo * m_AlbedoTexture->Sample(uv);
return glm::clamp(m_Roughness * m_MetallicRoughnessTexture->Sample(uv).g, 0.0f, 1.0f);
return glm::clamp(m_Metallic * m_MetallicRoughnessTexture->Sample(uv).b, 0.0f, 1.0f);
```

That means:

- base color textures multiply the material albedo
- roughness uses the green channel of the metallic-roughness texture
- metallic uses the blue channel of the metallic-roughness texture

When the exercise is complete, textured glTF models should no longer render as plain material colors.
