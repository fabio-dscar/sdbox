#include <app.h>

int main() {
    sdbox::SdboxApp app{};
    app.init("testFolder");
    app.loop();
}