//
//  power_up.hpp
//  Breakout
//
//  Created by Luke on 21/11/2019.
//  Copyright © 2019 Luke Peacock. All rights reserved.
//

#ifndef power_up_hpp
#define power_up_hpp
#include <game_object.hpp>

const glm::vec2 SIZE(60, 20);
const glm::vec2 VELOCITY(0.0f, 150.0f);

class PowerUp : public GameObject
{
public:
    // PowerUp State
    std::string Type;
    GLfloat     Duration;
    GLboolean   Activated;
    // Constructor
    PowerUp(std::string type, glm::vec3 color, GLfloat duration,
            glm::vec2 position, Texture2D texture)
        : GameObject(position, SIZE, texture, color, VELOCITY),
          Type(type), Duration(duration), Activated()
    { }
};

#endif /* power_up_hpp */
