#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include <Windows.h>
#include "ClientPacketHandler.h"
#include "Minion.h"
#include <iostream>
#include <sstream>
#include "ServerFunc.h"
#include <functional>
#include "Nexus.h"
#include "Inhibitor.h"
#include "Turret.h"
#include "Monster.h"

Room GRoom;

void Room::BlueEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetObjectId();
	WRITE_LOCK;
		_bluePlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//패킷을 생성해서 들어왔다는 것을 모두에게 보여줘야 함.
}

void Room::RedEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetObjectId();
	WRITE_LOCK;
		_redPlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//패킷을 생성해서 들어왔다는 것을 모두에게 보여줘야 함.
}

void Room::Leave(PlayerRef player, GameSessionRef session)
{
	if (session->GetPlayer() == nullptr)
		return;
	uint64 currentPlayerId = session->GetPlayer()->GetObjectId();
	WRITE_LOCK;
		_bluePlayers.erase(currentPlayerId);
		_redPlayers.erase(currentPlayerId);
		_allPlayers.erase(currentPlayerId);



		//TO DO : 패킷을 생성해서 나갔다는 것을 모두에게 보여줘야 함.
}

void Room::Broadcast(SendBufferRef sendBuffer, GameSessionRef session)
{
	WRITE_LOCK;
	for (auto& p : _bluePlayers)
	{
		shared_ptr<GameSession> playerSession = p.second->GetOwnerSession().lock();
		playerSession->Send(sendBuffer);
	}
	for (auto& p : _redPlayers)
	{
		shared_ptr<GameSession> playerSession = p.second->GetOwnerSession().lock();
		playerSession->Send(sendBuffer);
	}
}

void Room::ExcuteAfterTime(SendBufferRef sendBuffer, uint64 milliSeconds)
{
	static atomic<bool> flag = false;

	if (flag.exchange(true)!=false)
		return;

	Sleep(milliSeconds);

	Broadcast(sendBuffer, nullptr);

	flag.exchange(false);
}

void Room::GameStart(SendBufferRef _sendBuffer, uint64 milliSeconds)
{	
	thread t([this, _sendBuffer, milliSeconds]() {
		Sleep(milliSeconds);
		bool success = true;
		int64 playerNum = GRoom.GetPlayerSize();
	map<uint64, PlayerRef> players = GRoom.GetPlayers();

	//챔피언을 픽 안한 사람이 있으면 게임 시작 안함.
	for (auto& p : players) {
		if (p.second->GetChampionType() == 0)
			success = false;
	}

	PKT_S_GAME_START_WRITE pktWriter(success);

	PKT_S_GAME_START_WRITE::PlayerInfoList playerInfoList = pktWriter.ReservePlayerInfoList(playerNum);


	//플레이어들의 초기 위치값 설정
	vector<ObjectMove> playerMove;
	{
		ObjectMove mPlayerMove;
		mPlayerMove.moveDir = ObjectMove::MoveDir{0.f,  -3.14 / 2, 0.f };
		mPlayerMove.pos = ObjectMove::Pos{0.0f, 0.0f, 0.0f };

		// Blue, Red, Blue Red 순으로 나중에 좌표 바꿔야 한다. (현재는 blue 에 다 생김)
		mPlayerMove.pos = ObjectMove::Pos{ 53.0f,30.0f,27.0f };
		playerMove.push_back(mPlayerMove);

		mPlayerMove.pos = ObjectMove::Pos{ 95.0f, 30.0f, 41.0f };
		playerMove.push_back(mPlayerMove);

		mPlayerMove.pos = ObjectMove::Pos{ 88.0f, 30.0f, 86.0f };
		playerMove.push_back(mPlayerMove);

		mPlayerMove.pos = ObjectMove::Pos{47.f, 30.f, 97.f };
		playerMove.push_back(mPlayerMove);

	}

	//플레이어 정보를 패킷에 넣음
	int i = 0;
	for (auto& p : players) {
		cout << "Player " << to_string(p.second->GetObjectId()) << "의 정보입니다." << endl;
		playerInfoList[i].champion = p.second->GetChampionType();
		playerInfoList[i].faction = p.second->GetFaction();
		playerInfoList[i].id = p.second->GetObjectId();
		playerInfoList[i].posInfo = playerMove[i];
		playerInfoList[i].host = p.second->GetHost();

		//플레이어 문자열을 안에 넣음
		int64 nickNameSize = p.second->GetName().size();
		wstring nickName = p.second->GetName();
		PKT_S_GAME_START_WRITE::NickNameList nick = pktWriter.ReserveNickNameList(&playerInfoList[i], nickNameSize);
		for (int j = 0; j < nickNameSize; j++) {
			nick[j] = { nickName[j] };
		}
		i++;
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	Broadcast(sendBuffer, nullptr);

	thread t1(std::bind(&Room::GameStartSpawn, this, _sendBuffer, 500, success));
	t1.detach();
		});

	t.detach();
	
}

void Room::GameStartSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds, bool _success)
{
	thread t1([this, milliSeconds, _success]() {
		int a = 0;

		if (_success == false) {
			cout << "GameStart실패해서 Spawn패킷 실행하지 않음";
			return;
		}

		Sleep(milliSeconds);

		NexusSpawn(milliSeconds);

		InhibitorSpawn(milliSeconds);

		TurretSpawn(milliSeconds);

		thread t2(std::bind(&Room::TimeThread, this));

		t2.detach();

		Sleep(3000);

		MinionSpawn(1000, 0);

		for (int i = (int)UnitType::SOUTH_GROMP; i <= (int)UnitType::BARON; i++) {
			MonsterSpawn(1000, (UnitType)i);
		}	

	});

	t1.detach();
}

