//
// Created by 邢磊 on 2019/3/26.
//

#include <string>
#include <iostream>
#include "protocol.h"
#include <fstream>
#include "random.h"

#include "util/strencoding.h"

/** Username used when cookie authentication is in use (arbitrary, only for
 * recognizability in debugging/logging purposes)
 */
static const std::string COOKIEAUTH_USER = "__cookie__";
/** Default name for auth cookie file */
static const std::string COOKIEAUTH_FILE = ".cookie";


UniValue JSONRPCReplyObj(const UniValue& result, const UniValue& error, const UniValue& id)
{
  UniValue reply(UniValue::VOBJ);
  if (!error.isNull())
    reply.pushKV("result", NullUniValue);
  else
    reply.pushKV("result", result);
  reply.pushKV("error", error);
  reply.pushKV("id", id);
  return reply;
}

std::string JSONRPCReply(const UniValue& result, const UniValue& error, const UniValue& id)
{
  UniValue reply = JSONRPCReplyObj(result, error, id);
  return reply.write() + "\n";
}

UniValue JSONRPCError(int code, const std::string& message)
{
  UniValue error(UniValue::VOBJ);
  error.pushKV("code", code);
  error.pushKV("message", message);
  return error;
}

bool GenerateAuthCookie(std::string *cookie_out) {
  const size_t COOKIE_SIZE = 32;
  unsigned char rand_pwd[COOKIE_SIZE];
  GetRandBytes(rand_pwd, COOKIE_SIZE);
  std::string cookie = COOKIEAUTH_USER + ":" + HexStr(rand_pwd, rand_pwd + COOKIE_SIZE);

  std::cout << cookie << std::endl;

  /** the umask determines what permissions are used to create this file -
   * these are set to 077 in init.cpp unless overridden with -sysperms.
   */
  std::ofstream file;
  file.open("./.cookie");
  if (!file.is_open()) {
    printf("Unable to open cookie authentication file cookie.tmp for writing\n");
    return false;
  }
  file << cookie;
  file.close();

  if (cookie_out)
    *cookie_out = cookie;
  return true;
}