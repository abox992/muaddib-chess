#ifndef PIECE_SPRITE_H
#define PIECE_SPRITE_H

#include <SFML/Graphics.hpp>

const std::string file[] = {"a", "b", "c", "d", "e", "f", "g", "h"};

struct PieceSprite {
    sf::Sprite sprite;

    bool isDragging;
    sf::Vector2f mouseOffset;

    // used to determine if drag location (ie move) is valid or not
    std::string currentLocation;
    std::string prevLocation;

    PieceSprite(sf::Sprite sprite);

    void disableDragging();
};

#endif