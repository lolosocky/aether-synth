# Station design system: include() this after JUCE is available, then link a target with station_use(<target>).
set(STATION_DIR ${CMAKE_CURRENT_LIST_DIR})
if(NOT TARGET StationFonts)
  juce_add_binary_data(StationFonts
    HEADER_NAME StationFontData.h
    NAMESPACE StationFontData
    SOURCES
      ${STATION_DIR}/fonts/SpaceGrotesk_400Regular.ttf
      ${STATION_DIR}/fonts/SpaceGrotesk_500Medium.ttf
      ${STATION_DIR}/fonts/SpaceGrotesk_700Bold.ttf
      ${STATION_DIR}/fonts/IBMPlexMono_400Regular.ttf
      ${STATION_DIR}/fonts/IBMPlexMono_500Medium.ttf)
  set_target_properties(StationFonts PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()

function(station_use target)
  target_sources(${target} PRIVATE ${STATION_DIR}/Station.cpp)
  target_include_directories(${target} PRIVATE ${STATION_DIR})
  target_link_libraries(${target} PRIVATE StationFonts)
endfunction()
