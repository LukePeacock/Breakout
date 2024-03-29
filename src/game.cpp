//
//  game.cpp
//  Breakout
//
//  Created by Luke on 20/11/2019.
//  Copyright © 2019 Luke Peacock. All rights reserved.
//

#include "game.hpp"
#include "resource_manager.hpp"
#include "sprite_renderer.hpp"
#include "game_level.hpp"
#include "ball_object.hpp"
#include "particle_generator.hpp"
#include "post_processor.hpp"
#include "power_up.hpp"
#include "text_renderer.hpp"
#include <sstream>
#include <irrklang/irrKlang.h>

#include <iostream>
using namespace irrklang;

// Handles game functions: render, update, process input, power ups, collisions, etc.

// Game-state data
GameObject          *Player;
SpriteRenderer      *Renderer;
BallObject          *Ball;
ParticleGenerator   *Particles;
PostProcessor       *Effects;
ISoundEngine        *SoundEngine = createIrrKlangDevice();
TextRenderer        *Text;

GLfloat ShakeTime = 0.0f;

// constructor
Game::Game(GLuint width, GLuint height)
    : State(GAME_MENU), Keys(), Width(width), Height(height), Level(0), Lives(3), Score(0)
{

}

// Destructor
Game::~Game()
{
    delete Renderer;
    delete Player;
    delete Ball;
    delete Text;
    delete Particles;
    delete Effects;
    delete SoundEngine;
}

// Initialise shaders, textures, and other game assets
void Game::Init(int width, int height)
{
    this->Width = width;
    this->Height = height;
    // Load shaders
    ResourceManager::LoadShader("assets/shaders/sprite_shader.vs", "assets/shaders/sprite_shader.frag", nullptr, "sprite");
    ResourceManager::LoadShader("assets/shaders/particle_shader.vs", "assets/shaders/particle_shader.frag", nullptr, "particle");
    ResourceManager::LoadShader("assets/shaders/post_processing.vs", "assets/shaders/post_processing.frag", nullptr, "postprocessing");
    ResourceManager::LoadShader("assets/shaders/post_processing.vs", "assets/shaders/post_processing.frag", nullptr, "postprocessing");
    
    // Configure shaders
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(this->Width), static_cast<GLfloat>(this->Height), 0.0f, -1.0f, 1.0f);
    ResourceManager::GetShader("sprite").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("sprite").SetMatrix4("projection", projection);
    ResourceManager::GetShader("particle").Use().SetInteger("sprite", 0);
    ResourceManager::GetShader("particle").SetMatrix4("projection", projection);
    
    // Load textures
    ResourceManager::LoadTexture("assets/textures/background.jpg", GL_FALSE, "background");
    ResourceManager::LoadTexture("assets/textures/ball.png", GL_TRUE, "ball");
    ResourceManager::LoadTexture("assets/textures/block.png", GL_FALSE, "block");
    ResourceManager::LoadTexture("assets/textures/block_solid.png", GL_FALSE, "block_solid");
    ResourceManager::LoadTexture("assets/textures/paddle.png", true, "paddle");
    ResourceManager::LoadTexture("assets/textures/particle.png", GL_TRUE, "particle");
    ResourceManager::LoadTexture("assets/textures/powerup_speed.png", GL_TRUE, "powerup_speed");
    ResourceManager::LoadTexture("assets/textures/powerup_sticky.png", GL_TRUE, "powerup_sticky");
    ResourceManager::LoadTexture("assets/textures/powerup_increase.png", GL_TRUE, "powerup_increase");
    ResourceManager::LoadTexture("assets/textures/powerup_confuse.png", GL_TRUE, "powerup_confuse");
    ResourceManager::LoadTexture("assets/textures/powerup_chaos.png", GL_TRUE, "powerup_chaos");
    ResourceManager::LoadTexture("assets/textures/powerup_passthrough.png", GL_TRUE, "powerup_passthrough");
    
    // Set render-specific controls
    Renderer = new SpriteRenderer(ResourceManager::GetShader("sprite"));
    Particles = new ParticleGenerator(ResourceManager::GetShader("particle"), ResourceManager::GetTexture("particle"), 500);
    Effects = new PostProcessor(ResourceManager::GetShader("postprocessing"), this->Width, this->Height);
    // Load levels
    GameLevel one; one.Load("assets/levels/one.lvl", this->Width, this->Height * 0.5);
    GameLevel two; two.Load("assets/levels/two.lvl", this->Width, this->Height * 0.5);
    GameLevel three; three.Load("assets/levels/three.lvl", this->Width, this->Height * 0.5);
    GameLevel four; four.Load("assets/levels/four.lvl", this->Width, this->Height * 0.5);
    GameLevel five; five.Load("assets/levels/five.lvl", this->Width, this->Height * 0.5);
    this->Levels.push_back(one);
    this->Levels.push_back(two);
    this->Levels.push_back(three);
    this->Levels.push_back(four);
    this->Levels.push_back(five);
    this->Level = 0;       // zero-indexed list: level 1 = level[0], etc..
    
    // Create player paddle
    glm::vec2 playerPos = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Player = new GameObject(playerPos, PLAYER_SIZE, ResourceManager::GetTexture("paddle"));

    // Create ball
    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
    Ball = new BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY,
        ResourceManager::GetTexture("ball"));
    
    // Text renderer
    Text = new TextRenderer(this->Width, this->Height);
    Text->Load("assets/fonts/ocraext.TTF", 24);
}

