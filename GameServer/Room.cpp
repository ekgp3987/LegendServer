#include "pch.h"
#include "Room.h"
#include "Player.h"
#include "GameSession.h"
#include <Windows.h>
#include "ClientPacketHandler.h"

Room GRoom;

void Room::BlueEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetPlayerId();
	WRITE_LOCK;
		_bluePlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//패킷을 생성해서 들어왔다는 것을 모두에게 보여줘야 함.
}

void Room::RedEnter(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetPlayerId();
	WRITE_LOCK;
		_redPlayers[currentPlayerId] = player;
		_allPlayers[currentPlayerId] = player;
		//패킷을 생성해서 들어왔다는 것을 모두에게 보여줘야 함.
}

void Room::Leave(PlayerRef player, GameSessionRef session)
{
	uint64 currentPlayerId = session->GetPlayer()->GetPlayerId();
	WRITE_LOCK;
		_bluePlayers.erase(currentPlayerId);
		_redPlayers.erase(currentPlayerId);
		_allPlayers.erase(currentPlayerId);

		//패킷을 생성해서 나갔다는 것을 모두에게 보여줘야 함.
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

void Room::GameStart(SendBufferRef sendBuffer, uint64 milliSeconds)
{
	WRITE_LOCK;
	bool success = true;

	Sleep(milliSeconds);

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

	//플레이어 정보를 패킷에 넣음
	int i = 0;
	for (auto& p : players) {
		playerInfoList[i].champion = p.second->GetChampionType();
		playerInfoList[i].faction = p.second->GetPlayerFaction();
		playerInfoList[i].id = p.second->GetPlayerId();
		playerInfoList[i].posInfo = playerMove[i]; 

		//플레이어 문자열을 안에 넣음
		int64 nickNameSize = p.second->GetName().size();
		wstring nickName = p.second->GetName();
		PKT_S_GAME_START_WRITE::NickNameList nick = pktWriter.ReserveNickNameList(&playerInfoList[i], nickNameSize);
		for (int j = 0; j < nickNameSize; j++) {
			nick[j] = {nickName[j]};
		}
		i++;
	}

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	Broadcast(sendBuffer, nullptr);
}
