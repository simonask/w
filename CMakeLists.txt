cmake_minimum_required(VERSION 2.6)

project(wayward)

include_directories(.)
include_directories(/usr/local/include)
set(CMAKE_CXX_FLAGS "-std=c++11 -O0 -g")

set(WAYWARD_SUPPORT_SOURCES
  wayward/support/format.cpp
  wayward/support/uri.cpp
  wayward/support/node.cpp
  wayward/support/json.cpp
  wayward/support/datetime/datetime.cpp
  wayward/support/datetime/clock.cpp
  wayward/support/datetime/interval.cpp
  wayward/support/logger.cpp
  wayward/support/error.cpp
)

set(WAYWARD_SOURCES
  wayward/app.cpp
  wayward/render.cpp
  wayward/request.cpp
  wayward/log.cpp
)

set(WAYWARD_TESTING_SOURCES
  wayward/testing/time_machine.cpp
)

set(PERSISTENCE_SOURCES
  persistence/connection_provider.cpp
  persistence/connection_retainer.cpp
  persistence/adapter.cpp
  persistence/data_store.cpp
  persistence/connection_pool.cpp
  persistence/p.cpp
  persistence/postgresql.cpp
  persistence/postgresql_renderers.cpp
  persistence/primary_key.cpp
  persistence/relational_algebra.cpp
  persistence/types.cpp
  persistence/datetime.cpp
)

add_library(wayward_support SHARED ${WAYWARD_SUPPORT_SOURCES})
add_library(wayward SHARED ${WAYWARD_SOURCES})
add_library(persistence SHARED ${PERSISTENCE_SOURCES})
add_library(wayward_testing SHARED ${WAYWARD_TESTING_SOURCES})

target_link_libraries(wayward wayward_support)
target_link_libraries(persistence wayward_support)
target_link_libraries(wayward_testing wayward wayward_support)

find_library(LIBEVENT event)
find_library(LIBPQ pq)
target_link_libraries(persistence ${LIBPQ})
target_link_libraries(wayward ${LIBEVENT})
target_link_libraries(wayward_support ${LIBEVENT})

if (test)
  add_subdirectory(tests)
  enable_testing()
endif(test)
