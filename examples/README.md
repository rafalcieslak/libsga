In most examples, F11 toggles fullscreen mode and Esc quits the program. Most examples use GLM for vector operations.

Available example programs:

### Triangle

Very simple program that only renders a flat triangle. Shows the most basic API features.

### Cube

Renders a simple 3D textured cube. Demonstrates texturing support and mouse input (mouse movement rotates the cube). 1/2/3 switches wiregrame/point modes.

### FragTest1

Rendrers a 2D grid, only using a fragment shader. Not very interesting, mostly used for compatibility tests.

### TextureTarget

Renders a 2D grid to file `output.png`, demonstrating window-less use of the API.

### TwoOffscreenPipelines

Applies two filters (expressed as fragment shaders) in form of two subsequent pipelines to an image loaded from file and stores the result to another file. This demonstrates using SGA for image processing and offline operations.

### UnwrapEnvmap

Displays an equirectangular representation of a spherical envmap. Dragging with mouse scrolls the image around. This demonstrates using GPU samplers for fast image warping.

### Sampler

Demonstrates anisotropic filtering.

### Teapot

A very basic forward renderer displaying the traditional Utah Teapot model. Mouse movement rotates camera angle. 1/2/3 switches wiregrame/point modes. Uses assimp to load model from file.

### SSAA

Teapot model is rendered in a higher resolution, resulting image is downsampled to target resolution, which demonstrates basic antialiasing via super-sampling.

### Shadertoy

This program provides comparibility with shadertoy.com shaders. Usage:
```
./shadertoy SHADER_FILE [IMAGE1 [IMAGE2 [...]]]
```
Should be 100% compatible with shaders that don't use any external features except for mouse input and image sampling.

### Deferred

Simple deferred shading pipeline. Hold SHIFT to preview G-Buffer.

### Text

Demonstrates text rendering with FreeType2.

### Sponza

Displays the Darbovic Sponza model with no lighting (all textures 100% emissive). Demonstrates user movement with WASD and multiple-material setup.
