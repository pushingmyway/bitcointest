//
// Created by 邢磊 on 2019/3/26.
//
#include "httpserver.h"
#include "support/events.h"

#include "event2/http.h"
#include "event2/buffer.h"
#include "event2/event.h"
#include "event2/bufferevent.h"

#include <assert.h>

static const unsigned int MAX_SIZE = 0x02000000;

//! Bound listening sockets
std::vector<evhttp_bound_socket *> boundSockets;

//! libevent event loop
static struct event_base *eventBase = nullptr;

//! HTTP server
struct evhttp *eventHTTP = nullptr;

struct event_base *EventBase() {
  return eventBase;
}

struct HTTPPathHandler {
  HTTPPathHandler(std::string _prefix, bool _exactMatch, HTTPRequestHandler _handler) :
    prefix(_prefix), exactMatch(_exactMatch), handler(_handler) {
  }

  std::string prefix;
  bool exactMatch;
  HTTPRequestHandler handler;
};


//! Handlers for (sub)paths
std::vector<HTTPPathHandler> pathHandlers;

void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler) {
  printf("Registering HTTP handler for %s (exactmatch %d)\n", prefix.c_str(), exactMatch);
  pathHandlers.push_back(HTTPPathHandler(prefix, exactMatch, handler));
}

static void httpevent_callback_fn(evutil_socket_t, short, void *data) {
  // Static handler: simply call inner handler
  HTTPEvent *self = static_cast<HTTPEvent *>(data);
  self->handler();
  if (self->deleteWhenTriggered)
    delete self;
}

HTTPEvent::HTTPEvent(struct event_base *base, bool _deleteWhenTriggered, const std::function<void()> &_handler) :
  deleteWhenTriggered(_deleteWhenTriggered), handler(_handler) {
  ev = event_new(base, -1, 0, httpevent_callback_fn, this);
  assert(ev);
}

HTTPEvent::~HTTPEvent() {
  event_free(ev);
}

void HTTPEvent::trigger(struct timeval *tv) {
  if (tv == nullptr)
    event_active(ev, 0, 0); // immediately trigger event in main thread
  else
    evtimer_add(ev, tv); // trigger after timeval passed
}


HTTPRequest::HTTPRequest(struct evhttp_request *_req) : req(_req),
                                                        replySent(false) {
}

HTTPRequest::~HTTPRequest() {
  if (!replySent) {
    // Keep track of whether reply was sent to avoid request leaks
    printf("%s: Unhandled request\n", __func__);
    WriteReply(HTTP_INTERNAL, "Unhandled request");
  }
  // evhttpd cleans up the request, as long as a reply was sent.
}

std::pair<bool, std::string> HTTPRequest::GetHeader(const std::string &hdr) const {
  const struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
  assert(headers);
  const char *val = evhttp_find_header(headers, hdr.c_str());
  if (val)
    return std::make_pair(true, val);
  else
    return std::make_pair(false, "");
}

std::string HTTPRequest::ReadBody() {
  struct evbuffer *buf = evhttp_request_get_input_buffer(req);
  if (!buf)
    return "";
  size_t size = evbuffer_get_length(buf);
  /** Trivial implementation: if this is ever a performance bottleneck,
   * internal copying can be avoided in multi-segment buffers by using
   * evbuffer_peek and an awkward loop. Though in that case, it'd be even
   * better to not copy into an intermediate string but use a stream
   * abstraction to consume the evbuffer on the fly in the parsing algorithm.
   */
  const char *data = (const char *) evbuffer_pullup(buf, size);
  if (!data) // returns nullptr in case of empty buffer
    return "";
  std::string rv(data, size);
  evbuffer_drain(buf, size);
  return rv;
}

void HTTPRequest::WriteHeader(const std::string &hdr, const std::string &value) {
  struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
  assert(headers);
  evhttp_add_header(headers, hdr.c_str(), value.c_str());
}

/** Closure sent to main thread to request a reply to be sent to
 * a HTTP request.
 * Replies must be sent in the main loop in the main http thread,
 * this cannot be done from worker threads.
 */
void HTTPRequest::WriteReply(int nStatus, const std::string &strReply) {
  assert(!replySent && req);
//  if (ShutdownRequested()) {
//    WriteHeader("Connection", "close");
//  }
  // Send event to main http thread to send reply message
  struct evbuffer *evb = evhttp_request_get_output_buffer(req);
  assert(evb);
  evbuffer_add(evb, strReply.data(), strReply.size());
  auto req_copy = req;
  HTTPEvent *ev = new HTTPEvent(eventBase, true, [req_copy, nStatus] {
      evhttp_send_reply(req_copy, nStatus, nullptr, nullptr);
      // Re-enable reading from the socket. This is the second part of the libevent
      // workaround above.
      if (event_get_version_number() >= 0x02010600 && event_get_version_number() < 0x02020001) {
        evhttp_connection *conn = evhttp_request_get_connection(req_copy);
        if (conn) {
          bufferevent *bev = evhttp_connection_get_bufferevent(conn);
          if (bev) {
            bufferevent_enable(bev, EV_READ | EV_WRITE);
          }
        }
      }
  });
  ev->trigger(nullptr);
  replySent = true;
  req = nullptr; // transferred back to main thread
}

