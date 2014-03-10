#pragma once
#ifndef W_PRIVATE_HPP_INCLUDED
#define W_PRIVATE_HPP_INCLUDED

#include <event2/http.h>
#include "w.hpp"

namespace w {
  namespace priv {
    Request make_request_from_evhttp_request(evhttp_request*);
  }
}

#endif /* end of include guard: SYMBOL */
