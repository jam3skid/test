#include "r3dPCH.h"
#include "r3d.h"

#include "GameCommon.h"

#include "FriendsServerLogic.h"

FriendsServerLogic gFriendsServerLogic;

FriendsServerLogic::FriendsServerLogic()
{
  InitializeCriticalSection(&csNetwork);

  isConnected_      = false;
  friendsServerId_  = 0;
  connectStartTime_ = -999;
}

FriendsServerLogic::~FriendsServerLogic()
{
  g_net.Deinitialize();
  
  DeleteCriticalSection(&csNetwork);
}

void FriendsServerLogic::OnNetPeerConnected(DWORD peerId)
{
  r3dOutToLog("connected to friends server peer%02d\n", peerId);
  
  // send validation packet immediately
  FSPKT_ValidateConnectingPeer_s n;
  n.version = FSNET_VERSION;
  n.key1    = FSNET_KEY1;
  n.key2    = FSNET_KEY2;
  net_->SendToHost(&n, sizeof(n), true);

  // set connected flag after sending validation packet.
  isConnected_ = true;
  
  return;
}

void FriendsServerLogic::OnNetPeerDisconnected(DWORD peerId)
{
  //r3dOutToLog("disconnected from friends server\n");

  isConnected_      = false;
  friendsServerId_  = 0;
  connectStartTime_ = -999;
}

void FriendsServerLogic::OnNetData(DWORD peerId, const r3dNetPacketHeader* packetData, int packetSize)
{
  switch(packetData->EventID) 
  {
    default:
      r3dError("FriendsServerLogic: invalid packetId %d", packetData->EventID);
      return;
      
    case FSPKT_S2C_FriendServerInfo:
    {
      const FSPKT_S2C_FriendServerInfo_s& n = *(FSPKT_S2C_FriendServerInfo_s*)packetData;
      
      friendsServerId_ = n.serverId;
      r3d_assert(friendsServerId_);

      break;
    }
      
    case FSPKT_ValidateConnectingPeer:
    {
      const FSPKT_ValidateConnectingPeer_s& n = *(FSPKT_ValidateConnectingPeer_s*)packetData;

      //check version
      if(n.version != FSNET_VERSION)
      {
        r3dOutToLog("Wrong friends server version. Please get update\n");
        net_->DisconnectPeer(peerId);
        OnNetPeerDisconnected(peerId);
      }

      break;
    }

	case FSPKT_S2C_InvitePeerToPartyAns:
	{
	  r3dOutToLog("WE RECEIVED IT!\n");
      break;
    }
  }

  return;
}

void FriendsServerLogic::Tick()
{
  R3DPROFILE_FUNCTION("FriendsServerLogic::Tick");
  EnterCriticalSection(&csNetwork);
  if(net_) {
    net_->Update();
  }
  LeaveCriticalSection(&csNetwork);

  return;
}

int FriendsServerLogic::ConnectAsync(const char* host, int port)
{
  // hide inside critical section, because it can be called from other thread in FrontEndNew
  EnterCriticalSection(&csNetwork);

  versionChecked_ = false;
  
  g_net.Initialize(this, "friendsNet");
  g_net.CreateClient(0);
  int res = g_net.Connect(host, port);
  
  LeaveCriticalSection(&csNetwork);

  return res;
}

void FriendsServerLogic::Disconnect()
{
  // hide inside critical section, because it can be called from other thread in FrontEndNew
  r3dCSHolder cs1(csNetwork);

  g_net.Deinitialize();
  isConnected_ = false;
}

void FriendsServerLogic::SendPartyInviteToPeer(int CustomerID)
{
  FSPKT_S2C_InvitePeerToParty_s n;
  n.CustomerID = CustomerID;
  net_->SendToHost(&n, sizeof(n));
}
