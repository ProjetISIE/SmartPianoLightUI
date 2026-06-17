#include "AppController.hpp"

int main(int argc, char* argv[]) {
    AppController app;
    app.init(argc, argv);
    app.run();
    return 0;
}
