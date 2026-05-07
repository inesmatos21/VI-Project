# NVIDIA FLIP Practical

## Goal

Use NVIDIA FLIP to compare two rendered images and understand where the visible errors are.

FLIP is an image difference tool made for rendered images. It does not only ask whether two pixels are numerically different. It tries to estimate how visible the difference is to a viewer, then produces an error map that shows where the important differences are.

In this project, the renderer writes PPM files:

- `image.ppm` for the rendered image
- `image-OIDN.ppm` for the denoised image, when denoising is enabled

The Python FLIP command-line tool expects PNG or EXR images, so this practical converts the project output from PPM to PNG before running FLIP.

Reference:

- [NVlabs/flip](https://github.com/NVlabs/flip)

## Setup

Install FLIP and Pillow:

```sh
python -m pip install flip-evaluator pillow
```

If your system uses `python3` instead of `python`, use:

```sh
python3 -m pip install flip-evaluator pillow
```

Check that FLIP is available:

```sh
flip -h
```

The important command shape is:

```sh
flip --reference reference.png --test test.png
```

The reference image is the image you trust more. The test image is the image you want to evaluate.

## Practical Example

This example compares a noisy low-sample render against a cleaner high-sample render.

First build the project if needed:

```sh
cmake --build build -j16
```

Render a noisy test image:

```sh
./build/VI-RT --spp 8
```

Convert the PPM render to PNG:

```sh
python -c "from PIL import Image; Image.open('image.ppm').save('test.png')"
```

If your system uses `python3`, use:

```sh
python3 -c "from PIL import Image; Image.open('image.ppm').save('test.png')"
```

Now render a cleaner reference image:

```sh
./build/VI-RT --spp 128
```

Convert that render to PNG:

```sh
python -c "from PIL import Image; Image.open('image.ppm').save('reference.png')"
```

Or, with `python3`:

```sh
python3 -c "from PIL import Image; Image.open('image.ppm').save('reference.png')"
```

Compare the two images:

```sh
flip --reference reference.png --test test.png
```

After the command runs, inspect:

- the console statistics, especially mean and weighted median FLIP
- the generated FLIP error map image

The number tells you how different the images are overall. The error map tells you where the visible differences are.

## Comparing Denoising

If denoising is enabled, the renderer also writes:

```txt
image-OIDN.ppm
```

You can compare the denoised image against the non-denoised render from the same run:

```sh
python -c "from PIL import Image; Image.open('image.ppm').save('noisy.png')"
python -c "from PIL import Image; Image.open('image-OIDN.ppm').save('denoised.png')"
flip --reference noisy.png --test denoised.png
```

This is not a quality comparison against ground truth. It only shows what the denoiser changed. For denoising quality, use a high-sample reference image:

```sh
flip --reference reference.png --test denoised.png
```

## What To Watch

Keep the comparison fair:

- use the same scene
- use the same camera
- use the same resolution
- use the same tone mapping and gamma correction
- use the same denoising setting, unless denoising is exactly what you are testing

Do not directly compare images with different cameras, resolutions, exposure, tone mapping, or post-processing. FLIP will still produce a result, but the result will describe all of those changes mixed together.

When reading the error map, pay attention to:

- sampling noise in indirect lighting and shadows
- fireflies or isolated bright pixels
- structured artifacts that repeat across the image
- errors along silhouettes and triangle edges
- incorrect texture lookup, wrapping, or filtering
- material mistakes, such as roughness or metallic values using the wrong channel
- denoising blur, especially around high-frequency texture detail

Lower FLIP values are better, but do not judge only by one number. A small average error can still hide a serious local artifact if the problem affects only a small part of the image.

## Student Checklist

Before submitting a comparison, make sure you can answer:

1. Which image is the reference?
2. Which image is the test?
3. What changed between the two renders?
4. Are the scene, camera, resolution, and post-processing identical?
5. Where does the FLIP map show the strongest visible error?
6. Is the error random noise, a structured bug, or a deliberate algorithmic difference?
