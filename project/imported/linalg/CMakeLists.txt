add_library(linalg STATIC)
TARGET_SOURCES(linalg
    PRIVATE
    include/linalg/linalg.h

    src/linalg_impl.cpp
)
target_include_directories(linalg PUBLIC include)