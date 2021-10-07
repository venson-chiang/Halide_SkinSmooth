# Halide_SkinSmooth
Skinsmooth processing using Halide

# Requirements
Halide 12.0.0 or above: https://github.com/halide/Halide

# Result
<img src="https://github.com/venson-chiang/Halide_SkinSmooth/blob/main/test_images/output1.png" width="50%" height="50%">

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
The method used in this project is reference to https://github.com/cpuimage/skin_smoothing

