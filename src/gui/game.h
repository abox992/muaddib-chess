#ifndef GAME_H
#define GAME_H

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

#include "piece_sprite.h"
#include "../constants.h"

#define BOARD_SQUARE_SIZE 100
#define PIECE_IMG_SIZE 128
#define WINDOW_HEIGHT 800
#define WINDOW_WIDTH 800

class Game {
private:
    sf::RenderWindow* window;
    sf::VideoMode videoMode;
    sf::Event event;
    
    sf::Sound sound;
    sf::SoundBuffer buffer;

    // pawn, knight, bishop, rook, queen, king
    std::vector<sf::Texture> whiteTextures;
    std::vector<sf::Texture> blackTextures;

    std::vector<PieceSprite> whitePieces;
    std::vector<PieceSprite> blackPieces;
    std::vector<PieceSprite*> pieces;

    void initWindow();
    void initTextures();
    void initPieceSprites(Color color);

    void drawBoard();
    void drawPieces();

    bool isMouseOver(const sf::Sprite& sprite);
    void updateDraggingPos();
public:

    Game();
    virtual ~Game();

    void update();
    void render();

    void runGameLoop();
};

#endif