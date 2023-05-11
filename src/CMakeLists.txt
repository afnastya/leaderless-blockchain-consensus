set(HEADERS     core/message.hpp
                core/transaction.hpp
                core/chain.hpp
                network/channel.hpp
                network/netmanager.hpp
                network/network.hpp
                consensus/metrics.hpp
                consensus/ReliableBroadcast.hpp
                consensus/BinConsensus.hpp
                consensus/DBFT.hpp
                node/node.hpp
                simulation/simulation.hpp
                simulation/structs.hpp)

set(SOURCES     core/transaction.cpp
                network/network.cpp
                consensus/ReliableBroadcast.cpp
                consensus/BinConsensus.cpp
                consensus/DBFT.cpp
                node/node.cpp
                simulation/simulation.cpp)

add_library(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} glog::glog)
