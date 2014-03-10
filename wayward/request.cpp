#include "wayward/private.hpp"

#include <event2/keyvalq_struct.h>
#include <event2/buffer.h>
#include <stdlib.h>
#include <sstream>

namespace w {
  namespace priv {
    Request make_request_from_evhttp_request(evhttp_request* req) {
      Request r;

      switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET:     r.method = "GET";     break;
        case EVHTTP_REQ_POST:    r.method = "POST";    break;
        case EVHTTP_REQ_HEAD:    r.method = "HEAD";    break;
        case EVHTTP_REQ_PUT:     r.method = "PUT";     break;
        case EVHTTP_REQ_DELETE:  r.method = "DELETE";  break;
        case EVHTTP_REQ_OPTIONS: r.method = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE:   r.method = "TRACE";   break;
        case EVHTTP_REQ_CONNECT: r.method = "CONNECT"; break;
        case EVHTTP_REQ_PATCH:   r.method = "PATCH";   break;
      }

      evkeyvalq* headers = evhttp_request_get_input_headers(req);
      for (evkeyval* header = headers->tqh_first; header; header = header->next.tqe_next) {
        r.headers[header->key] = header->value;
      }

      evbuffer* body = evhttp_request_get_input_buffer(req);
      if (body) {
        std::stringstream body_buffer;
        while (evbuffer_get_length(body)) {
          char tmp[128];
          int n = evbuffer_remove(body, tmp, sizeof(tmp));
          if (n > 0)
            body_buffer.write(tmp, n);
        }
        r.body = body_buffer.str();
      }

      char* decoded_uri_str = evhttp_decode_uri(evhttp_request_get_uri(req));
      r.uri = URI(decoded_uri_str);
      ::free(decoded_uri_str);
      return r;
    }
  }
}
