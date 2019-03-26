//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_SERVER_H
#define BITCOINTEST_SERVER_H

#include "univalue/univalue.h"

#include <vector>

/** Opaque base class for timers returned by NewTimerFunc.
 * This provides no methods at the moment, but makes sure that delete
 * cleans up the whole state.
 */
class RPCTimerBase
{
public:
  virtual ~RPCTimerBase() {}
};


/**
 * RPC timer "driver".
 */
class RPCTimerInterface
{
public:
  virtual ~RPCTimerInterface() {}
  /** Implementation name */
  virtual const char *Name() = 0;
  /** Factory function for timers.
   * RPC will call the function to create a timer that will call func in *millis* milliseconds.
   * @note As the RPC mechanism is backend-neutral, it can use different implementations of timers.
   * This is needed to cope with the case in which there is no HTTP server, but
   * only GUI RPC console, and to break the dependency of pcserver on httprpc.
   */
  virtual RPCTimerBase* NewTimer(std::function<void()>& func, int64_t millis) = 0;
};

/** Set the factory function for timers */
void RPCSetTimerInterface(RPCTimerInterface *iface);



class JSONRPCRequest {
public:
  UniValue id;
  std::string strMethod;
  UniValue params;
  bool fHelp;
  std::string URI;
  std::string authUser;
  std::string peerAddr;

  JSONRPCRequest() : id(NullUniValue), params(NullUniValue), fHelp(false) {}

  void parse(const UniValue &valRequest);
};

/** Query whether RPC is running */
bool IsRPCRunning();

typedef UniValue(*rpcfn_type)(const JSONRPCRequest &jsonRequest);

class CRPCCommand {
public:
  std::string category;
  std::string name;
  rpcfn_type actor;
  std::vector<std::string> argNames;
};

/**
 * Bitcoin RPC command dispatcher.
 */
class CRPCTable {
private:
  std::map<std::string, const CRPCCommand *> mapCommands;
public:
  CRPCTable();

  const CRPCCommand *operator[](const std::string &name) const;

  std::string help(const std::string &name, const JSONRPCRequest &helpreq) const;

  /**
   * Execute a method.
   * @param request The JSONRPCRequest to execute
   * @returns Result of the call.
   * @throws an exception (UniValue) when an error happens.
   */
  UniValue execute(const JSONRPCRequest &request) const;

  /**
  * Returns a list of registered commands
  * @returns List of registered commands.
  */
  std::vector<std::string> listCommands() const;


  /**
   * Appends a CRPCCommand to the dispatch table.
   *
   * Returns false if RPC server is already running (dump concurrency protection).
   *
   * Commands cannot be overwritten (returns false).
   *
   * Commands with different method names but the same callback function will
   * be considered aliases, and only the first registered method name will
   * show up in the help text command listing. Aliased commands do not have
   * to have the same behavior. Server and client code can distinguish
   * between calls based on method name, and aliased commands can also
   * register different names, types, and numbers of parameters.
   */
  bool appendCommand(const std::string &name, const CRPCCommand *pcmd);
};

extern CRPCTable tableRPC;




std::string JSONRPCExecBatch(const JSONRPCRequest& jreq, const UniValue& vReq);


#endif //BITCOINTEST_SERVER_H
