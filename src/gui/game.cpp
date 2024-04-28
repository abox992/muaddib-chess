#include "game.h"
#include "piece_sprite.h"
#include "../types.h"
#include <iostream>
#include <sstream>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>

Game::Game() {
    initWindow();
    initTextures();
    initPieceSprites(Color::WHITE);
    initPieceSprites(Color::BLACK);

    // init the pieces vector (note these are pointers to the original ones, we dont have to worry about updating this every time)
    for (auto& p : this->whitePieces) {
        this->pieces.push_back(&p);
    }
    for (auto& p : this->blackPieces) {
        this->pieces.push_back(&p);
    }
}

Game::~Game() {
    delete this->window;
}

void Game::initWindow() {
    // create the window
    this->videoMode.height = WINDOW_HEIGHT;
    this->videoMode.width = WINDOW_WIDTH;
    this->window = new sf::RenderWindow(this->videoMode, "Chess", sf::Style::Titlebar | sf::Style::Close);

    this->window->setPosition(sf::Vector2i(2690, 2030));

    //std::cout << this->window->getPosition().x << ", " << this->window->getPosition().y << std::endl;
}

void Game::initTextures() {
    const std::vector<std::string> pieces = {"pawn", "knight", "bishop", "rook", "queen", "king"};

    for (auto piece : pieces) {
        sf::Texture texture;
        if (!texture.loadFromFile("../pieces-images/white-" + piece + ".png")) {
            // error...
            std::cout << "failed to load image" << std::endl;
        }
        texture.setSmooth(true);

        whiteTextures.push_back(texture);

        if (!texture.loadFromFile("../pieces-images/black-" + piece + ".png")) {
            // error...
            std::cout << "failed to load image" << std::endl;
        }
        texture.setSmooth(true);

        blackTextures.push_back(texture);
    }

}

