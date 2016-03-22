ADD_EXECUTABLE (mkconsole make-console.cc)
TARGET_LINK_LIBRARIES (mkconsole ${NETPLAY_LIBS})

ADD_EXECUTABLE (start-game start-game.cc)
TARGET_LINK_LIBRARIES (start-game ${NETPLAY_LIBS})
