 #pragma once
#include <ctime>

class Room
{
public:
	void BlueEnter(PlayerRef player, GameSessionRef session);
	void RedEnter(PlayerRef player, GameSessionRef session);
	void Leave(PlayerRef player, GameSessionRef session);
	void Broadcast(SendBufferRef sendBuffer, GameSessionRef session);
	void ExcuteAfterTime(SendBufferRef sendBuffer, uint64 milliSeconds);
	void GameStart(SendBufferRef sendBuffer, uint64 milliSeconds);
	void GameStartSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds, bool _success);

	uint64 GetPlayerSize() { return _bluePlayers.size() + _redPlayers.size(); }
	uint64 GetBluePlayerSize() { return _bluePlayers.size() ; }
	uint64 GetRedPlayerSize() { return _redPlayers.size(); }
	map<uint64, PlayerRef> GetPlayers() { return _allPlayers; }

private:
	USE_LOCK;
	atomic<uint64>				roomId;
	map<uint64, PlayerRef> _bluePlayers;
	map<uint64, PlayerRef> _redPlayers;
	map<uint64, PlayerRef> _allPlayers;

	map<uint64, MinionRef> _blueMinions;
	map<uint64, MinionRef> _redMinions;
};

extern Room GRoom;