// Update game state including ball position, collisions, particles, power ups, shake timer,
// loss state, and win state
void Game::Update(GLfloat dt)
{
    // Update objects
    Ball->Move(dt, this->Width, SoundEngine);
    // Check for collisions
    this->DoCollisions();
    // Update particles
    Particles->Update(dt, *Ball, 2, glm::vec2(Ball->Radius / 2));
    // Update PowerUps
    this->UpdatePowerUps(dt);
    if (ShakeTime > 0.0f)
    {
        ShakeTime -= dt;
        if (ShakeTime <= 0.0f)
            Effects->Shake = GL_FALSE;
    }
    // Check loss condition
    if (Ball->Position.y >= this->Height) // Did ball reach bottom edge?
    {
         --this->Lives;
        // Did the player lose all his lives? : Game over
        if (this->Lives == 0)
        {
            this->ResetLevel();
            this->State = GAME_MENU;
        }
        this->ResetPlayer();
    }
    // Check win condition
    if (this->State == GAME_ACTIVE && this->Levels[this->Level].IsCompleted())
    {
        GLuint score = this->Score;
        this->ResetLevel();
        this->ResetPlayer();
        Effects->Chaos = GL_TRUE;
        this->State = GAME_WIN;
        this->Score = score;
    }
}


// Process input, start game, change level, move paddle, etc.
void Game::ProcessInput(GLfloat dt)
{
    // If player is in menu, can change and choose level
    if (this->State == GAME_MENU)
    {
        // Choose current level
        if (this->Keys[GLFW_KEY_ENTER] && !this->KeysProcessed[GLFW_KEY_ENTER])
        {
            this->State = GAME_ACTIVE;
            this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
        }
        // See next level
        if (this->Keys[GLFW_KEY_W] && !this->KeysProcessed[GLFW_KEY_W])
        {
            
            this->Level = (this->Level + 1) % this->Levels.size();
            this->KeysProcessed[GLFW_KEY_W] = GL_TRUE;
        }
        // See previous level
        if (this->Keys[GLFW_KEY_S] && !this->KeysProcessed[GLFW_KEY_S])
        {
            if (this->Level > 0)
                --this->Level;
            else
                this->Level = (GLuint) this->Levels.size() - 1;
            this->KeysProcessed[GLFW_KEY_S] = GL_TRUE;
        }
    }
    // If game is won, can return to menu
    if (this->State == GAME_WIN)
    {
        // Return to menu
        if (this->Keys[GLFW_KEY_ENTER])
        {
            this->Score = 0;
            this->KeysProcessed[GLFW_KEY_ENTER] = GL_TRUE;
            Effects->Chaos = GL_FALSE;
            this->State = GAME_MENU;
        }
    }
    // If game is active, move paddle
    if (this->State == GAME_ACTIVE)
    {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // Move playerboard left
        if (this->Keys[GLFW_KEY_A])
        {
            if (Player->Position.x >= 0)
                Player->Position.x -= velocity;
                if (Ball->Stuck)
                    Ball->Position.x -= velocity;
        }
        // Move player right
        if (this->Keys[GLFW_KEY_D])
        {
            if (Player->Position.x <= this->Width - Player->Size.x)
                Player->Position.x += velocity;
                if (Ball->Stuck)
                    Ball->Position.x += velocity;
        }
        // Launch ball
        if (this->Keys[GLFW_KEY_SPACE])
            Ball->Stuck = false;
    }
}

