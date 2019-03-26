//
// Created by 邢磊 on 2019/3/25.
//

//#include <event.h>
//#include <memory>
//#include <stdexcept>
//#include <evhttp.h>
//#include <vector>
//#include <string>
//
//#include "httprpc.h"
//#include "httpserver.h"
//
//
//
//
//
//int main() {
//
//
//  auto base = event_base_new(); //event_base都包含了一组事件 对应一个方法（比如 epoll select）
//
//  auto evhttp = evhttp_new(base);//evhttp_new  用于创建一个新的HTTP server
//
//
//
//  raii_event_base base_ctr = obtain_event_base();
//
//  /* Create a new evhttp object to handle requests. */
//  auto http_ctr = obtain_evhttp(base_ctr.get());
//
//  evhttp_set_timeout(http_ctr.get(), DEFAULT_HTTP_SERVER_TIMEOUT);
//  evhttp_set_max_headers_size(http_ctr.get(), MAX_HEADERS_SIZE);
//  evhttp_set_max_body_size(http_ctr.get(), MAX_SIZE);
//
//
//  //rpc 端口8332
//
//  int http_port = 8332;
//  std::vector<std::pair<std::string, uint16_t>> endpoints;
//  endpoints.push_back(std::make_pair("::1", http_port));
//  endpoints.push_back(std::make_pair("127.0.0.1", http_port));
//
//  for (std::vector<std::pair<std::string, uint16_t> >::iterator i = endpoints.begin(); i != endpoints.end(); ++i) {
//
//    evhttp_bound_socket *bind_handle = evhttp_bind_socket_with_handle(http_ctr.get(),
//                                                                      i->first.empty() ? nullptr : i->first.c_str(),
//                                                                      i->second);
//    if (bind_handle) {
//
//      boundSockets.push_back(bind_handle);
//      printf("Binding RPC on address %s port %i success.\n", i->first.c_str(), i->second);
//    } else {
//    }
//  }
//
//// transfer ownership to eventBase/HTTP via .release()
//  eventBase = base_ctr.release();
//  eventHTTP = http_ctr.release();
//
//  StartHTTPRPC();
//
//
//}