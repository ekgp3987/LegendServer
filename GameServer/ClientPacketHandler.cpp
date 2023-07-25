#include "pch.h"
#include "ClientPacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"

#include "Player.h"
#include "Room.h"

#include <string>
#include <codecvt>

void ClientPacketHandler::HandlePacket(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	BufferReader br(buffer, len);

	PacketHeader header;
	br.Peek(&header);

	if (header.id == C_PICK_CHAMPION)
		int a = 0;

	switch (header.id)
	{
	case C_LOGIN:
		Handle_C_LOGIN(session, buffer, len);
		break;
	case C_PICK_FACTION:
		Handle_C_PICK_FACTION(session, buffer, len);
		break;
	case C_PICK_CHAMPION:
		Handle_C_PICK_CHAMPION(session, buffer, len);
		break;
	case C_MOVE:
		Handle_C_MOVE(session, buffer, len);
		break;
	default:
		break;
	}
}

void ClientPacketHandler::Handle_C_LOGIN(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	PKT_C_LOGIN* pkt = reinterpret_cast<PKT_C_LOGIN*>(buffer);

	if (pkt->Validate() == false)
		return;

	PKT_C_LOGIN::NickName nickNames = pkt->GetNickName();

	cout << "�г����� ���̴� " << nickNames.Count() << "�Դϴ�."<<endl;

	wstring resultNickName = L"";
	for (auto& nickname : nickNames)
	{
		resultNickName.push_back(nickname.nickname);
	}
	
	wcout << "�г����� " << resultNickName << "�Դϴ�." << endl;

	static Atomic<uint64> idGenerator = 1;

	bool success = true;
	FactionType faction = FactionType::BLUE;

	//�÷��̾� ����	
	
		PlayerRef playerRef = MakeShared<Player>();
		playerRef->SetPlayerId(idGenerator);
		playerRef->SetName(resultNickName);
		playerRef->SetOwnerSession(gameSession);
		playerRef->SetOwnerRoom(&GRoom);
		gameSession->SetPlayer(playerRef);

		//�÷��̾� �뿡 �־���
		if (GRoom.GetBluePlayerSize() > GRoom.GetRedPlayerSize()) {
			GRoom.RedEnter(playerRef, gameSession);
			faction = FactionType::RED;
			playerRef->SetPlayerFaction(faction);
		}
		else if (GRoom.GetRedPlayerSize() >= GRoom.GetBluePlayerSize()) {
			GRoom.BlueEnter(playerRef, gameSession);
			faction = FactionType::BLUE;
			playerRef->SetPlayerFaction(faction);
		}
	
	
	uint64 playerNum = GRoom.GetPlayerSize();
	if (playerNum > 4)
		success = false;
	PKT_S_LOGIN_WRITE pktWriter(success, idGenerator, faction);
	PKT_S_LOGIN_WRITE::PlayerList playerList = pktWriter.ReservePlayerList(playerNum);
	map<uint64, PlayerRef> players = GRoom.GetPlayers();
	int i = 0;

	for (auto &p : players) {
		playerList[i] = { p.second->GetPlayerId(), p.second->GetPlayerFaction()};
		wstring playerNickName = p.second->GetName();
		PKT_S_LOGIN_WRITE::NickNameList nick = pktWriter.ReserveNickNameList(&playerList[i], playerNickName.length());
		for (int j = 0; j < playerNickName.length(); j++) {
			nick[j] = { playerNickName[j] };
		}
		i++;
	}

	idGenerator++;

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_PICK_FACTION(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "HANDLE_C_PICK_FACTION�� ������" << endl;

	PKT_C_PICK_FACTION* pkt = reinterpret_cast<PKT_C_PICK_FACTION*>(buffer);

	if (pkt->Validate() == false)
		return;
	
	bool success = false;

	//���� ���߿� ==4�� �������ֱ� 4���� �� �־�߸� �÷��� �ǰԲ�
	if (GRoom.GetPlayerSize() < 5)
		success = true;

	PKT_S_PICK_FACTION_WRITE pktWriter(true, WaitingStatus::RUN);
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);

	if(success == true)
		GRoom.GameStart(sendBuffer,3000);
}

void ClientPacketHandler::Handle_C_PICK_CHAMPION(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "Handle_C_PICK_CHAMPION�� ������" << endl;

	PKT_C_PICK_CHAMPION* pkt = reinterpret_cast<PKT_C_PICK_CHAMPION*>(buffer);

	if (pkt->Validate() == false)
		return;

	gameSession->GetPlayer()->SetChampionType(pkt->champion);

	bool success = false;

	//���� ���߿� ==4�� �������ֱ� 4���� �� �־�߸� �÷��� �ǰԲ�
	if (GRoom.GetPlayerSize() < 5)
		success = true;

	PKT_S_PICK_CHAMPION_WRITE pktWriter(success, gameSession->GetPlayer()->GetPlayerId(), pkt->champion);

	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}

void ClientPacketHandler::Handle_C_MOVE(PacketSessionRef& session, BYTE* buffer, int32 len)
{
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	BufferReader br(buffer, len);

	cout << "Handle_C_MOVE�� ������" << endl;

	PKT_C_MOVE* pkt = reinterpret_cast<PKT_C_MOVE*>(buffer);

	if (pkt->Validate() == false)
		return;

	PlayerMove playerMove = pkt->playerMove;

	PKT_S_MOVE_WRITE pktWriter(gameSession->GetPlayer()->GetPlayerId(), playerMove);
	SendBufferRef sendBuffer = pktWriter.CloseAndReturn();

	GRoom.Broadcast(sendBuffer, nullptr);
}
 