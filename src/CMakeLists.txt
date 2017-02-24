include_directories(${OPENAL_INCLUDE_DIR})

qt5_add_resources(QRC_FILE ../miniplayer.qrc)

add_executable(miniplayer
  main.cpp
  miniplayer/MiniPlayer.cpp
  miniplayer/qt/QmlMiniPlayer.cpp
  miniplayer/qt/QmlVideoSurface.cpp
  miniplayer/qt/SGVideoNode.cpp
  miniplayer/output/audio/AudioOutputOpenAL.cpp
  ${QRC_FILE}
)

target_link_libraries(miniplayer
  Qt5::Quick
  ${OPENAL_LIBRARY}
  ${FFMPEG_LIBAVFORMAT}
  ${FFMPEG_LIBAVCODEC}
  ${FFMPEG_LIBAVUTIL}
  ${FFMPEG_LIBSWRESAMPLE}
  ${CMAKE_THREAD_LIBS_INIT}
)

install(TARGETS miniplayer RUNTIME DESTINATION bin)
