#pragma once
#include <vector>
#include "SFML/Graphics.hpp"

struct Float_State
{
    std::vector<float> state;
};

struct Float_Step_Return
{
    sf::Image state;
    sf::Image next_state;
    float action;
    float reward;
    float terminated;
    float truncated;
};

struct Full_Float_Step_Return
{
    std::vector<float> data;
};