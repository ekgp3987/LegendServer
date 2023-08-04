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

Room GRoom;

void Room::BlueEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetObjectId();
	WRITE_LOCK;
		_bluePlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//��Ŷ�� �����ؼ� ���Դٴ� ���� ��ο��� ������� ��.
}

void Room::RedEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetObjectId();
	WRITE_LOCK;
		_redPlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//��Ŷ�� �����ؼ� ���Դٴ� ���� ��ο��� ������� ��.
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



		//TO DO : ��Ŷ�� �����ؼ� �����ٴ� ���� ��ο��� ������� ��.
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

	//è�Ǿ��� �� ���� ����� ������ ���� ���� ����.
	for (auto& p : players) {
		if (p.second->GetChampionType() == 0)
			success = false;
	}

	PKT_S_GAME_START_WRITE pktWriter(success);

	PKT_S_GAME_START_WRITE::PlayerInfoList playerInfoList = pktWriter.ReservePlayerInfoList(playerNum);


	//�÷��̾���� �ʱ� ��ġ�� ����
	vector<ObjectMove> playerMove;
	{
		ObjectMove mPlayerMove;
		mPlayerMove.moveDir = ObjectMove::MoveDir{ 0.0f,0.0f,0.0f };
		mPlayerMove.pos = ObjectMove::Pos{ 0.0f,0.0f,0.0f };

		mPlayerMove.pos = ObjectMove::Pos{ 10.0f,0.0f,10.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = ObjectMove::Pos{ 30.0f,0.0f,10.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = ObjectMove::Pos{ 10.0f,0.0f,100.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = ObjectMove::Pos{ 30.0f,0.0f,10.0f };
		playerMove.push_back(mPlayerMove);
	}

	//�÷��̾� ������ ��Ŷ�� ����
	int i = 0;
	for (auto& p : players) {
		cout << "Player " << to_string(p.second->GetObjectId()) << "�� �����Դϴ�." << endl;
		playerInfoList[i].champion = p.second->GetChampionType();
		playerInfoList[i].faction = p.second->GetFaction();
		playerInfoList[i].id = p.second->GetObjectId();
		playerInfoList[i].posInfo = playerMove[i];
		playerInfoList[i].host = p.second->GetHost();

		//�÷��̾� ���ڿ��� �ȿ� ����
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

	//thread t1(TimeThread);

	GameStartSpawn(_sendBuffer, 3000, success);

	//t1.detach();
		});

	t.detach();
	
}

void Room::GameStartSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds, bool _success)
{
	thread t2(std::bind(&Room::TimeThread, this));

	thread t1([this, _sendBuffer, milliSeconds, _success]() {
		int a = 0;

		if (_success == false) {
			cout << "GameStart�����ؼ� Spawn��Ŷ �������� ����";
			return;
		}

		Sleep(milliSeconds);		

		NexusSpawn(_sendBuffer, milliSeconds);

		InhibitorSpawn(_sendBuffer, milliSeconds);
		
		TurretSpawn(_sendBuffer, milliSeconds);

		MinionSpawn(_sendBuffer, milliSeconds);

	});

	t1.detach();
	t2.detach();
}

void Room::TimeThread()
{
	for (int seconds = 0; seconds <= 3600; seconds++) {
		second = seconds;
		cout << "���� ���� �� " << seconds << "�ʰ� �������ϴ�." << endl;
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
	_blueMinions.erase(_objectID);
	_redMinions.erase(_objectID);
}

void Room::NexusSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds)
{
	{
		cout << "��� �ؼ��� ����" << endl;
		NexusRef nexusRef = MakeShared<Nexus>();

		//ObjectInfo ����
		ObjectInfo& objectInfo = nexusRef->GetObjectInfo();
		ObjectMove::MoveDir moveDir = { .0f,.0f,.0f };
		ObjectMove::Pos pos = { 229.7f ,15.9f, 241.5f };
		CC CCType = CC::CLEAR;
		ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
		SetObjectInfo(objectInfo, nexusRef->GetObjectId(), UnitType::NEXUS, Faction::BLUE, Lane::NONE, objectMove);

		PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		Broadcast(sendBuffer, nullptr);
	}

	{
		cout << "���� �ؼ��� ����" << endl;
		NexusRef nexusRef = MakeShared<Nexus>();

		//ObjectInfo ����
		ObjectInfo& objectInfo = nexusRef->GetObjectInfo();
		ObjectMove::MoveDir moveDir = { .0f,.0f,.0f };
		ObjectMove::Pos pos = { 1952.174f ,15.26f, 1956.22f };
		CC CCType = CC::CLEAR;
		ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
		SetObjectInfo(objectInfo, nexusRef->GetObjectId(), UnitType::NEXUS, Faction::RED, Lane::NONE, objectMove);

		PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		Broadcast(sendBuffer, nullptr);
	}
}

void Room::InhibitorSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds)
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
			cout << "��� ������ ����" << endl;
			InhibitorRef inhibitorRef = MakeShared<Inhibitor>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = inhibitorRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, inhibitorRef->GetObjectId(), UnitType::INHIBITOR, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "���� ������ ����" << endl;
			InhibitorRef inhibitorRef = MakeShared<Inhibitor>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = inhibitorRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, inhibitorRef->GetObjectId(), UnitType::INHIBITOR, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}
}

