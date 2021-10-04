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
        input.dim(0).set_stride(4);
        input.dim(2).set_stride(1);
        input.dim(1).set_stride(width*4);
        
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
        input.dim(0).set_stride(4);
        input.dim(2).set_stride(1);
        input.dim(1).set_stride(width*4);

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

/*
class SkinSmoothGenerator : public Halide::Generator<SkinSmoothGenerator> {
public:
    Input<Buffer<uint8_t>> input{"input", 3};
    Output<Buffer<uint8_t>> output{"output", 3};

    Var x, y, c, h;

    void generate() {
        Expr skinSum = skinDetection(Func(input), input.width(), input.height());
        Expr skin_rate = skinSum / input.width()*input.height() * 100;
        Expr radius = min(input.width(), input.height()) / skin_rate + 1;
        output = skinDenoise(input, input.width(), input.height(), radius, 10);
    }

private:
    Expr skinDetection(Func input, Expr width, Expr height) {

        Func avg, count;
        Expr output("skinDetection output");
        
        avg(x, y, c) = cast<float>(input(x, y  , c) + input(x+1, y  , c) + 
                                   input(x, y+1, c) + input(x+1, y+1, c)) / 4.f;


        Expr condition = avg(x, y, 0) >= 60 && avg(x, y, 1) >= 40 && avg(x, y, 2) >= 20 &&
                         avg(x, y, 0) >= avg(x, y, 2) && (avg(x, y, 0) - avg(x, y, 1)) >= 10 &&
                         (max(max(avg(x, y, 0), avg(x, y, 1)), avg(x, y, 2)) - 
                          min(min(avg(x, y, 0), avg(x, y, 1)), avg(x, y, 2)) >= 10);

        count(x, y) = select(condition, 1, 0);
        
        RDom r(0, width, 0, height);
        output = sum(count(r.x, r.y)); 

        // Schedule
        avg.compute_root().parallel(y).vectorize(x, 16);
        count.compute_root().parallel(y).vectorize(x, 16);

        return output;
    }
    
    Func skinDenoise(Func input, Expr width, Expr  height, Expr radius, Expr smoothLevel) {

        //Expr windowSize = (2 * radius + 1) * (2 * radius + 1);
        Func input_interior = BoundaryConditions::mirror_interior(input, {{0, width}, {0, height}});
        RDom r(-20, 20);
        r.where(r <= 10);
        Func sum_x, sum_y, sum_xx, sum_yy;
        sum_x(x, y, c) = sum(input_interior(x+r, y, c));
        
        sum_y(x, y, c) = sum(sum_x(x, y+r, c));
        
        sum_xx(x, y, c) = sum(pow(input_interior(x+r, y, c), 2));
        sum_yy(x, y, c) = sum(pow(sum_xx(x, y+r, c), 2));
        
        Func mean, diff, edge, mask_edge, var, out;
        mean(x, y, c) = cast<float>(sum_y(x, y, c)) / cast<float>(windowSize);
        diff(x, y, c) = mean(x, y, c) - input(x, y, c);
        edge(x, y, c) = clamp(diff(x, y, c), 0.f, 255.f);
        mask_edge(x, y, c) = (edge(x, y, c) * cast<float>(input(x, y, c)) + 
                             (256.f - edge(x, y, c)) * mean(x, y, c)) / 256.f;
        var(x, y, c) = cast<float>(sum_yy(x, y, c) - mean(x, y, c)*sum_yy(x, y, c)) /
                       cast<float>(windowSize);
        
        Func smoothLut;
        smoothLut(h) = max(1, cast<int>((exp((cast<float>(h) * -1.f) / (smoothLevel * 255.f)) +
                                        smoothLevel * (cast<float>(h) + 1.f) + 1.f) * 0.5f));

        out(x, y, c) = mask_edge(x, y, c) - diff(x, y, c) * var(x, y, c) /
                        (var(x, y, c) + smoothLut(input(x, y, c)));

        Func output("skinDenoise output");
        output(x, y, c) = cast<uint8_t>(out(x, y, c));

        // Schedule
        output.compute_root().parallel(y).vectorize(x, 16);
        
        return sum_x;
    }
};

HALIDE_REGISTER_GENERATOR(SkinSmoothGenerator, skin_smooth)*/
