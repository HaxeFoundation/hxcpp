#pragma once

namespace hx {
    namespace maths {
        struct point {
            double x = 7;
            double y = 26;

            point() = default;
            point(double _x, double _y);
        };
    }
}