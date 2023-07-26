#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include <Windows.h>
#include "ClientPacketHandler.h"
#include "Minion.h"
#include <iostream>
#include <sstream>

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
	vector<PlayerMove> playerMove;
	{
		PlayerMove mPlayerMove;
		mPlayerMove.state = PlayerMove::PlayerState::IDLE;
		mPlayerMove.moveDir = PlayerMove::MoveDir{ 0.0f,0.0f,0.0f };
		mPlayerMove.pos = PlayerMove::Pos{ 0.0f,0.0f,0.0f };

		mPlayerMove.pos = PlayerMove::Pos{ 10.0f,0.0f,10.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = PlayerMove::Pos{ 30.0f,0.0f,10.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = PlayerMove::Pos{ 10.0f,0.0f,100.0f };
		playerMove.push_back(mPlayerMove);
		mPlayerMove.pos = PlayerMove::Pos{ 30.0f,0.0f,10.0f };
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

	//Sleep(3000);

	GameStartSpawn(_sendBuffer, 3000, success);
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

		for (int i = 0; i < 6; i++) {
			if ((i % 2) == 0) {
				cout << "��� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_blueMinions[minionRef->GetObjectId()] = minionRef;
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(minionRef->GetObjectId(), ObjectType::MELEEMINION, FactionType::BLUE);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer,nullptr);
				sendBuffer.reset();
			}
			else {
				cout << "���� �̴Ͼ� ����" << endl;
				MinionRef minionRef = MakeShared<Minion>();
				_redMinions[minionRef->GetObjectId()] = minionRef;
				PKT_S_SPAWN_OBJECT_WRITE pktWriter(minionRef->GetObjectId(), ObjectType::MELEEMINION, FactionType::RED);
				SendBufferRef sendBuffer = pktWriter.CloseAndReturn();
				Broadcast(sendBuffer, nullptr);
				sendBuffer.reset(); 
				Sleep(500);
			}			
		}

		cout << "�̴Ͼ� ���� ��Ŷ�� �� ����" << endl;
	});

	t1.detach();
}
