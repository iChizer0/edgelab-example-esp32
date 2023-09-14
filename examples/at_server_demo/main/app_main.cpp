#include "frontend/main_task.hpp"


extern "C" void app_main(void) {
    frontend::main_task::run();
}
