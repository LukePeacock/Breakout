//
//  ball_object.hpp
//  Breakout
//
//  Created by Luke on 21/11/2019.
//  Copyright © 2019 Luke Peacock. All rights reserved.
//

#ifndef ball_object_hpp
#define ball_object_hpp

#include "texture.hpp"
#include "sprite_renderer.hpp"
#include "game_object.hpp"

class BallObject : public GameObject
{
    public:
        // Ball state
        GLfloat   Radius;
        GLboolean Stuck;
  

        BallObject();
        BallObject(glm::vec2 pos, GLfloat radius, glm::vec2 velocity, Texture2D sprite);

        glm::vec2 Move(GLfloat dt, GLuint window_width);
        void      Reset(glm::vec2 position, glm::vec2 velocity);
};

#endif /* ball_object_hpp */