void Game::initPieceSprites(Color color) {

    std::vector<sf::Texture>* textures;
    std::vector<PieceSprite>* pieces;

    int frontRank;
    int backRank;

    if (color == Color::WHITE) {
        textures = &whiteTextures;
        pieces = &whitePieces;
        frontRank = 6;
        backRank = 7;
    } else {
        textures = &blackTextures;
        pieces = &blackPieces;
        frontRank = 1;
        backRank = 0;
    }

    // initial dummy sprite with correct scale
    sf::Sprite sprite;
    sprite.setScale(sf::Vector2f((float) BOARD_SQUARE_SIZE / PIECE_IMG_SIZE, (float) BOARD_SQUARE_SIZE / PIECE_IMG_SIZE));

    // pawns
    sprite.setTexture((*textures)[0]);
    for (int col = 0; col < 8; col++) {
        sprite.setPosition({static_cast<float>(0 + col * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * frontRank)});
        pieces->push_back(PieceSprite(sprite));
    }

    // knights
    sprite.setTexture((*textures)[1]);
    sprite.setPosition({static_cast<float>(BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});

    pieces->push_back(PieceSprite(sprite));

    sprite.setPosition({static_cast<float>(6 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});
    pieces->push_back(PieceSprite(sprite));

    // bishops
    sprite.setTexture((*textures)[2]);
    sprite.setPosition({static_cast<float>(2 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});

    pieces->push_back(PieceSprite(sprite));

    sprite.setPosition({static_cast<float>(5 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});
    pieces->push_back(PieceSprite(sprite));

    // rooks
    sprite.setTexture((*textures)[3]);
    sprite.setPosition({static_cast<float>(0), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});

    pieces->push_back(PieceSprite(sprite));

    sprite.setPosition({static_cast<float>(7 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});
    pieces->push_back(PieceSprite(sprite));

    // queen
    sprite.setTexture((*textures)[4]);
    sprite.setPosition({static_cast<float>(3 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});

    pieces->push_back(PieceSprite(sprite));

    // king
    sprite.setTexture((*textures)[5]);
    sprite.setPosition({static_cast<float>(4 * BOARD_SQUARE_SIZE), static_cast<float>(BOARD_SQUARE_SIZE * backRank)});

    pieces->push_back(PieceSprite(sprite));
}

void Game::update() {
    
}

void Game::render() {

}

bool Game::isMouseOver(const sf::Sprite& sprite) {
    int mouseX = sf::Mouse::getPosition(*this->window).x;
    int mouseY = sf::Mouse::getPosition(*this->window).y;

    int spriteX = sprite.getPosition().x;
    int spriteY = sprite.getPosition().y;

    int spriteWidth = sprite.getTexture()->getSize().x * sprite.getScale().x;
    int spriteHeight = sprite.getTexture()->getSize().y * sprite.getScale().y;

    return (mouseX >= spriteX && mouseX <= (spriteX + spriteWidth)) && (mouseY >= spriteY && mouseY <= (spriteY + spriteHeight));
}

void Game::updateDraggingPos() {

    for (auto& p : this->pieces) {
        if (p->isDragging) {
            p->sprite.setPosition(sf::Mouse::getPosition(*this->window).x - p->mouseOffset.x, sf::Mouse::getPosition(*this->window).y - p->mouseOffset.y); 
        }
    }
}

void Game::drawBoard() {
    sf::Color light(238,238,210);
    sf::Color dark(118,150,86);

    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            sf::RectangleShape rectangle(sf::Vector2f(BOARD_SQUARE_SIZE, BOARD_SQUARE_SIZE));
            rectangle.setFillColor((row + col) % 2 == 0 ? light : dark);
            rectangle.setPosition(col * BOARD_SQUARE_SIZE, row * BOARD_SQUARE_SIZE);
            window->draw(rectangle);
        }
    }
}

void Game::drawPieces() {
    int draggingIndex = -1;
    for (size_t i = 0; i < this->pieces.size(); i++) {

        // if a piece is being dragged, wait to draw it last so it appears above the rest
        if (this->pieces[i]->isDragging) {
            draggingIndex = i;
            continue;
        }

        window->draw(this->pieces[i]->sprite);
    }

    // draw the dragged piece
    if (draggingIndex != -1) {
        window->draw(this->pieces[draggingIndex]->sprite);
    }

}

void Game::runGameLoop() {

    while (window->isOpen()) {

        // check all the window's events that were triggered since the last iteration of the loop
        while (window->pollEvent(event)) {
            switch (event.type) {
                // "close requested" event: we close the window
                case sf::Event::Closed:
                    window->close();
                    break;
                case sf::Event::MouseButtonPressed:
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        // handle piece started to drag
                        for (auto& p : this->pieces) {
                            if (isMouseOver(p->sprite)) {
                                p->isDragging = true;
                                sf::Vector2f spritePos = p->sprite.getPosition();
                                p->mouseOffset = {sf::Mouse::getPosition(*this->window).x - spritePos.x, sf::Mouse::getPosition(*this->window).y - spritePos.y};
                                
                                p->prevLocation = p->currentLocation;

                                // change opacity
                                p->sprite.setColor(sf::Color(255, 255, 255, 128));
                            }
                        }
                    }
                    break;
                case sf::Event::MouseButtonReleased:
                    // handle drag released
                    if (event.mouseButton.button == sf::Mouse::Left) {
                        for (auto& p : this->pieces) {
                            if (p->isDragging) {
                                p->disableDragging();
                                int fileIndex = p->sprite.getPosition().x / BOARD_SQUARE_SIZE;

                                std::stringstream ss;
                                ss << file[fileIndex] << 8 - p->sprite.getPosition().y / BOARD_SQUARE_SIZE;

                                p->currentLocation = ss.str();

                                // change opacity
                                p->sprite.setColor(sf::Color(255, 255, 255, 255));

                                std::cout << p->prevLocation << p->currentLocation << std::endl;

                                if (!buffer.loadFromFile("../sounds/move-self.wav")) {
                                    //error
                                }
                                sound.setBuffer(buffer);
                                sound.play();

                            }
                        }
                    }
                    break;
                default:
                    break;

            }
        }

        // clear the window
        window->clear();

        // draw everything here...
        // window.draw(...);

        drawBoard();
        updateDraggingPos();
        drawPieces();

        //std::cout << this->window->getPosition().x << ", " << this->window->getPosition().y << std::endl;

        // end the current frame
        window->display();
    }
}
