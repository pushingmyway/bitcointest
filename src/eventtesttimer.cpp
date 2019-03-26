//
// Created by 邢磊 on 2019/3/26.
//

#include <memory>
#include <event2/event-config.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/util.h>
#include <event2/listener.h>

#include <stdio.h>
#include <iostream>
using namespace std;

//定时器的实现

#define MAKE_RAII(type) \
/* deleter */\
struct type##_deleter {\
    void operator()(struct type* ob) {\
        type##_free(ob);\
    }\
};\
/* unique ptr typedef */\
typedef std::unique_ptr<struct type, type##_deleter> raii_##type

MAKE_RAII(event_base);
MAKE_RAII(event);
MAKE_RAII(evconnlistener);

inline raii_event_base obtain_event_base() {
  auto result = raii_event_base(event_base_new());
  if (!result.get())
    throw std::runtime_error("cannot create event_base");
  return result;
}

inline raii_event obtain_event(struct event_base* base, evutil_socket_t s, short events, event_callback_fn cb, void* arg) {
  return raii_event(event_new(base, s, events, cb, arg));
}

inline raii_evconnlistener obtain_evconnlistener(struct event_base *base, evconnlistener_cb cb, void *ptr, unsigned flags, int backlog, const struct sockaddr *sa, int socklen) {
  return raii_evconnlistener(
    evconnlistener_new_bind(base, cb, ptr, flags, backlog, sa, socklen));
}

struct timeval lasttime;

static void
timeoutCallback(evutil_socket_t fd, short event, void *arg)
{
  struct timeval newtime, difference;
  struct event *timeout = (struct event *)arg;
  double elapsed;
  struct timeval tv{2,0};

  evutil_gettimeofday(&newtime, NULL);
  evutil_timersub(&newtime, &lasttime, &difference);
  elapsed = difference.tv_sec +
            (difference.tv_usec / 1.0e6);

  cout << "timeoutCallback called at "<<newtime.tv_sec <<" , " << elapsed << " seconds elapsed."<<endl;
  lasttime = newtime;


//	evutil_timerclear(&tv);
//	tv.tv_sec = 2;
  event_add(timeout, &tv);

}


/**
 *
 * event_new()   初始化状态
 * add()         等待状态
 * 触发事件满足    激活状态
 *
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char **argv)
{
  struct timeval tv{2,0};
  int flags;
  flags = EV_PERSIST;

  // Obtain event base
  auto raii_base = obtain_event_base();

  auto raii_event = obtain_event(raii_base.get(), -1, 0, nullptr, nullptr);
  /* Initalize one event */
  event_assign(raii_event.get(), raii_base.get(), -1, flags, timeoutCallback, (void*) raii_event.get());

//	evutil_timerclear(&tv);
//	tv.tv_sec = 2;
  event_add(raii_event.get(), &tv);  //这里的tv表示2秒的超时

  evutil_gettimeofday(&lasttime, NULL);

  event_base_dispatch(raii_base.get());

  return 0;
}
