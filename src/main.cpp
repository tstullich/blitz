#include "renderer.h"

int main() {
    blitz::Renderer app;
    try {
        app.run();
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}