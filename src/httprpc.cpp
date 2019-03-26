//
// Created by 邢磊 on 2019/3/26.
//

#include <string>
#include "rpc/protocol.h"
#include "rpc/server.h"
#include "httpserver.h"
#include <util/memory.h>

/** WWW-Authenticate to present with 401 Unauthorized response */
static const char *WWW_AUTH_HEADER_DATA = "Basic realm=\"jsonrpc\"";

/** Simple one-shot callback timer to be used by the RPC mechanism to e.g.
 * re-lock the wallet.
 */
class HTTPRPCTimer : public RPCTimerBase
{
public:
  HTTPRPCTimer(struct event_base* eventBase, std::function<void()>& func, int64_t millis) :
    ev(eventBase, false, func)
  {
    struct timeval tv;
    tv.tv_sec = millis/1000;
    tv.tv_usec = (millis%1000)*1000;
    ev.trigger(&tv);
  }
private:
  HTTPEvent ev;
};

class HTTPRPCTimerInterface : public RPCTimerInterface
{
public:
  explicit HTTPRPCTimerInterface(struct event_base* _base) : base(_base)
  {
  }
  const char* Name() override
  {
    return "HTTP";
  }
  RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) override
  {
    return new HTTPRPCTimer(base, func, millis);
  }
private:
  struct event_base* base;
};


static void JSONErrorReply(HTTPRequest *req, const UniValue &objError, const UniValue &id) {
  // Send error reply from json-rpc error object
  int nStatus = HTTP_INTERNAL_SERVER_ERROR;
  int code = find_value(objError, "code").get_int();

  if (code == RPC_INVALID_REQUEST)
    nStatus = HTTP_BAD_REQUEST;
  else if (code == RPC_METHOD_NOT_FOUND)
    nStatus = HTTP_NOT_FOUND;

  std::string strReply = JSONRPCReply(NullUniValue, objError, id);

  req->WriteHeader("Content-Type", "application/json");
  req->WriteReply(nStatus, strReply);
}


static bool HTTPReq_JSONRPC(HTTPRequest *req, const std::string &) {
  // JSONRPC handles only POST
  if (req->GetRequestMethod() != HTTPRequest::POST) {
    req->WriteReply(HTTP_BAD_METHOD, "JSONRPC server handles only POST requests");
    return false;
  }
  // Check authorization
  std::pair<bool, std::string> authHeader = req->GetHeader("authorization");
  if (!authHeader.first) {
    req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
    req->WriteReply(HTTP_UNAUTHORIZED);
    return false;
  }

  JSONRPCRequest jreq;
//  jreq.peerAddr = req->GetPeer().ToString();
//  if (!RPCAuthorized(authHeader.second, jreq.authUser)) {
//    LogPrintf("ThreadRPCServer incorrect password attempt from %s\n", jreq.peerAddr);
//
//    /* Deter brute-forcing
//       If this results in a DoS the user really
//       shouldn't have their RPC port exposed. */
//    MilliSleep(250);
//
//    req->WriteHeader("WWW-Authenticate", WWW_AUTH_HEADER_DATA);
//    req->WriteReply(HTTP_UNAUTHORIZED);
//    return false;
//  }

  try {
    // Parse request
    UniValue valRequest;
    if (!valRequest.read(req->ReadBody()))
      throw JSONRPCError(RPC_PARSE_ERROR, "Parse error");

    // Set the URI
    jreq.URI = req->GetURI();

    std::string strReply;
    // singleton request
    if (valRequest.isObject()) {
      jreq.parse(valRequest);

      UniValue result = tableRPC.execute(jreq);

      // Send reply
      strReply = JSONRPCReply(result, NullUniValue, jreq.id);

      // array of requests
    } else if (valRequest.isArray())
      strReply = JSONRPCExecBatch(jreq, valRequest.get_array());
    else
      throw JSONRPCError(RPC_PARSE_ERROR, "Top-level object parse error");

    req->WriteHeader("Content-Type", "application/json");
    req->WriteReply(HTTP_OK, strReply);
  } catch (const UniValue &objError) {
    JSONErrorReply(req, objError, jreq.id);
    return false;
  } catch (const std::exception &e) {
    JSONErrorReply(req, JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
    return false;
  }
  return true;
}


/* Pre-base64-encoded authentication token */
static std::string strRPCUserColonPass;

/* Stored RPC timer interface (for unregistration) */
static std::unique_ptr<HTTPRPCTimerInterface> httpRPCTimerInterface;


static bool InitRPCAuthentication() {

  printf("No rpcpassword set - using random cookie authentication.\n");
  if (!GenerateAuthCookie(&strRPCUserColonPass)) {
    return false;
  }


//  if (gArgs.GetArg("-rpcauth","") != "")
//  {
//    LogPrintf("Using rpcauth authentication.\n");
//  }
  return true;
}


bool StartHTTPRPC() {

  printf("Starting HTTP RPC server\n");
  if (!InitRPCAuthentication())
    return false;

  RegisterHTTPHandler("/", true, HTTPReq_JSONRPC);
//  if (g_wallet_init_interface.HasWalletSupport()) {
//    RegisterHTTPHandler("/wallet/", false, HTTPReq_JSONRPC);
//  }
  struct event_base *eventBase = EventBase();
  assert(eventBase);
  httpRPCTimerInterface = MakeUnique<HTTPRPCTimerInterface>(eventBase);
  RPCSetTimerInterface(httpRPCTimerInterface.get());  //TODO 这个是干嘛用的？？？
  return true;
}