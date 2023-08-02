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
	thread t1([this, _sendBuffer, milliSeconds, _success]() {
		int a = 0;

		if (_success == false) {
			cout << "GameStart�����ؼ� Spawn��Ŷ �������� ����";
			return;
		}

		Sleep(milliSeconds);		

		//�� ���ο� �� 3������ ���� �̴Ͼ� ����
		for (int i = 0; i < 3; i++) {
			//3���δ� �̴Ͼ� ����
			for (int j = 0; j < 3; j++) {
				Lane laneType = Lane::END;
				if (j == 0)
					laneType = Lane::TOP;
				if (j == 1)
					laneType == Lane::MID;
				if (j == 2)
					laneType == Lane::BOTTOM;
				{
					cout << "��� �̴Ͼ� ����" << endl;
					MinionRef minionRef = MakeShared<Minion>();
					_blueMinions[minionRef->GetObjectId()] = minionRef;

					//ObjectInfo ����
					ObjectInfo& objectInfo = minionRef->GetObjectInfo();
					ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
					ObjectMove::Pos pos = { 10.f,10.f,10.f };
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
					ObjectMove::Pos pos = { 10.f,10.f,10.f };
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

		////�� ���ο� �� 3������ ���Ÿ� �̴Ͼ� ����
		//for (int i = 0; i < 3; i++) {
		//	//3���δ� �̴Ͼ� ���� //j�� 0�̸� ž 1�̸� �̵� 2�� ����
		//	for (int j = 0; j < 3; j++) {
		//		LaneType laneType;
		//		if (j == 0)
		//			laneType = LaneType::TOP;
		//		if (j == 1)
		//			laneType == LaneType::MID;
		//		if (j == 2)
		//			laneType == LaneType::BOTTOM;
		//		{
		//			cout << "��� �̴Ͼ� ����" << endl;
		//			MinionRef minionRef = MakeShared<Minion>();
		//			_blueMinions[minionRef->GetObjectId()] = minionRef;

		//			//ObjectInfo ����
		//			ObjectInfo& objectInfo = minionRef->GetObjectInfo();
		//			ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
		//			ObjectMove::Pos pos = { 10.f,10.f,10.f };
		//			CC_TYPE CCType = CC_TYPE::NONE;
		//			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
		//			SetObjectInfo(objectInfo, minionRef->GetObjectId(), ObjectType::CASTER_MINION, FactionType::BLUE, laneType, objectMove);

		//			//��Ŷ ����
		//			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		//			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		//			Broadcast(sendBuffer, nullptr);
		//		}
		//		{
		//			cout << "���� �̴Ͼ� ����" << endl;
		//			MinionRef minionRef = MakeShared<Minion>();
		//			_redMinions[minionRef->GetObjectId()] = minionRef;

		//			//ObjectInfo ����
		//			ObjectInfo& objectInfo = minionRef->GetObjectInfo();
		//			ObjectMove::MoveDir moveDir = { 10.f,10.f,10.f };
		//			ObjectMove::Pos pos = { 10.f,10.f,10.f };
		//			CC_TYPE CCType = CC_TYPE::NONE;
		//			ObjectMove objectMove(1, 100.f, 100.f, 10.f, 20.f, moveDir, pos, CCType);
		//			SetObjectInfo(objectInfo, minionRef->GetObjectId(), ObjectType::CASTER_MINION, FactionType::RED, laneType, objectMove);


		//			PKT_S_SPAWN_OBJECT_WRITE pktWriter(objectInfo);
		//			SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
		//			Broadcast(sendBuffer, nullptr);
		//		}
		//	}
		//	Sleep(500);
		//}

		cout << "�̴Ͼ� ���� ��Ŷ�� �� ����" << endl;
	});

	t1.detach();
}

void Room::TimeThread()
{
	for (int seconds = 0; seconds <= 3600; seconds++) {
		second = seconds;
		cout << "���� ���� �� " << seconds << "�ʰ� �������ϴ�." << endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

void Room::RemoveObject(uint64 _objectID)
{
	WRITE_LOCK;
	_bluePlayers.erase(_objectID);
	_redPlayers.erase(_objectID);
	_allPlayers.erase(_objectID);
	_blueMinions.erase(_objectID);
	_redMinions.erase(_objectID);
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
