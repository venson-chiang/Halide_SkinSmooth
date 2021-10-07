#include <Halide.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <stdint.h>

using namespace Halide;

class skinDetectGenerator : public Halide::Generator<skinDetectGenerator> {

public:
    Input<Buffer<uint8_t>> input{"input", 3};
    Input<int> width{"width"};
    Input<int> height{"height"};
    Output<Buffer<int>> output{"output", 1};
    
    Var x, y, c, n;

    void generate() {
        
        Func floatin("float_input"), avg("average"), count("count");
        
        floatin(x, y, c) = cast<float>(input(x, y, c));
        avg(x, y, c) = (floatin(x, y  , c) + floatin(x+1, y  , c) + 
                        floatin(x, y+1, c) + floatin(x+1, y+1, c)) / 4.f;

        Expr condition = avg(x, y, 0) >= 60.f && avg(x, y, 1) >= 40.f && avg(x, y, 2) >= 20.f &&
                         avg(x, y, 0) >= avg(x, y, 2) && (avg(x, y, 0) - avg(x, y, 1)) >= 10.f &&
                         (max(max(avg(x, y, 0), avg(x, y, 1)), avg(x, y, 2)) - 
                          min(min(avg(x, y, 0), avg(x, y, 1)), avg(x, y, 2)) >= 10.f);

        count(x, y) = select(condition, 1, 0);
        
        RDom r(0, width, 0, height);
        output(n) = sum(count(clamp(r.x, 0, width-2), clamp(r.y, 0, height-2))); 

        // Schedule
        avg.compute_root().parallel(y).vectorize(x, 16);
        count.compute_root().parallel(y).vectorize(x, 16);
        output.compute_root();
    }
};
HALIDE_REGISTER_GENERATOR(skinDetectGenerator, skinDetection)

class skinDenoiseGenerator : public Halide::Generator<skinDenoiseGenerator> {
public:
    Input<Buffer<uint8_t>> input{"input", 3};
    Input<int> width{"width"};
    Input<int> height{"height"};
    Input<int> radius{"radius"};
    Input<int> smoothLevel{"smoothLevel"};

    Output<Buffer<uint8_t>> output{"output", 3};

    Var x, y, c, h;

    void generate() {

        Expr windowSize = (2 * radius + 1) * (2 * radius + 1);
        Func input_interior = BoundaryConditions::mirror_interior(input, {{0, width}, {0, height}});
        
        RDom r(-radius, 2 * radius + 1);
        Func sumx, sumy, powerx, powery;
        
        sumx(x, y, c) = sum(cast<int>(input_interior(x+r, y, c)));
        sumy(x, y, c) = sum(sumx(x, y+r, c));
        
        powerx(x, y, c) = sum(pow(cast<int>(input_interior(x+r, y, c)), 2));
        powery(x, y, c) = sum(powerx(x, y+r, c));
        
        Func mean("mean"), diff("difference"), edge("edge"), mask_edge("mask_edge"), 
             var("var"), out("tmep_out");

        mean(x, y, c) = sumy(x, y, c) / windowSize;
        diff(x, y, c) = mean(x, y, c) - cast<int>(input(x, y, c));
        edge(x, y, c) = clamp(diff(x, y, c), 0.f, 255.f);
        mask_edge(x, y, c) = (edge(x, y, c) * cast<int>(input(x, y, c)) + 
                             (256.f - edge(x, y, c)) * mean(x, y, c)) / 256.f;
        var(x, y, c) = (powery(x, y, c) - mean(x, y, c)*sumy(x, y, c)) / windowSize;
        
        Func smoothLut("smoothLut");
        smoothLut(h) = max(1, cast<int>((exp((cast<int>(h) * -1) / (smoothLevel * 255)) +
                                        smoothLevel * (cast<int>(h) + 1) + 1) * 0.5f));

        out(x, y, c) = mask_edge(x, y, c) - diff(x, y, c) * var(x, y, c) /
                        (var(x, y, c) + smoothLut(input(x, y, c)));

        output(x, y, c) = cast<uint8_t>(clamp(out(x, y, c), 0.f, 255.f));
        
        // Schedule
        sumx.compute_root().parallel(y).vectorize(x, 16);
        sumy.compute_root().parallel(y).vectorize(x, 16);
        mean.compute_root().parallel(y).vectorize(x, 16);
        powerx.compute_root().parallel(y).vectorize(x, 16);
        powery.compute_root().parallel(y).vectorize(x, 16);
        mask_edge.compute_root().parallel(y).vectorize(x, 16);
        var.compute_root().parallel(y).vectorize(x, 16);
        
        smoothLut.compute_root();
        out.compute_root().parallel(y).vectorize(x, 16);
        
        output.compute_root().parallel(y).vectorize(x, 16);
        
    }
};
HALIDE_REGISTER_GENERATOR(skinDenoiseGenerator, skinDenoise)
