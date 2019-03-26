//
// Created by 邢磊 on 2019/3/26.
//

#include "httpserver.h"
#include "httprpc.h"

static bool AppInitServers()
{
//  RPCServer::OnStarted(&OnRPCStarted);
//  RPCServer::OnStopped(&OnRPCStopped);
  if (!InitHTTPServer())
    return false;
//  StartRPC();  //会调用到OnRPCStarted()
  if (!StartHTTPRPC())
    return false;
//  if (gArgs.GetBoolArg("-rest", DEFAULT_REST_ENABLE)) StartREST();
//  StartHTTPServer();
  return true;
}

int main() {


  AppInitServers();
}