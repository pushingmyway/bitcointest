//
// Created by 邢磊 on 2019/3/26.
//

#include "server.h"

#include "protocol.h"
#include <unordered_map>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

static std::atomic<bool> g_rpc_running{false};

/* Timer-creating functions */
static RPCTimerInterface* timerInterface = nullptr;

void RPCSetTimerInterface(RPCTimerInterface *iface)
{
  timerInterface = iface;
}


/**
 * Process named arguments into a vector of positional arguments, based on the
 * passed-in specification for the RPC call's arguments.
 */
static inline JSONRPCRequest transformNamedArguments(const JSONRPCRequest& in, const std::vector<std::string>& argNames)
{
  JSONRPCRequest out = in;
  out.params = UniValue(UniValue::VARR);
  // Build a map of parameters, and remove ones that have been processed, so that we can throw a focused error if
  // there is an unknown one.
  const std::vector<std::string>& keys = in.params.getKeys();
  const std::vector<UniValue>& values = in.params.getValues();
  std::unordered_map<std::string, const UniValue*> argsIn;
  for (size_t i=0; i<keys.size(); ++i) {
    argsIn[keys[i]] = &values[i];
  }
  // Process expected parameters.
  int hole = 0;
  for (const std::string &argNamePattern: argNames) {
    std::vector<std::string> vargNames;
    boost::algorithm::split(vargNames, argNamePattern, boost::algorithm::is_any_of("|"));
    auto fr = argsIn.end();
    for (const std::string & argName : vargNames) {
      fr = argsIn.find(argName);
      if (fr != argsIn.end()) {
        break;
      }
    }
    if (fr != argsIn.end()) {
      for (int i = 0; i < hole; ++i) {
        // Fill hole between specified parameters with JSON nulls,
        // but not at the end (for backwards compatibility with calls
        // that act based on number of specified parameters).
        out.params.push_back(UniValue());
      }
      hole = 0;
      out.params.push_back(*fr->second);
      argsIn.erase(fr);
    } else {
      hole += 1;
    }
  }
  // If there are still arguments in the argsIn map, this is an error.
  if (!argsIn.empty()) {
    throw JSONRPCError(RPC_INVALID_PARAMETER, "Unknown named parameter " + argsIn.begin()->first);
  }
  // Return request with named arguments transformed to positional arguments
  return out;
}






static UniValue getrpcinfo(const JSONRPCRequest& request)
{
//  if (request.fHelp || request.params.size() > 0) {
//    throw std::runtime_error(
//      RPCHelpMan{"getrpcinfo",
//                 "\nReturns details of the RPC server.\n",
//                 {},
//                 RPCResults{},
//                 RPCExamples{""},
//      }.ToString()
//    );
//  }
//
//  LOCK(g_rpc_server_info.mutex);
  UniValue active_commands(UniValue::VARR);
//  for (const RPCCommandExecutionInfo& info : g_rpc_server_info.active_commands) {
//    UniValue entry(UniValue::VOBJ);
//    entry.pushKV("method", info.method);
//    entry.pushKV("duration", GetTimeMicros() - info.start);
//    active_commands.push_back(entry);
//  }

  UniValue result(UniValue::VOBJ);
  result.pushKV("active_commands", active_commands);

  return result;
}

UniValue help(const JSONRPCRequest& jsonRequest)
{
//  if (jsonRequest.fHelp || jsonRequest.params.size() > 1)
//    throw std::runtime_error(
//      RPCHelpMan{"help",
//                 "\nList all commands, or get help for a specified command.\n",
//                 {
//                   {"command", RPCArg::Type::STR, /* default */ "all commands", "The command to get help on"},
//                 },
//                 RPCResult{
//                   "\"text\"     (string) The help text\n"
//                 },
//                 RPCExamples{""},
//      }.ToString()
//    );

  std::string strCommand;
  if (jsonRequest.params.size() > 0)
    strCommand = jsonRequest.params[0].get_str();

  return tableRPC.help(strCommand, jsonRequest);
}

