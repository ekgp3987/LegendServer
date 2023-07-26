#include "pch.h"
#include "Player.h"
#include "ServerFunc.h"

Player::Player()
	: Object(Object_Type::PLAYER)
	, name(nullptr)
{
}

Player::~Player()
{
}
