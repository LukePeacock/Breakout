//
//  game.hpp
//  Breakout
//
//  Created by Luke on 20/11/2019.
//  Copyright © 2019 Luke Peacock. All rights reserved.
//

#ifndef game_hpp
#define game_hpp
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "game_level.hpp"
#include "power_up.hpp"


// Represents the current state of the game
enum GameState {
    GAME_ACTIVE,
    GAME_MENU,
    GAME_WIN
};

// Represents the four possible (collision) directions
enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Initial size of the player paddle
const glm::vec2 PLAYER_SIZE(100, 20);
// Initial velocity of the player paddle
const GLfloat PLAYER_VELOCITY(500.0f);


// Initial velocity of the Ball
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
// Radius of the ball object
const GLfloat BALL_RADIUS = 12.5f;


// Defines a Collision typedef that represents collision data
typedef std::tuple<GLboolean, Direction, glm::vec2> Collision; // <collision?, what direction?, difference vector center - closest point>


// Game holds all game-related state and functionality.
// Combines all game-related data into a single class for
// easy access to each of the components and manageability.
class Game
{
public:
    // Game state
    GameState              State;
    GLboolean              Keys[1024];
    GLboolean              KeysProcessed[1024];
    GLuint                 Width, Height;
    std::vector<GameLevel> Levels;
    GLuint                 Level;
    GLuint                 Score;
    GLuint                 Lives;
    std::vector<PowerUp>   PowerUps;
    
    
    // Constructor/Destructor
    Game(GLuint width, GLuint height);
    ~Game();
    // Initialize game state (load all shaders/textures/levels)
    void Init(int width, int height);
    // GameLoop
    void ProcessInput(GLfloat dt);
    void Update(GLfloat dt);
    void Render(GLuint textScale);
    void DoCollisions();
    // Reset
    void ResetLevel();
    void ResetPlayer();
    
    void SpawnPowerUps(GameObject &block);
    void UpdatePowerUps(GLfloat dt);
};

#endif /* game_hpp */
