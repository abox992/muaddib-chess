#include "piece_sprite.h"
#include "game.h"
#include <cassert>

PieceSprite::PieceSprite(sf::Sprite sprite) {
    this->sprite = sprite;
    this->isDragging = false;
}

void PieceSprite::disableDragging() {
    assert(this->isDragging);

    // snap to position
    sf::Vector2f pos = sprite.getPosition();
    int x = pos.x + BOARD_SQUARE_SIZE / 2;
    x -= x % BOARD_SQUARE_SIZE;
    int y = pos.y + BOARD_SQUARE_SIZE / 2;
    y -= y % BOARD_SQUARE_SIZE;
    sprite.setPosition({static_cast<float>(x), static_cast<float>(y)});

    this->isDragging = false;
}