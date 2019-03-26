//
// Created by 邢磊 on 2019/3/26.
//

#ifndef BITCOINTEST_EVENTS_H
#define BITCOINTEST_EVENTS_H

#include <event2/event.h>
#include <event2/http.h>


struct event_base_deleter {
  void operator()(struct event_base *ob) {
    event_base_free(ob);
  }
};

struct evhttp_deleter {
  void operator()(struct evhttp *ob) {
    evhttp_free(ob);
  }
};

typedef std::unique_ptr<struct event_base, event_base_deleter> raii_event_base;
typedef std::unique_ptr<struct evhttp, evhttp_deleter> raii_evhttp;


raii_event_base obtain_event_base() {  //todo ??? inline

  auto result = raii_event_base(event_base_new());  //类似于vector<int>() //匿名对象

  if (!result.get()) {
    throw std::runtime_error("cannot create event_baase");
  }

  return result;
}


raii_evhttp obtain_evhttp(struct event_base *base) {

  auto result = raii_evhttp(evhttp_new(base));

  if (!result.get()) {
    throw std::runtime_error("cannot create evhttp");
  }

  return result;

}

#endif //BITCOINTEST_EVENTS_H
