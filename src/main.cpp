#include "app/path_tracer_app.h"
#include <iostream>

int main() {
    try {
        PathTracerApp app;
        app.run();
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << '\n';
        return 1;
    }
}