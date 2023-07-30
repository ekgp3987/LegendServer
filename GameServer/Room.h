 #pragma once
#include "ServerFunc.h"
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
	void TimeThread();

	void SetObjectInfo(OUT ObjectInfo& _objectInfo, uint64 _objectId, ObjectType _objectType, Faction _factionType, Lane _laneType, ObjectMove _objectMove);
	void SetObjectMove(OUT ObjectMove& _objectMove, int _LV, float _HP, float _MP, float _AD, float _Defence, ObjectMove::MoveDir _moveDir, ObjectMove::Pos _pos);

	uint64 GetPlayerSize() { return _bluePlayers.size() + _redPlayers.size(); }
	uint64 GetBluePlayerSize() { return _bluePlayers.size() ; }
	uint64 GetRedPlayerSize() { return _redPlayers.size(); }
	map<uint64, PlayerRef> GetPlayers() { return _allPlayers; }
	PlayerRef GetPlayer(uint64 _playerId) { return _allPlayers[_playerId]; }

private:
	USE_LOCK;
	atomic<uint64>				roomId;
	map<uint64, PlayerRef> _bluePlayers;
	map<uint64, PlayerRef> _redPlayers;
	map<uint64, PlayerRef> _allPlayers;

	map<uint64, MinionRef> _blueMinions;
	map<uint64, MinionRef> _redMinions;

	float second;
};

extern Room GRoom;