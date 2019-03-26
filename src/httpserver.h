//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_HTTPSERVER_H
#define BITCOINTEST_HTTPSERVER_H

#include <string>
#include <functional>

#include <event2/http.h>

#include <vector>

static const int DEFAULT_HTTP_SERVER_TIMEOUT=30;
/** Maximum size of http request (request line + headers) */
static const size_t MAX_HEADERS_SIZE = 8192;

/** Initialize HTTP server.
 * Call this before RegisterHTTPHandler or EventBase().
 */
bool InitHTTPServer();


struct evhttp_request;


/** Return evhttp event base. This can be used by submodules to
 * queue timers or custom events.
 */
struct event_base* EventBase();

/** In-flight HTTP request.
 * Thin C++ wrapper around evhttp_request.
 */
class HTTPRequest
{
private:
  struct evhttp_request* req;
  bool replySent;

public:
  explicit HTTPRequest(struct evhttp_request* req);
  ~HTTPRequest();

  enum RequestMethod {
    UNKNOWN,
    GET,
    POST,
    HEAD,
    PUT
  };

  /** Get requested URI.
   */
  std::string GetURI() const;

//  /** Get CService (address:ip) for the origin of the http request.
//   */
//  CService GetPeer() const;

  /** Get request method.
   */
  RequestMethod GetRequestMethod() const;

  /**
   * Get the request header specified by hdr, or an empty string.
   * Return a pair (isPresent,string).
   */
  std::pair<bool, std::string> GetHeader(const std::string& hdr) const;

  /**
   * Read request body.
   *
   * @note As this consumes the underlying buffer, call this only once.
   * Repeated calls will return an empty string.
   */
  std::string ReadBody();

  /**
   * Write output header.
   *
   * @note call this before calling WriteErrorReply or Reply.
   */
  void WriteHeader(const std::string& hdr, const std::string& value);

  /**
   * Write HTTP reply.
   * nStatus is the HTTP status code to send.
   * strReply is the body of the reply. Keep it empty to send a standard message.
   *
   * @note Can be called only once. As this will give the request back to the
   * main thread, do not call any other HTTPRequest methods after calling this.
   */
  void WriteReply(int nStatus, const std::string& strReply = "");
};




/** Handler for requests to a certain HTTP path */
typedef std::function<bool(HTTPRequest* req, const std::string &)> HTTPRequestHandler;


/** Register handler for prefix.
 * If multiple handlers match a prefix, the first-registered one will
 * be invoked.
 */
void RegisterHTTPHandler(const std::string &prefix, bool exactMatch, const HTTPRequestHandler &handler);


/** Event class. This can be used either as a cross-thread trigger or as a timer.
 */
class HTTPEvent
{
public:
  /** Create a new event.
   * deleteWhenTriggered deletes this event object after the event is triggered (and the handler called)
   * handler is the handler to call when the event is triggered.
   */
  HTTPEvent(struct event_base* base, bool deleteWhenTriggered, const std::function<void()>& handler);
  ~HTTPEvent();

  /** Trigger the event. If tv is 0, trigger it immediately. Otherwise trigger it after
   * the given time has elapsed.
   */
  void trigger(struct timeval* tv);

  bool deleteWhenTriggered;
  std::function<void()> handler;
private:
  struct event* ev;
};



#endif //BITCOINTEST_HTTPSERVER_H
