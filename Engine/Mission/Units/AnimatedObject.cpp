#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "AnimatedObject.h"
#include <cmath>

AnimatedObject::AnimatedObject()
{

}

AnimatedObject::~AnimatedObject()
{

}

// movement
void AnimatedObject::setGlobalPosition(sf::Vector2f newPos)
{
    pos_global = newPos;
}

void AnimatedObject::setLocalPosition(sf::Vector2f newPos)
{
    pos_local = newPos;
}

void AnimatedObject::moveGlobalPosition(sf::Vector2f moveBy)
{
    pos_global = sf::Vector2f(pos_global.x + moveBy.x, pos_global.y + moveBy.y);
}

void AnimatedObject::moveLocalPosition(sf::Vector2f moveBy)
{
    pos_local = sf::Vector2f(pos_local.x + moveBy.x, pos_local.y + moveBy.y);
}

sf::Vector2f AnimatedObject::getGlobalPosition()
{
    return pos_global;
}

sf::Vector2f AnimatedObject::getLocalPosition()
{
    return pos_local;
}

// transformation
void AnimatedObject::setRotation(float newRotation)
{
    rotation = newRotation;
}

void AnimatedObject::rotateBy(float degrees)
{
    rotation += degrees;
}

float AnimatedObject::getRotation()
{
    return rotation;
}

void AnimatedObject::setScale(sf::Vector2f newScale)
{
    scale = newScale;
}

void AnimatedObject::moveScale(sf::Vector2f moveBy)
{
    scale = sf::Vector2f(scale.x + moveBy.x, scale.y + moveBy.y);
}

sf::Vector2f AnimatedObject::getScale()
{
    return scale;
}

// animation control
void AnimatedObject::playAnimation()
{
    animation.isPlaying = true;
}

void AnimatedObject::stopAnimation()
{
    animation.isPlaying = false;
}

void AnimatedObject::setAnimation(std::string& shortAnimName)
{
    animation.setAnimation(shortAnimName);
}

void AnimatedObject::setAnimationSpeed(float newFPS)
{
    animation.animationSpeed = newFPS;
}

void AnimatedObject::setAnimationFrame(int newFrame)
{
    animation.currentFrame = newFrame;
}

std::string AnimatedObject::getAnimation()
{
    return animation.getAnimation();
}

float AnimatedObject::getAnimationSpeed()
{
    return animation.animationSpeed;
}

int AnimatedObject::getAnimationFrame()
{
    return static_cast<unsigned int>(floor(animation.currentFrame));
}

void AnimatedObject::setAnimationLoop(bool newLoop)
{
    animation.isLooping = newLoop;
}

bool AnimatedObject::getAnimationLoop()
{
    return animation.isLooping;
}

void AnimatedObject::LoadConfig(const std::string& anim_path)
{
    animation.Load(anim_path);
}

void AnimatedObject::Draw()
{
    animation.position = sf::Vector2f(pos_global.x + pos_local.x, pos_global.y + pos_local.y);
    animation.scale = scale;
    animation.rotation = rotation;
    animation.Draw();
}

void AnimatedObject::Update()
{

}

// fallback compatibility stuff
void AnimatedObject::setAnimationSegment(std::string new_segment_name) {}
void AnimatedObject::setAnimationSegment(std::string new_segment_name, bool force_start) {}
std::string AnimatedObject::getAnimationSegment() {}
float AnimatedObject::getAnimationPos() {}
void AnimatedObject::setColor(sf::Color c) {}
sf::Color AnimatedObject::getColor() {}
void AnimatedObject::applyEquipment(std::vector<int> item_id, int slot, bool offhand) {}
void AnimatedObject::setLoop(bool a) {}