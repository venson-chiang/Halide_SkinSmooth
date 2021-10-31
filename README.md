# Halide_SkinSmooth
Skinsmooth using Halide

# Requirements
Halide 12.0.0 or above: https://github.com/halide/Halide

# Methods
1.Detect how much skin color in the image, to calculate filter radius.

2.Apply SkinSmooth filter to denoise skin in the image.

# Input Images
Input images are reference from https://www.cnblogs.com/cpuimage/p/13172510.html

<img src="https://github.com/venson-chiang/Halide_SkinSmooth/blob/main/test_images/test1.jpg" width="50%" height="50%">

# Result
<img src="https://github.com/venson-chiang/Halide_SkinSmooth/blob/main/output_images/output.png" width="50%" height="50%">

# Usage
1. Change HALIDE_DISTRIB_PATH to yours in Makefile.inc
```
HALIDE_DISTRIB_PATH ?= /mnt/d/Software/Halide-12/distrib 
```
2. Run Makefile 
```
make test
```

# Reference
SkinSmooth method is reference to https://github.com/cpuimage/skin_smoothing

