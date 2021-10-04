#include "HalideBuffer.h"
#include "halide_image_io.h"
#include "bin/host/skinDenoise.h"
#include "bin/host/skinDetection.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <math.h>

using namespace Halide;
using namespace Halide::Tools;

int main(int argc, char** argv) {

    Halide::Runtime::Buffer<uint8_t> input;
    Halide::Runtime::Buffer<int> skinSum(1);
    
    input = load_image("test1.jpg");
    int width = input.width();
    int height = input.height();

    skinDetection(input, width, height, skinSum);
    
    std::cout << "width=" << width <<", height=" << height << std::endl;
    std::cout << "skinSum=" << skinSum(0) << std::endl;

    float skin_rate = skinSum(0) / (float) (width * height) * 100;
    int radius = width / skin_rate + 1;
    std::cout << "raius=" << radius << std::endl;

    Halide::Runtime::Buffer<uint8_t> output(width, height, 3);
    skinDenoise(input, width, height, 35, 10, output);

    save_image(output, "outpu1.png");

    printf("Success!!\n");
    return 0;

}