cmake_minimum_required(VERSION 2.6)

project(wayward)

include_directories(.)
set(CMAKE_CXX_FLAGS "-std=c++11 -O0 -g")

set(WAYWARD_SUPPORT_SOURCES
  wayward/support/format.cpp
  wayward/support/uri.cpp
)

set(WAYWARD_SOURCES
  wayward/app.cpp
  wayward/render.cpp
  wayward/request.cpp
)

set(PERSISTENCE_SOURCES
  persistence/p.cpp
  persistence/postgresql.cpp
  persistence/postgresql_renderers.cpp
  persistence/primary_key.cpp
  persistence/relational_algebra.cpp
  persistence/types.cpp
)

add_library(wayward_support SHARED ${WAYWARD_SUPPORT_SOURCES})
add_library(wayward SHARED ${WAYWARD_SOURCES})
add_library(persistence SHARED ${PERSISTENCE_SOURCES})

target_link_libraries(wayward wayward_support)
target_link_libraries(persistence wayward_support)

find_library(LIBEVENT event)
find_library(LIBPQ pq)
target_link_libraries(persistence ${LIBPQ})
target_link_libraries(wayward ${LIBEVENT})
target_link_libraries(wayward_support ${LIBEVENT})

add_executable(test test.cpp)
target_link_libraries(test wayward)
target_link_libraries(test persistence)
