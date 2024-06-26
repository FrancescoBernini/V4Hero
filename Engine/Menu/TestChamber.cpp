#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include "TestChamber.h"
#include "CoreManager.h"
#include <spdlog/spdlog.h>

TestChamber::TestChamber()
{
    Initialize();
}

void TestChamber::Initialize()
{
    bg.setFillColor(sf::Color::White);

    sf::Clock loadSpeed;
    sf::Int64 cur=0, old=0;
    sf::Int64 bench1,bench2,bench3;
    cur = loadSpeed.getElapsedTime().asMicroseconds();
    SPDLOG_INFO("Load timer: {}us", cur-old);
    old = cur;
    yaripon.LoadConfig("resources/units/unit/yaripon.zip");
    cur = loadSpeed.getElapsedTime().asMicroseconds();
    bench1 = cur-old;
    old = cur;
    hatapon.LoadConfig("resources/units/unit/hatapon.zip");
    cur = loadSpeed.getElapsedTime().asMicroseconds();
    bench2 = cur-old;
    old = cur;
    dropped_item.LoadConfig("resources/units/unit/dropped_item.zip");
    cur = loadSpeed.getElapsedTime().asMicroseconds();
    bench3 = cur-old;
    SPDLOG_INFO("Loading benchmark:");
    SPDLOG_INFO("Yaripon 1: {}s ({}us)", bench1 / 1000000.f, bench1);
    SPDLOG_INFO("Yaripon 2: {}s ({}us)", bench2 / 1000000.f, bench2);
    SPDLOG_INFO("Yaripon 3: {}s ({}us)", bench3 / 1000000.f, bench3);

    initialized = true;
    SPDLOG_DEBUG("Welcome to the Test Chamber");
}

void TestChamber::Update()
{
    sf::RenderWindow* window = CoreManager::getInstance().getWindow();

    bg.setSize(sf::Vector2f(window->getSize().x,window->getSize().y));
    window->draw(bg);

    yaripon.pos_global = sf::Vector2f(400, 200);
    yaripon.Draw();

    hatapon.pos_global = sf::Vector2f(100, 200);
    hatapon.Draw();

    dropped_item.pos_global = sf::Vector2f(100, 500);
    dropped_item.Draw();
}

TestChamber::~TestChamber()
{

}