include_directories(${FFMPEG_INCLUDE_DIR})
include_directories(${OPENAL_INCLUDE_DIR})

qt5_add_resources(QRC_FILE ../miniplayer.qrc)

add_executable(miniplayer
  main.cpp
  miniplayer/Command.hpp
  miniplayer/MiniPlayer.cpp
  miniplayer/MiniPlayer.hpp
  miniplayer/Queue.hpp
  miniplayer/qt/QmlMiniPlayer.cpp
  miniplayer/qt/QmlMiniPlayer.hpp
  miniplayer/qt/QmlVideoSurface.cpp
  miniplayer/qt/QmlVideoSurface.hpp
  miniplayer/qt/QVideoFrame.hpp
  miniplayer/qt/SGVideoNode.cpp
  miniplayer/qt/SGVideoNode.hpp
  miniplayer/output/audio/AudioOutput.hpp
  miniplayer/output/audio/AudioOutputOpenAL.cpp
  miniplayer/output/audio/AudioOutputOpenAL.hpp
  ${QRC_FILE}
)

target_link_libraries(miniplayer
  Qt5::Quick
  ${OPENAL_LIBRARY}
  ${FFMPEG_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS miniplayer RUNTIME DESTINATION bin)
