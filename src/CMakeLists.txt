add_library(CHKSYSTEMD chk-systemd.cpp chk-systemd-utils.cpp)
target_link_libraries(CHKSYSTEMD ${LIBS})

add_library(CHKCTL chk-ctl.cpp)
target_link_libraries(CHKCTL ${LIBS} CHKSYSTEMD)

add_library(CHKUI chk-wmain.cpp chk-wutils.cpp)
target_link_libraries(CHKUI ${LIBS} CHKSYSTEMD CHKCTL)

add_executable(chkservice chkservice.cpp)
target_link_libraries(chkservice ${LIBS} CHKSYSTEMD CHKCTL CHKUI)
target_link_libraries(chkservice boost_regex)

install(TARGETS chkservice RUNTIME DESTINATION bin)