//CService HTTPRequest::GetPeer() const {
//  evhttp_connection *con = evhttp_request_get_connection(req);
//  CService peer;
//  if (con) {
//    // evhttp retains ownership over returned address string
//    const char *address = "";
//    uint16_t port = 0;
//    evhttp_connection_get_peer(con, (char **) &address, &port);
//    peer = LookupNumeric(address, port);
//  }
//  return peer;
//}

std::string HTTPRequest::GetURI() const {
  return evhttp_request_get_uri(req);
}

HTTPRequest::RequestMethod HTTPRequest::GetRequestMethod() const {
  switch (evhttp_request_get_command(req)) {
    case EVHTTP_REQ_GET:
      return GET;
      break;
    case EVHTTP_REQ_POST:
      return POST;
      break;
    case EVHTTP_REQ_HEAD:
      return HEAD;
      break;
    case EVHTTP_REQ_PUT:
      return PUT;
      break;
    default:
      return UNKNOWN;
      break;
  }
}

/** Bind HTTP server to specified addresses */
static bool HTTPBindAddresses(struct evhttp *http) {
//  int http_port = gArgs.GetArg("-rpcport", BaseParams().RPCPort());  //main chain  default port 8332

  //rpc 端口8332
  int http_port = 8332;
  std::vector<std::pair<std::string, uint16_t> > endpoints;

  // Determine what addresses to bind to
//  if (!(gArgs.IsArgSet("-rpcallowip") &&
//        gArgs.IsArgSet("-rpcbind"))) { // Default to loopback if not allowing external IPs
  endpoints.push_back(std::make_pair("::1", http_port));
  endpoints.push_back(std::make_pair("127.0.0.1", http_port));
//    if (gArgs.IsArgSet("-rpcallowip")) {
//      LogPrintf("WARNING: option -rpcallowip was specified without -rpcbind; this doesn't usually make sense\n");
//    }
//    if (gArgs.IsArgSet("-rpcbind")) {
//      LogPrintf(
//        "WARNING: option -rpcbind was ignored because -rpcallowip was not specified, refusing to allow everyone to connect\n");
//    }
//  } else if (gArgs.IsArgSet("-rpcbind")) { // Specific bind address
//    for (const std::string &strRPCBind : gArgs.GetArgs("-rpcbind")) {
//      int port = http_port;
//      std::string host;
//      SplitHostPort(strRPCBind, port, host);
//      endpoints.push_back(std::make_pair(host, port));
//    }
//  }

  // Bind addresses
  for (std::vector<std::pair<std::string, uint16_t> >::iterator i = endpoints.begin(); i != endpoints.end(); ++i) {
    printf("Binding RPC on address %s port %i\n", i->first.c_str(), i->second);
    evhttp_bound_socket *bind_handle = evhttp_bind_socket_with_handle(http,
                                                                      i->first.empty() ? nullptr : i->first.c_str(),
                                                                      i->second);
    if (bind_handle) {
//      CNetAddr addr;
//      if (i->first.empty() || (LookupHost(i->first.c_str(), addr, false) && addr.IsBindAny())) {
//        LogPrintf("WARNING: the RPC server is not safe to expose to untrusted networks such as the public internet\n");
//      }
      boundSockets.push_back(bind_handle);
    } else {
      printf("Binding RPC on address %s port %i failed.\n", i->first.c_str(), i->second);
    }
  }
  return !boundSockets.empty();
}


bool InitHTTPServer() {

  raii_event_base base_ctr = obtain_event_base();

  /* Create a new evhttp object to handle requests. */
  auto http_ctr = obtain_evhttp(base_ctr.get());

  evhttp_set_timeout(http_ctr.get(), DEFAULT_HTTP_SERVER_TIMEOUT);
  evhttp_set_max_headers_size(http_ctr.get(), MAX_HEADERS_SIZE);
  evhttp_set_max_body_size(http_ctr.get(), MAX_SIZE);


  if (!HTTPBindAddresses(http_ctr.get())) {

    return false;
  }
  //rpc 端口8332

// transfer ownership to eventBase/HTTP via .release()
  eventBase = base_ctr.release();
  eventHTTP = http_ctr.release();

  return true;
}