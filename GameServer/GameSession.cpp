#include "pch.h"
#include "GameSession.h"
#include "GameSessionManager.h"
#include "ClientPacketHandler.h"
#include "Room.h"

void GameSession::OnConnected()
{
	GSessionManager.Add(static_pointer_cast<GameSession>(shared_from_this()));
	cout << "클라이언트와 연결 됨" << endl;
}

void GameSession::OnDisconnected()
{
	PacketSessionRef session = GetPacketSessionRef();
	GameSessionRef gameSession = static_pointer_cast<GameSession>(session);

	GSessionManager.Remove(static_pointer_cast<GameSession>(shared_from_this()));
	GRoom.Leave(gameSession->GetPlayer(), gameSession);
}

void GameSession::OnRecvPacket(BYTE* buffer, int32 len)
{
	PacketSessionRef session = GetPacketSessionRef();
	PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);


	ClientPacketHandler::HandlePacket(session, buffer, len);
}

void GameSession::OnSend(int32 len)
{
}