#include "zobrist.h"
#include "cli.h"

// #include "gui/game.h"

int main(int argc, char* argv[]) {
    Zobrist::initZobrist();
    cli::loop();

    return 0;
}
