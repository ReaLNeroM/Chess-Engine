#include <iostream>
#include <vector>
#include <cstdlib> 
#include <cstring>

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include "magic.h"
#include "helper.h"
#include "piece.h"
#include "board.h"
#include "gameHandler.h"
#include "chessAI.h"

bool validateInteger(char* array){
	while(*array != '\0'){
		std::cerr << *array << '\n';
		if('0' > *array or *array > '9'){
			return false;
		}
		array++;
	}

	return true;
}

void printHelp(){
	std::cout << "ChessGame - a (nearly) fully functioned chess clone and chess engine. \n";
	std::cout << "By default, the AI plays by itself.\n";
	std::cout << " -b, --black          AI plays white, player plays black\n";
	std::cout << " -w, --white          AI plays black, player plays white\n";
	std::cout << " -m, --moves-ahead    How many moves ahead the AI simulates\n";
	std::cout << " -h, --help           Print this help message.\n";
}

int main(int argc, char* argv[]){
	sf::RenderWindow window(sf::VideoMode(Magic::size, Magic::size), "It Works!");
	window.setFramerateLimit(60);

	Magic::playerColor = Magic::color::none;

	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-h") == 0 or strcmp(argv[i], "--help") == 0){
			printHelp();
			exit(0);
		} else if(strcmp(argv[i], "-w") == 0 or strcmp(argv[i], "--white") == 0){
			Magic::playerColor = Magic::color::white;
		} else if(strcmp(argv[i], "-b") == 0 or strcmp(argv[i], "--black") == 0){
			Magic::playerColor = Magic::color::black;
		} else if(strcmp(argv[i], "-m") == 0 or strcmp(argv[i], "--moves-ahead") == 0){
			if(i + 1 != argc and validateInteger(argv[i + 1])){
				Magic::propagationLimit = strtol(argv[i + 1], NULL, 10);
			}
			i++;
		} else {
			std::cerr << "ERR: unrecognized command\n";
			exit(-1);
		}
	}

	BoardStructure::init();
	sf::SoundBuffer buffer;
	buffer.loadFromFile("media/Move.ogg");
	sf::Sound sound;
	sound.setBuffer(buffer);

	sf::Texture boardImage;
	boardImage.loadFromFile(Magic::boardString);

	sf::Sprite board;
	board.setTexture(boardImage, true);
	board.setPosition(0.f, 0.f);
	board.setScale((double) Magic::size / boardImage.getSize().x, 
					(double) Magic::size / boardImage.getSize().y);

	int clickState = 0;
	Piece::Base *held = NULL;

	std::pair<sf::Vector2i, sf::Vector2i> lastMove = {{-1, -1}, {-1, -1}};

	auto x = AI::getBestMove();
	bool AIMoved = false;

	while (window.isOpen()){
		sf::Event event;
		auto mouseLocation = sf::Mouse::getPosition(window);

		while (window.pollEvent(event)){
			if (event.type == sf::Event::Closed){
				window.close();
			} else if(event.type == sf::Event::MouseButtonPressed){
				if(event.mouseButton.button == sf::Mouse::Left){
					held = BoardStructure::find(mouseLocation);

					if(held != NULL){
						clickState = 1;
					}
				} else if(event.mouseButton.button == sf::Mouse::Right){
					BoardStructure::undoMove();
					lastMove = std::pair<sf::Vector2i, sf::Vector2i>{{-1, -1}, {-1, -1}};
					x = std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -2}};
					AIMoved = true;
				}
			} else if(event.type == sf::Event::MouseButtonReleased){
				if(clickState != 1 or held == NULL){
					continue;
				}

				sf::Vector2i prevBoardPos = held->boardPos;
				if(GameHandler::attemptMove(*held, Helper::getIndices(mouseLocation), true)){
					lastMove = {prevBoardPos, Helper::getIndices(mouseLocation)};
					x = std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -2}};
					AIMoved = false;
					GameHandler::checkWin();
					sound.play();
				}			

				clickState = 2;
				held = NULL;
			}
		}

		window.draw(board);

		if(x != std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -1}} and 
			x != std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -2}}){
			if(!AIMoved and GameHandler::attemptMove(BoardStructure::board[x.first.y][x.first.x], x.second, false)){
				lastMove = x;
				if(Magic::playerColor != Magic::color::none){
					x = std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -2}};
				}
				sound.play();
				GameHandler::checkWin();
			}
			AIMoved = false;
		}

		if(lastMove != std::pair<sf::Vector2i, sf::Vector2i>{{-1, -1}, {-1, -1}}){
			sf::Vector2f startPos = sf::Vector2f(lastMove.first.x * Magic::cellSize, lastMove.first.y * Magic::cellSize);
			sf::Vector2f nextPos = sf::Vector2f(lastMove.second.x * Magic::cellSize, lastMove.second.y * Magic::cellSize);
			sf::CircleShape startPosCircle(Magic::cellSize / 2.0);
			sf::CircleShape nextPosCircle(Magic::cellSize / 2.0);
			startPosCircle.setFillColor(sf::Color::Green);
			nextPosCircle.setFillColor(sf::Color::Red);
			startPosCircle.setPosition(startPos);
			nextPosCircle.setPosition(nextPos);
			window.draw(startPosCircle);
			window.draw(nextPosCircle);
		}

		if(clickState == 1){
			if(held == NULL){
				std::cerr << "No unit selected" << '\n';
				return -1;
			}

			held->cleared = true;

			sf::Sprite x;
			x.setPosition(sf::Vector2f(mouseLocation.x - Magic::cellSize / 2.0, 
												 mouseLocation.y - Magic::cellSize / 2.0));
			x.setScale(sf::Vector2f((double) Magic::cellSize / Magic::pieceTextures[held->pieceColor][held->pieceType].getSize().x, 
											   (double) Magic::cellSize / Magic::pieceTextures[held->pieceColor][held->pieceType].getSize().y));
   			x.setTexture(Magic::pieceTextures[held->pieceColor][held->pieceType]);
			window.draw(x);
		}

		BoardStructure::drawPieces(window);

		window.display();

		if(x == std::pair<sf::Vector2i,sf::Vector2i>{{-1, -1}, {-1, -2}}){
			x = AI::getBestMove();
		} //postpone calculating AI move so that the board is rendered properly
		
		if(clickState == 1 and held != NULL){
			held->cleared = false;
		}
	}

	return 0;
}
