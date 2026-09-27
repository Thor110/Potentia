# Copies SRC to DST only when DST is not there yet: for files a person changes after the build
# puts them in place (maps/default.map, the map anchors are added to), which a rebuild must not
# overwrite. Run with: cmake -DSRC=... -DDST=... -P copy_if_missing.cmake
if(NOT EXISTS "${DST}")
    get_filename_component(dir "${DST}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
    file(COPY_FILE "${SRC}" "${DST}")
endif()