void Room::TurretSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds)
{
	//1��Ÿ�� ����
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
			laneType == Lane::MID;
			bluePos = { 861.113f, 12.f, 941.186f };
			redPos = { 1328.513f, 12.0f, 1250.263f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType == Lane::BOTTOM;
			bluePos = { 1546.112f, 12.f, 151.179f };
			redPos = { 2045.903f, 12.0f, 655.733f };
			blueDir = { 174.047f, 87.112f, 172.7f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "1�� ��� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "1�� ���� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}

	//2��Ÿ�� ����
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
			laneType == Lane::MID;
			bluePos = { 743.113f, 12.f, 703.186f };
			redPos = { 1439.891f, 12.0f, 1487.584f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType == Lane::BOTTOM;
			bluePos = { 1017.112f, 12.f, 221.179f };
			redPos = { 1965.386f, 12.0f, 1206.867f };
			blueDir = { 0.f, -83.897f, 0.f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "2�� ��� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "2�� ���� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}

	//3��Ÿ�� ����
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
			laneType == Lane::MID;
			bluePos = { 545.113f, 12.f, 549.186f };
			redPos = { 1643.650f, 12.0f, 1654.128f };
			blueDir = { 180.f, -47.597f, -180.f };
			redDir = { 0.f, 46.978f, 0.f };
		}
		if (j == 2) {
			laneType == Lane::BOTTOM;
			bluePos = { 511.851f, 12.f, 179.241f };
			redPos = { 2009.956f, 12.0f, 1561.171f };
			blueDir = { 0.f, -83.897f, 0.f };
			redDir = { 0.f, -3.962f, 0.f };
		}
		{
			cout << "3�� ��� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "3�� ���� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}

	//�ֵ���
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
			cout << "��� �ֵ��� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = blueDir;
			ObjectMove::Pos pos = bluePos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::BLUE, laneType, objectMove);

			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
		{
			cout << "���� �ֵ��� Ÿ�� ����" << endl;
			TurretRef turretRef = MakeShared<Turret>();

			//ObjectInfo ����
			ObjectInfo& objectInfo = turretRef->GetObjectInfo();
			ObjectMove::MoveDir moveDir = redDir;
			ObjectMove::Pos pos = redPos;
			CC CCType = CC::CLEAR;
			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
			SetObjectInfo(objectInfo, turretRef->GetObjectId(), UnitType::TURRET, Faction::RED, laneType, objectMove);


			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
			Broadcast(sendBuffer, nullptr);
		}
	}
}

void Room::MinionSpawn(SendBufferRef _sendBuffer, uint64 milliSeconds)
{
	//�� ���ο� �� 3������ ���� �̴Ͼ� ����
	for (int i = 0; i < 3; i++) {
		//3���δ� �̴Ͼ� ����
		for (int j = 0; j < 3; j++) {
			Lane laneType = Lane::END;
			ObjectMove::Pos bluePos = {};
			ObjectMove::Pos redPos = {};
			if (j == 0) {
				laneType = Lane::TOP;
				bluePos = { 165.0f, 12.0f, 309.0f };
				redPos = { 1882.0,12.0,2036.0 };
			}
			if (j == 1) {
				laneType == Lane::MID;
				bluePos = { 300.0f, 12.0f, 300.0f };
				redPos = { 1883.0f,12.0f,1906.0f };
			}
			if (j == 2) {
				laneType == Lane::BOTTOM;
				bluePos = { 292.0f, 12.0f, 191.0f };
				redPos = { 2013.0f,12.0f,1911.0f };
			}
			{
				cout << "��� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_blueMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo ����
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::MELEE_MINION, Faction::BLUE, laneType, objectMove);

				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			{
				cout << "���� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo ����
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::MELEE_MINION, Faction::RED, laneType, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
		Sleep(500);
	}

	//�� ���ο� �� 3������ ���Ÿ� �̴Ͼ� ����
	for (int i = 0; i < 3; i++) {
		//3���δ� �̴Ͼ� ���� //j�� 0�̸� ž 1�̸� �̵� 2�� ����
		for (int j = 0; j < 3; j++) {
			Lane laneType = Lane::END;
			ObjectMove::Pos bluePos = {};
			ObjectMove::Pos redPos = {};
			if (j == 0) {
				laneType = Lane::TOP;
				bluePos = { 165.0f, 12.0f, 309.0f };
				redPos = { 1882.0,12.0,2036.0 };
			}
			if (j == 1) {
				laneType == Lane::MID;
				bluePos = { 300.0f, 12.0f, 300.0f };
				redPos = { 1883.0f,12.0f,1906.0f };
			}
			if (j == 2) {
				laneType == Lane::BOTTOM;
				bluePos = { 292.0f, 12.0f, 191.0f };
				redPos = { 2013.0f,12.0f,1911.0f };
			}
			{
				cout << "��� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_blueMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo ����
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = bluePos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::RANGED_MINION, Faction::BLUE, laneType, objectMove);

				//��Ŷ ����
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
			{
				cout << "���� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;

				//ObjectInfo ����
				ObjectInfo& objectInfo = minionRef->GetObjectInfo();
				ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
				ObjectMove::Pos pos = redPos;
				CC CCType = CC::CLEAR;
				ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
				SetObjectInfo(objectInfo, minionRef->GetObjectId(), UnitType::RANGED_MINION, Faction::RED, laneType, objectMove);


				PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
			}
		}
		Sleep(500);
	}

	cout << "�̴Ͼ� ���� ��Ŷ�� �� ����" << endl;
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