UniValue stop(const JSONRPCRequest& jsonRequest)
{
  // Accept the deprecated and ignored 'detach' boolean argument
  // Also accept the hidden 'wait' integer argument (milliseconds)
  // For instance, 'stop 1000' makes the call wait 1 second before returning
  // to the client (intended for testing)
//  if (jsonRequest.fHelp || jsonRequest.params.size() > 1)
//    throw std::runtime_error(
//      RPCHelpMan{"stop",
//                 "\nStop Bitcoin server.",
//                 {},
//                 RPCResults{},
//                 RPCExamples{""},
//      }.ToString());
//  // Event loop will exit after current HTTP requests have been handled, so
//  // this reply will get back to the client.
//  StartShutdown();
//  if (jsonRequest.params[0].isNum()) {
//    MilliSleep(jsonRequest.params[0].get_int());
//  }
  return "Bitcoin server stopping";
}

static UniValue uptime(const JSONRPCRequest& jsonRequest)
{
//  if (jsonRequest.fHelp || jsonRequest.params.size() > 0)
//    throw std::runtime_error(
//      RPCHelpMan{"uptime",
//                 "\nReturns the total uptime of the server.\n",
//                 {},
//                 RPCResult{
//                   "ttt        (numeric) The number of seconds that the server has been running\n"
//                 },
//                 RPCExamples{
//                   HelpExampleCli("uptime", "")
//                   + HelpExampleRpc("uptime", "")
//                 },
//      }.ToString());

//  return GetTime() - GetStartupTime();
  return 0;
}

// clang-format off
static const CRPCCommand vRPCCommands[] =
  { //  category              name                      actor (function)         argNames
    //  --------------------- ------------------------  -----------------------  ----------
    /* Overall control/query calls */
    { "control",            "getrpcinfo",             &getrpcinfo,             {}  },
    { "control",            "help",                   &help,                   {"command"}  },
    { "control",            "stop",                   &stop,                   {"wait"}  },
    { "control",            "uptime",                 &uptime,                 {}  },
  };
// clang-format on


CRPCTable::CRPCTable()
{
  unsigned int vcidx;
  for (vcidx = 0; vcidx < (sizeof(vRPCCommands) / sizeof(vRPCCommands[0])); vcidx++)
  {
    const CRPCCommand *pcmd;

    pcmd = &vRPCCommands[vcidx];
    mapCommands[pcmd->name] = pcmd;
  }
}

const CRPCCommand *CRPCTable::operator[](const std::string &name) const
{
  std::map<std::string, const CRPCCommand*>::const_iterator it = mapCommands.find(name);
  if (it == mapCommands.end())
    return nullptr;
  return (*it).second;
}

bool CRPCTable::appendCommand(const std::string& name, const CRPCCommand* pcmd) {
  if (IsRPCRunning())
    return false;

  // don't allow overwriting for now
  std::map<std::string, const CRPCCommand *>::const_iterator it = mapCommands.find(name);
  if (it != mapCommands.end())
    return false;

  mapCommands[name] = pcmd;
  return true;
}

UniValue CRPCTable::execute(const JSONRPCRequest &request) const
{
  // Return immediately if in warmup
//  {
//    LOCK(cs_rpcWarmup);
//    if (fRPCInWarmup)
//      throw JSONRPCError(RPC_IN_WARMUP, rpcWarmupStatus);
//  }

  // Find method
  const CRPCCommand *pcmd = tableRPC[request.strMethod];
  if (!pcmd)
    throw JSONRPCError(RPC_METHOD_NOT_FOUND, "Method not found");

  try
  {
//    RPCCommandExecution execution(request.strMethod);
    // Execute, convert arguments to array if necessary
    if (request.params.isObject()) {
      return pcmd->actor(transformNamedArguments(request, pcmd->argNames));
    } else {
      return pcmd->actor(request);
    }
  }
  catch (const std::exception& e)
  {
    throw JSONRPCError(RPC_MISC_ERROR, e.what());
  }
}

std::vector<std::string> CRPCTable::listCommands() const
{
  std::vector<std::string> commandList;
  for (const auto& i : mapCommands) commandList.emplace_back(i.first);
  return commandList;
}