void Room::TimeThread()
{
	for (int seconds = 0; seconds <= 3600; seconds++) {
		second = seconds;
		cout << "게임 시작 후 " << seconds << "초가 지났습니다." << endl;
		if (GetPlayerSize() == 0)
			return;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Room::RemoveObject(uint64 _objectID)
{
	WRITE_LOCK;
	//_bluePlayers.erase(_objectID);
	//_redPlayers.erase(_objectID);
	//_allPlayers.erase(_objectID);
	auto it1 = _monsters.find(_objectID);
	if (it1 != _monsters.end()) {
		cout << "정글몹이 사망함." << endl;
		uint64 monsterID = it1->first;
		UnitType monsterType = it1->second;
		{
			WRITE_LOCK;
			_monsters.erase(monsterID);
		}
		thread t1(std::bind(&Room::MonsterSpawn, this, 10000, monsterType));
		t1.detach();
	}
	else {
		return;
	}

	_blueMinions.erase(_objectID);
	_redMinions.erase(_objectID);

	auto it2 = _blueInhibitors.find(_objectID);
	if (it2 != _blueInhibitors.end()) {
		cout << "블루팀 억제기가 파괴됨" << endl;
		uint64 inhibitorID = it2->first;
		InhibitorRef inhibitorRef = it2->second;
		_brokenInhibitors[inhibitorID] = inhibitorRef;

		thread t3(std::bind(&Room::InhibitorRespawn, this, 300000, inhibitorRef));
		t3.detach();
	}
	else {
		return;
	}

	auto it3 = _redInhibitors.find(_objectID);
	if (it3 != _redInhibitors.end()) {
		cout << "레드팀 억제기가 파괴됨" << endl;
		uint64 inhibitorID = it3->first;
		InhibitorRef inhibitorRef = it3->second;
		_brokenInhibitors[inhibitorID] = inhibitorRef;

		thread t3(std::bind(&Room::InhibitorRespawn, this, 300000, inhibitorRef));
		t3.detach();
	}
	else {
		return;
	}
}

void Room::NexusSpawn( uint64 milliSeconds)
{
	{
		cout << "블루 넥서스 생성" << endl;
		NexusRef nexusRef = MakeShared<Nexus>();

		//ObjectInfo 설정
		ObjectInfo& objectInfo = nexusRef->GetObjectInfo();
		ObjectMove::MoveDir moveDir = { .0f,.0f,.0f };
		ObjectMove::Pos pos = { 229.7f ,15.9f, 241.5f };
		CC CCType = CC::CLEAR;
		ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
		SetObjectInfo(objectInfo, nexusRef->GetObjectId(), UnitType::NEXUS, Faction::BLUE, Lane::NONE, objectMove);

		PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		Broadcast(sendBuffer, nullptr);
	}

	{
		cout << "레드 넥서스 생성" << endl;
		NexusRef nexusRef = MakeShared<Nexus>();

		//ObjectInfo 설정
		ObjectInfo& objectInfo = nexusRef->GetObjectInfo();
		ObjectMove::MoveDir moveDir = { .0f,.0f,.0f };
		ObjectMove::Pos pos = { 1952.174f ,15.26f, 1956.22f };
		CC CCType = CC::CLEAR;
		ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
		SetObjectInfo(objectInfo, nexusRef->GetObjectId(), UnitType::NEXUS, Faction::RED, Lane::NONE, objectMove);

		PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		Broadcast(sendBuffer, nullptr);
	}
}

void Room::InhibitorSpawn( uint64 milliSeconds)
{
	for (int j = 0; j < 3; j++) {
		Lane laneType = Lane::END;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		ObjectMove::MoveDir blueDir = {};
		ObjectMove::MoveDir redDir = {};
		if (j == 0) {
			laneType = Lane::TOP;
			bluePos = { 169.86f, 14.2f, 527.02f };
			redPos = { 1661.7f,14.8f,2013.9f };
			blueDir = { 0.f,-89.48f,0.f };
			redDir = { -180.f,0.f,-180.f };
		}
		if (j == 1) {
			laneType == Lane::MID;
			bluePos = { 475.717f, 14.2f, 473.633f };
			redPos = { 1711.f,14.8f,1721.f };
			blueDir = { 0.f,-45.f,0.f };
			redDir = { -180.f,0.f,-180.f };
		}
		if (j == 2) {
			laneType == Lane::BOTTOM;
			bluePos = { 501.97f, 14.2f, 183.0f };
			redPos = { 2006.9f,14.8f,1670.1f };
			blueDir = { 0.f,2.f,0.f };
			redDir = { 0.f,90.f,0.f };
		}
		{
			cout << "블루 억제기 생성" << endl;
			InhibitorRef inhibitorRef = MakeShared<Inhibitor>();
			_blueInhibitors[inhibitorRef->GetObjectId()] = inhibitorRef;

			//ObjectInfo 설정
			ObjectInfo& objectInfo = inhibitorRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, inhibitorRef->GetObjectId(), UnitType::INHIBITOR, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "레드 억제기 생성" << endl;
			InhibitorRef inhibitorRef = MakeShared<Inhibitor>();
			_redInhibitors[inhibitorRef->GetObjectId()] = inhibitorRef;

			//ObjectInfo 설정
			ObjectInfo& objectInfo = inhibitorRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, inhibitorRef->GetObjectId(), UnitType::INHIBITOR, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}
}

void Room::TurretSpawn( uint64 milliSeconds)
{
	//1차타워 생성
	for (int j = 0; j < 3; j++) {
		Lane laneType = Lane::END;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		ObjectMove::MoveDir blueDir = {};
		ObjectMove::MoveDir redDir = {};
		if (j == 0) {
			laneType = Lane::TOP;
			bluePos = { 149.798f, 12.f, 1527.123f };
			redPos = { 638.259f, 12.0f, 2046.865f };
			blueDir = { 0.f, -2.155f, 0.f };
			redDir = { 0.f, 90.f, 0.f };
		}
		if (j == 1) {
			laneType = Lane::MID;
			bluePos = { 861.113f, 12.f, 941.186f };
			redPos = { 1328.513f, 12.0f, 1250.263f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType = Lane::BOTTOM;
			bluePos = { 1546.112f, 12.f, 151.179f };
			redPos = { 2045.903f, 12.0f, 655.733f };
			blueDir = { 174.047f, 87.112f, 172.7f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "1차 블루 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);

			for (int i = 0; i < 4; i++) {
				SetObjectInfo(objectInfo, turretRef->GetObjectId() + i, UnitType::TURRET, Faction::BLUE, laneType, objectMove);
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
		{
			cout << "1차 레드 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);

			for (int i = 0; i < 4; i++) {
				SetObjectInfo(objectInfo, turretRef->GetObjectId()+i, UnitType::TURRET, Faction::RED, laneType, objectMove);
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
	}

	//2차타워 생성
	for (int j = 0; j < 3; j++) {
		Lane laneType = Lane::END;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		ObjectMove::MoveDir blueDir = {};
		ObjectMove::MoveDir redDir = {};
		if (j == 0) {
			laneType = Lane::TOP;
			bluePos = { 225.798f, 12.f, 971.123f };
			redPos = { 1167.060f, 12.0f, 1977.248f };
			blueDir = { -180.f, -2.596f, -180.f };
			redDir = { 0.f, 90.f, 0.f };
		}
		if (j == 1) {
			laneType = Lane::MID;
			bluePos = { 743.113f, 12.f, 703.186f };
			redPos = { 1439.891f, 12.0f, 1487.584f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType = Lane::BOTTOM;
			bluePos = { 1017.112f, 12.f, 221.179f };
			redPos = { 1965.386f, 12.0f, 1206.867f };
			blueDir = { 0.f, -83.897f, 0.f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "2차 블루 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "2차 레드 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}

	//3차타워 생성
	for (int j = 0; j < 3; j++) {
		Lane laneType = Lane::END;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		ObjectMove::MoveDir blueDir = {};
		ObjectMove::MoveDir redDir = {};
		if (j == 0) {
			laneType = Lane::TOP;
			bluePos = { 173.798f, 12.f, 634.123f };
			redPos = { 1545.206f, 12.0f, 2013.792f };
			blueDir = { -180.f, -5.714f, -180.f };
			redDir = { 0.f, 90.f, 0.f };
		}
		if (j == 1) {
			laneType = Lane::MID;
			bluePos = { 545.113f, 12.f, 549.186f };
			redPos = { 1643.650f, 12.0f, 1654.128f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType = Lane::BOTTOM;
			bluePos = { 632.851f, 12.f, 183.241f };
			redPos = { 2009.956f, 12.0f, 1561.171f };
			blueDir = { 0.f, -83.897f, 0.f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "3차 블루 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "3차 레드 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}

	//쌍둥이
	for (int j = 0; j < 2; j++) {
		Lane laneType = Lane::NONE;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		ObjectMove::MoveDir blueDir = {};
		ObjectMove::MoveDir redDir = {};
		if (j == 0) {
			bluePos = { 257.426f, 12.0f, 336.225f };
			redPos = { 1860.699f, 12.0f, 1933.449f };
			blueDir = { 180.f, -34.597f, -180.0f };
			redDir = { 0.f, 69.658f, 0.f };
		}
		if (j == 1) {
			bluePos = { 325.845f, 12.0f, 261.304f };
			redPos = { 1926.281f, 12.0f, 1863.799f };
			blueDir = { 180.f, -60.597f, -180.0f };
			redDir = { 0.f, 40.125f, 0.f };
		}
		{
			cout << "블루 쌍둥이 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "레드 쌍둥이 타워 생성" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo 설정
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}
}

void Room::MinionSpawn( uint64 _spawnTickTime, uint64 _spawnTime)
{
	Sleep(_spawnTime);

	//슈퍼미니언 생성 방복문
	for (auto it = _brokenInhibitors.begin(); it != _brokenInhibitors.end(); it++) {
		Faction inhibitorFaction = it->second->GetFaction();
		Lane inhibitorLane = it->second->GetObjectInfo().lane;

		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};

		if (inhibitorFaction == Faction::RED) {
			if (inhibitorLane == Lane::TOP) {
				bluePos = { 165.0f, 12.0f, 309.0f };

				cout << "블루 슈퍼미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::BLUE, inhibitorLane, objectMove);

				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else if (inhibitorLane == Lane::MID) {
				bluePos = { 300.0f, 12.0f, 300.0f };

				cout << "블루 슈퍼미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::BLUE, inhibitorLane, objectMove);

				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else if (inhibitorLane == Lane::BOTTOM) {
				bluePos = { 308.0f, 12.0f, 181.0f };

				cout << "블루 슈퍼미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::BLUE, inhibitorLane, objectMove);

				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else {
				cout << "ERROR : 억제기의 라인이 지정이 안되어 있습니다." << endl;
			}
		}
		else if (inhibitorFaction == Faction::BLUE) {
			if (inhibitorLane == Lane::TOP) {
				redPos = { 1882.0,12.0,2036.0 };

				cout << "레드 슈퍼 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::RED, inhibitorLane, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else if (inhibitorLane == Lane::MID) {
				redPos = { 1883.0f,12.0f,1906.0f };

				cout << "레드 슈퍼 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::RED, inhibitorLane, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else if (inhibitorLane == Lane::BOTTOM) {
				redPos = { 2013.0f,12.0f,1911.0f };

				cout << "레드 슈퍼 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SUPER_MINION, Faction::RED, inhibitorLane, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			else {
				cout << "ERROR : 억제기의 라인이 지정이 안되어 있습니다." << endl;
			}
		}
		else {
			cout << "ERROR : 억제기의 Faction이 지정이 안되어 있습니다." << endl;
		}
	}

	//한 라인에 총 3마리의 근접 미니언 생성
	for (int i = 0; i < 1; i++) {
		//3라인당 미니언 생성
		for (int j = 0; j < 3; j++) {
			Lane laneType = Lane::END;
			ObjectMove::Pos bluePos = {};
			ObjectMove::Pos redPos = {};
			if (j == 0) {
				laneType = Lane::TOP;
				bluePos = { 165.0f, 12.0f, 309.0f };
				redPos = { 1882.0,12.0,2036.0 };
			}
			else if (j == 1) {
				laneType = Lane::MID;
				bluePos = { 300.0f, 12.0f, 300.0f };
				redPos = { 1883.0f,12.0f,1906.0f };
			}
			else if (j == 2) {
				laneType = Lane::BOTTOM;
				bluePos = { 308.0f, 12.0f, 181.0f };
				redPos = { 2013.0f,12.0f,1911.0f };
			}
			{
				if(laneType == Lane::END) laneType = Lane::TOP;

				cout << "블루 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_blueMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::MELEE_MINION, Faction::BLUE, laneType, objectMove);

				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			{
				cout << "레드 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::MELEE_MINION, Faction::RED, laneType, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
		Sleep(1000);
	}

	//대포미니언 생성
	for (int j = 0; j < 3; j++) {
		Lane laneType = Lane::END;
		ObjectMove::Pos bluePos = {};
		ObjectMove::Pos redPos = {};
		if (j == 0) {
			laneType = Lane::TOP;
			bluePos = { 165.0f, 12.0f, 309.0f };
			redPos = { 1882.0,12.0,2036.0 };
		}
		else if (j == 1) {
			laneType = Lane::MID;
			bluePos = { 300.0f, 12.0f, 300.0f };
			redPos = { 1883.0f,12.0f,1906.0f };
		}
		else if (j == 2) {
			laneType = Lane::BOTTOM;
			bluePos = { 308.0f, 12.0f, 181.0f };
			redPos = { 2013.0f,12.0f,1911.0f };
		}
		{
			if (laneType == Lane::END) laneType = Lane::TOP;

			cout << "블루 대포 미니언 생성" << endl;
			MinionRef minionRef = MakeShared<Minion>();
			_blueMinions[minionRef->GetObjectId()] = minionRef;

			//ObjectInfo 설정
			ObjectInfo& objectInfo = minionRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SIEGE_MINION, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "레드 대포 미니언 생성" << endl;
			MinionRef minionRef = MakeShared<Minion>();
			_redMinions[minionRef->GetObjectId()] = minionRef;

			//ObjectInfo 설정
			ObjectInfo& objectInfo = minionRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::SIEGE_MINION, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}
	Sleep(1000);

	//한 라인에 총 3마리의 원거리 미니언 생성
	for (int i = 0; i < 2; i++) {
		//3라인당 미니언 생성 //j가 0이면 탑, 1이면 미드, 2면 바텀
		for (int j = 0; j < 3; j++) {
			Lane laneType = Lane::END;
			ObjectMove::Pos bluePos = {};
			ObjectMove::Pos redPos = {};
			if (j == 0) {
				laneType = Lane::TOP;
				bluePos = { 165.0f, 12.0f, 309.0f };
				redPos = { 1882.0,12.0,2036.0 };
			}
			else if (j == 1) {
				laneType = Lane::MID;
				bluePos = { 300.0f, 12.0f, 300.0f };
				redPos = { 1883.0f,12.0f,1906.0f };
			}
			else if (j == 2) {
				laneType = Lane::BOTTOM;
				bluePos = { 292.0f, 12.0f, 191.0f };
				redPos = { 2013.0f,12.0f,1911.0f };
			}
			{
				if (laneType == Lane::END) laneType = Lane::TOP;

				cout << "블루 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_blueMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::RANGED_MINION, Faction::BLUE, laneType, objectMove);

				//패킷 생성
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			{
				cout << "레드 미니언 생성" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo 설정
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::RANGED_MINION, Faction::RED, laneType, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
		Sleep(1000);
	}

	cout << "미니언 생성 패킷을 다 보냄" << endl;

	////30000을 바꾸면 리스폰 시간이 변경됩니다.
	// 사용시 주석을 해제해주세요.
	//thread t3(std::bind(&Room::MinionSpawn, this, 1000, 30000));
	//t3.detach();
}

void Room::MonsterSpawn( uint64 milliSeconds, UnitType _monsterType)
{
	Sleep(milliSeconds);
	cout << "몬스터 생성" << endl;
	MonsterRef monsterRef = MakeShared<Monster>(_monsterType);

	//ObjectInfo 설정
	ObjectInfo& objectInfo = monsterRef->GetObjectInfo();
	ObjectMove::MoveDir moveDir = { 0.f,0.f,0.f };
	ObjectMove::Pos pos = { 10.f, 10.f, 10.f };
	CC CCType = CC::CLEAR;
	ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, 100.f, 100.f, false, moveDir, pos, CCType);
	SetObjectInfo(objectInfo, monsterRef->GetObjectId(), _monsterType, Faction::NONE, Lane::NONE, objectMove);

	{
		WRITE_LOCK;
		_monsters[monsterRef->GetObjectId()] = monsterRef->GetObjectInfo().unitType;
	}

	//패킷 생성
	PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
	Broadcast(sendBuffer, nullptr);
}

void Room::InhibitorRespawn(uint64 milliSeconds, InhibitorRef _inhibitorRef)
{
	Sleep(milliSeconds);

	PKT_S_SPAWN_OBJECT_WRITE pktWriter(_inhibitorRef->GetObjectInfo());
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
	Broadcast(sendBuffer, nullptr);

	_brokenInhibitors.erase(_inhibitorRef->GetObjectId());
}

void Room::SetObjectInfo(OUT ObjectInfo& _objectInfo, uint64 _objectId, UnitType _unitType, Faction _faction, Lane _lane, ObjectMove _objectMove)
{
	_objectInfo.objectId = _objectId;
	_objectInfo.unitType = _unitType;
	_objectInfo.faction = _faction;
	_objectInfo.lane = _lane;
	_objectInfo.objectMove = _objectMove;
}

void Room::SetObjectMove(OUT ObjectMove& _objectMove, int _LV, float _HP, float _MP, float _AttackPower, float _DefencePower, ObjectMove::MoveDir _moveDir, ObjectMove::Pos _pos)
{
	_objectMove.LV = _LV;
	_objectMove.HP = _HP;
	_objectMove.MP = _MP;
	_objectMove.AttackPower = _AttackPower;
	_objectMove.DefencePower = _DefencePower;
	_objectMove.moveDir = _moveDir;
	_objectMove.pos = _pos;
}