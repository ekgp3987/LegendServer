#pragma once
#include "Session.h"

class GameSession : public PacketSession
{
public:
	~GameSession()
	{
		cout << "~GameSession" << endl;
	}

	virtual void OnConnected() override;
	virtual void OnDisconnected() override;
	virtual void OnRecvPacket(BYTE* buffer, int32 len) override;
	virtual void OnSend(int32 len) override;

public:
	PlayerRef GetPlayer() { return player; }
	void SetPlayer(PlayerRef _player) { player = _player; }

private:
	PlayerRef player;
};