std::string CRPCTable::help(const std::string& strCommand, const JSONRPCRequest& helpreq) const
{
  std::string strRet;
//  std::string category;
//  std::set<rpcfn_type> setDone;
//  std::vector<std::pair<std::string, const CRPCCommand*> > vCommands;
//
//  for (const auto& entry : mapCommands)
//    vCommands.push_back(make_pair(entry.second->category + entry.first, entry.second));
//  sort(vCommands.begin(), vCommands.end());
//
//  JSONRPCRequest jreq(helpreq);
//  jreq.fHelp = true;
//  jreq.params = UniValue();
//
//  for (const std::pair<std::string, const CRPCCommand*>& command : vCommands)
//  {
//    const CRPCCommand *pcmd = command.second;
//    std::string strMethod = pcmd->name;
//    if ((strCommand != "" || pcmd->category == "hidden") && strMethod != strCommand)
//      continue;
//    jreq.strMethod = strMethod;
//    try
//    {
//      rpcfn_type pfn = pcmd->actor;
//      if (setDone.insert(pfn).second)
//        (*pfn)(jreq);
//    }
//    catch (const std::exception& e)
//    {
//      // Help text is returned in an exception
//      std::string strHelp = std::string(e.what());
//      if (strCommand == "")
//      {
//        if (strHelp.find('\n') != std::string::npos)
//          strHelp = strHelp.substr(0, strHelp.find('\n'));
//
//        if (category != pcmd->category)
//        {
//          if (!category.empty())
//            strRet += "\n";
//          category = pcmd->category;
//          strRet += "== " + Capitalize(category) + " ==\n";
//        }
//      }
//      strRet += strHelp + "\n";
//    }
//  }
//  if (strRet == "")
//    strRet = strprintf("help: unknown command: %s\n", strCommand);
//  strRet = strRet.substr(0,strRet.size()-1);
  return strRet;
}

CRPCTable tableRPC;


bool IsRPCRunning()
{
  return g_rpc_running;
}



static UniValue JSONRPCExecOne(JSONRPCRequest jreq, const UniValue& req)
{
  UniValue rpc_result(UniValue::VOBJ);

  try {
    jreq.parse(req);

    UniValue result = tableRPC.execute(jreq);
    rpc_result = JSONRPCReplyObj(result, NullUniValue, jreq.id);
  }
  catch (const UniValue& objError)
  {
    rpc_result = JSONRPCReplyObj(NullUniValue, objError, jreq.id);
  }
  catch (const std::exception& e)
  {
    rpc_result = JSONRPCReplyObj(NullUniValue,
                                 JSONRPCError(RPC_PARSE_ERROR, e.what()), jreq.id);
  }

  return rpc_result;
}

std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const UniValue& vReq)
{
  UniValue ret(UniValue::VARR);
  for (unsigned int reqIdx = 0; reqIdx < vReq.size(); reqIdx++)
    ret.push_back(JSONRPCExecOne(jreq, vReq[reqIdx]));

  return ret.write() + "\n";
}


void JSONRPCRequest::parse(const UniValue& valRequest)
{
  // Parse request
  if (!valRequest.isObject())
    throw JSONRPCError(RPC_INVALID_REQUEST, "Invalid Request object");
  const UniValue& request = valRequest.get_obj();

  // Parse id now so errors from here on will have the id
  id = find_value(request, "id");

  // Parse method
  UniValue valMethod = find_value(request, "method");
  if (valMethod.isNull())
    throw JSONRPCError(RPC_INVALID_REQUEST, "Missing method");
  if (!valMethod.isStr())
    throw JSONRPCError(RPC_INVALID_REQUEST, "Method must be a string");
  strMethod = valMethod.get_str();
//  if (fLogIPs)
//    LogPrint(BCLog::RPC, "ThreadRPCServer method=%s user=%s peeraddr=%s\n", SanitizeString(strMethod),
//             this->authUser, this->peerAddr);
//  else
//    LogPrint(BCLog::RPC, "ThreadRPCServer method=%s user=%s\n", SanitizeString(strMethod), this->authUser);

  // Parse params
  UniValue valParams = find_value(request, "params");
  if (valParams.isArray() || valParams.isObject())
    params = valParams;
  else if (valParams.isNull())
    params = UniValue(UniValue::VARR);
  else
    throw JSONRPCError(RPC_INVALID_REQUEST, "Params must be an array or object");
}