// Render game
void Game::Render(GLuint textScale)
{
    if(this->State == GAME_ACTIVE || this->State == GAME_MENU || this->State == GAME_WIN)
    {
        // Start rendering to framebuffer
        Effects->BeginRender();
            // Draw background
            Renderer->DrawSprite(ResourceManager::GetTexture("background"),glm::vec2(0, 0), glm::vec2(this->Width, this->Height), 0.0f);
            // Draw level, player, and power ups
            this->Levels[this->Level].Draw(*Renderer);
            Player->Draw(*Renderer);
            for (PowerUp &powerUp : this->PowerUps)
                if (!powerUp.Destroyed)
                    powerUp.Draw(*Renderer);
            // Draw particles
            Particles->Draw();
            // Draw ball
            Ball->Draw(*Renderer);
        // Stop rendering to framebuffer and render framebuffer to screen
        Effects->EndRender();
        Effects->Render(glfwGetTime());
        // Render text (don't include in postprocessing)
        std::stringstream lives; lives << this->Lives;
        Text->RenderText("Lives:" + lives.str(), 5.0f, 5.0f, 1.0f * textScale);
        std::stringstream score; score << this->Score;
        Text->RenderText("Score:" + score.str(), this->Width - (200.0f * textScale),5.0f, 1.0f * textScale);
    }
    // Display menu text
    if (this->State == GAME_MENU)
    {
        Text->RenderText("Press ENTER to start", 250.0f * textScale, this->Height/ 2, 1.0f * textScale);
        Text->RenderText("Press W or S to select level", 245.0f * textScale, this->Height / 2 + (20.0f * textScale) , 0.75f * textScale);
    }
    // Display win state text
    if (this->State == GAME_WIN)
    {
        Text->RenderText("You WON!!!", 320.0f * textScale, this->Height / 2 - (20.0f * textScale), 1.0f * textScale, glm::vec3(0.0f, 1.0f, 0.0f));
        std::stringstream score; score << this->Score;
        Text->RenderText("Score:" + score.str(), 320.0f * textScale, this->Height / 2, 1.0f * textScale, glm::vec3(0.0f, 1.0f, 0.0f));
        Text->RenderText("Press ENTER to retry or ESC to quit", 130.0f * textScale, this->Height / 2 + (20.0f * textScale), 1.0f * textScale, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}

// Reset the level, lives, and score
void Game::ResetLevel()
{
    if (this->Level == 0)
        this->Levels[0].Load("assets/levels/one.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 1)
        this->Levels[1].Load("assets/levels/two.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 2)
        this->Levels[2].Load("assets/levels/three.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 3)
        this->Levels[3].Load("assets/levels/four.lvl", this->Width, this->Height * 0.5f);
    else if (this->Level == 4)
    this->Levels[4].Load("assets/levels/five.lvl", this->Width, this->Height * 0.5f);
    this->Lives = 3;
    this->Score = 0;
}

// Reset the player position and effects
void Game::ResetPlayer()
{
    // Reset player/ball stats
    Player->Size = PLAYER_SIZE;
    Player->Position = glm::vec2(this->Width / 2 - PLAYER_SIZE.x / 2, this->Height - PLAYER_SIZE.y);
    Ball->Reset(Player->Position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2)), INITIAL_BALL_VELOCITY);
    // Also disable all active powerups
    Effects->Chaos = Effects->Confuse = GL_FALSE;
    Ball->PassThrough = Ball->Sticky = GL_FALSE;
    Player->Color = glm::vec3(1.0f);
    Ball->Color = glm::vec3(1.0f);
}


// PowerUps
GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type);

// Update all power-ups
void Game::UpdatePowerUps(GLfloat dt)
{
    // For powerups in game update position and duration of active powerups
    for (PowerUp &powerUp : this->PowerUps)
    {
        powerUp.Position += powerUp.Velocity * dt;
        if (powerUp.Activated)
        {
            powerUp.Duration -= dt;

            if (powerUp.Duration <= 0.0f)
            {
                // Remove powerup from list (will later be removed)
                powerUp.Activated = GL_FALSE;
                // Deactivate effects
                if (powerUp.Type == "sticky")
                {
                    // If there is no other effect of this type active, then deactivate it.
                    if (!IsOtherPowerUpActive(this->PowerUps, "sticky"))
                    {    // Only reset if no other PowerUp of type sticky is active
                        Ball->Sticky = GL_FALSE;
                        Player->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "pass-through")
                {
                    // If there is no other effect of this type active, then deactivate it.
                    if (!IsOtherPowerUpActive(this->PowerUps, "pass-through"))
                    {    // Only reset if no other PowerUp of type pass-through is active
                        Ball->PassThrough = GL_FALSE;
                        Ball->Color = glm::vec3(1.0f);
                    }
                }
                else if (powerUp.Type == "confuse")
                {
                    // If there is no other effect of this type active, then deactivate it.
                    if (!IsOtherPowerUpActive(this->PowerUps, "confuse"))
                    {    // Only reset if no other PowerUp of type confuse is active
                        Effects->Confuse = GL_FALSE;
                    }
                }
                else if (powerUp.Type == "chaos")
                {
                    // If there is no other effect of this type active, then deactivate it.
                    if (!IsOtherPowerUpActive(this->PowerUps, "chaos"))
                    {    // Only reset if no other PowerUp of type chaos is active
                        Effects->Chaos = GL_FALSE;
                    }
                }
            }
        }
    }
    // Remove all PowerUps from vector that are destroyed AND !activated (thus either off the map or finished)
    // Note we use a lambda expression to remove each PowerUp which is destroyed and not activated
    this->PowerUps.erase(std::remove_if(this->PowerUps.begin(), this->PowerUps.end(),
        [](const PowerUp &powerUp) { return powerUp.Destroyed && !powerUp.Activated; }
    ), this->PowerUps.end());
}

// Calculate chance of spawning
GLboolean ShouldSpawn(GLuint chance)
{
    GLuint random = rand() % chance;
    return random == 0;
}

// Spawn power-ups when necessary
void Game::SpawnPowerUps(GameObject &block)
{
    if (ShouldSpawn(75)) // 1 in 75 chance
        this->PowerUps.push_back(PowerUp("speed", glm::vec3(0.5f, 0.5f, 1.0f), 0.0f, block.Position, ResourceManager::GetTexture("powerup_speed")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("sticky", glm::vec3(1.0f, 0.5f, 1.0f), 20.0f, block.Position, ResourceManager::GetTexture("powerup_sticky")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pass-through", glm::vec3(0.5f, 1.0f, 0.5f), 10.0f, block.Position, ResourceManager::GetTexture("powerup_passthrough")));
    if (ShouldSpawn(75))
        this->PowerUps.push_back(PowerUp("pad-size-increase", glm::vec3(1.0f, 0.6f, 0.4), 0.0f, block.Position, ResourceManager::GetTexture("powerup_increase")));
    if (ShouldSpawn(15)) // Negative powerups should spawn more often
        this->PowerUps.push_back(PowerUp("confuse", glm::vec3(1.0f, 0.3f, 0.3f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_confuse")));
    if (ShouldSpawn(15))    // 1 in 15 chance
        this->PowerUps.push_back(PowerUp("chaos", glm::vec3(0.9f, 0.25f, 0.25f), 15.0f, block.Position, ResourceManager::GetTexture("powerup_chaos")));
}

// Active powerup
void ActivatePowerUp(PowerUp &powerUp)
{
    // Initiate a powerup based type of powerup
    if (powerUp.Type == "speed")
    {
        Ball->Velocity *= 1.2;
    }
    else if (powerUp.Type == "sticky")
    {
        Ball->Sticky = GL_TRUE;
        Player->Color = glm::vec3(1.0f, 0.5f, 1.0f);
    }
    else if (powerUp.Type == "pass-through")
    {
        Ball->PassThrough = GL_TRUE;
        Ball->Color = glm::vec3(1.0f, 0.5f, 0.5f);
    }
    else if (powerUp.Type == "pad-size-increase")
    {
        Player->Size.x += 50;
    }
    else if (powerUp.Type == "confuse")
    {
        if (!Effects->Chaos)
            Effects->Confuse = GL_TRUE; // Only activate if chaos wasn't already active
    }
    else if (powerUp.Type == "chaos")
    {
        if (!Effects->Confuse)
            Effects->Chaos = GL_TRUE;
    }
}
                            
// Check if another powerup of specified type is active
GLboolean IsOtherPowerUpActive(std::vector<PowerUp> &powerUps, std::string type)
{
    // Check if another PowerUp of the same type is still active
    // in which case we don't disable its effect (yet)
    for (const PowerUp &powerUp : powerUps)
    {
        if (powerUp.Activated)
            if (powerUp.Type == type)
                return GL_TRUE;
    }
    return GL_FALSE;
}


// Collision detection
GLboolean CheckCollision(GameObject &one, GameObject &two);
Collision CheckCollision(BallObject &one, GameObject &two);
Direction VectorDirection(glm::vec2 closest);

// Do the collision detection
void Game::DoCollisions()
{
    // For blocks in level
    for (GameObject &box : this->Levels[this->Level].Bricks)
    {
        // if block is not destroyed, check collision with ball
        if (!box.Destroyed)
        {
            Collision collision = CheckCollision(*Ball, box);
            if (std::get<0>(collision)) // If collision is true
            {
                // if block not solid, destroy, add to score, spawn power-up, and play audio cue
                if (!box.IsSolid)
                {
                    box.Destroyed = GL_TRUE;
                    this->Score += 10;
                    this->SpawnPowerUps(box);
                    SoundEngine->play2D("assets/audio/brick.wav", GL_FALSE);
                }
                else
                {
                    // else, cause shake effect, and play audio cue
                    ShakeTime = 0.05f;
                    Effects->Shake = GL_TRUE;
                    SoundEngine->play2D("assets/audio/wall.wav", GL_FALSE);
                }
                // Collision resolution
                Direction dir = std::get<1>(collision);
                glm::vec2 diff_vector = std::get<2>(collision);
                if (!(Ball->PassThrough && !box.IsSolid))
                {
                    if (dir == LEFT || dir == RIGHT) // Horizontal collision
                    {
                        Ball->Velocity.x = -Ball->Velocity.x; // Reverse horizontal velocity
                        // Relocate
                        GLfloat penetration = Ball->Radius - std::abs(diff_vector.x);
                        if (dir == LEFT)
                            Ball->Position.x += penetration; // Move ball to right
                        else
                            Ball->Position.x -= penetration; // Move ball to left;
                    }
                    else // Vertical collision
                    {
                        Ball->Velocity.y = -Ball->Velocity.y; // Reverse vertical velocity
                        // Relocate
                        GLfloat penetration = Ball->Radius - std::abs(diff_vector.y);
                        if (dir == UP)
                            Ball->Position.y -= penetration; // Move ball bback up
                        else
                            Ball->Position.y += penetration; // Move ball back down
                    }
                }
            }
        }
    }
    // For powerups in level, check collision with player.
    for (PowerUp &powerUp : this->PowerUps)
    {
        if (!powerUp.Destroyed)
        {
            if (powerUp.Position.y >= this->Height)
                powerUp.Destroyed = GL_TRUE;
            if (CheckCollision(*Player, powerUp))
            {    // Collided with player, now activate powerup and play audio cue
                ActivatePowerUp(powerUp);
                powerUp.Destroyed = GL_TRUE;
                powerUp.Activated = GL_TRUE;
                SoundEngine->play2D("assets/audio/powerup.wav", GL_FALSE);
            }
        }
    }
    
    // Also check collisions for player pad (unless stuck)
    Collision result = CheckCollision(*Ball, *Player);
    if (!Ball->Stuck && std::get<0>(result))
    {
        Ball->Stuck = Ball->Sticky;
        // Check where it hit the board, and change velocity based on where it hit the board
        GLfloat centerBoard = Player->Position.x + Player->Size.x / 2;
        GLfloat distance = (Ball->Position.x + Ball->Radius) - centerBoard;
        GLfloat percentage = distance / (Player->Size.x / 2);
        // Then move accordingly
        GLfloat strength = 2.0f;
        glm::vec2 oldVelocity = Ball->Velocity;
        Ball->Velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        //Ball->Velocity.y = -Ball->Velocity.y;
        Ball->Velocity = glm::normalize(Ball->Velocity) * glm::length(oldVelocity); // Keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
        // Fix sticky paddle
        Ball->Velocity.y = -1 * abs(Ball->Velocity.y);
        SoundEngine->play2D("assets/audio/paddle.ogg", GL_FALSE);
    }
}

// Collision with two blocks
GLboolean CheckCollision(GameObject &one, GameObject &two) // AABB - AABB collision
{
    // Collision x-axis?
    bool collisionX = one.Position.x + one.Size.x >= two.Position.x &&
        two.Position.x + two.Size.x >= one.Position.x;
    // Collision y-axis?
    bool collisionY = one.Position.y + one.Size.y >= two.Position.y &&
        two.Position.y + two.Size.y >= one.Position.y;
    // Collision only if on both axes
    return collisionX && collisionY;
}

// Collision with ball and block
Collision CheckCollision(BallObject &one, GameObject &two) // AABB - Circle collision
{
    // Get center point circle first
    glm::vec2 center(one.Position + one.Radius);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.Size.x / 2, two.Size.y / 2);
    glm::vec2 aabb_center(two.Position.x + aabb_half_extents.x, two.Position.y + aabb_half_extents.y);
    // Get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // Now that we know the the clamped values, add this to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // Now retrieve vector between center circle and closest point AABB and check if length < radius
    difference = closest - center;
    
    if (glm::length(difference) < one.Radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    else
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}

// Calculates which direction a vector is facing (N,E,S or W)
Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f),    // up
        glm::vec2(1.0f, 0.0f),    // right
        glm::vec2(0.0f, -1.0f),    // down
        glm::vec2(-1.0f, 0.0f)    // left
    };
    GLfloat max = 0.0f;
    GLuint best_match = -1;
    for (GLuint i = 0; i < 4; i++)
    {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max)
        {
            max = dot_product;
            best_match = i;
        }
    }
    return (Direction)best_match;
}
