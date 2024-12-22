#include "zobrist.h"
#include "cli.h"

// #include "gui/game.h"

int main() {

    Zobrist::initZobrist();
    cli::loop();

    return 0;
}
