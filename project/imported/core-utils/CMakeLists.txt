project(core-utils)

add_library(core-utils STATIC)
target_sources(core-utils
    PRIVATE

    include/core-utils/EnumUtils.h
    include/core-utils/Exceptions.h
    include/core-utils/FileUtils.h
    include/core-utils/FlagUtils.h
    include/core-utils/GlmInclude.h
    include/core-utils/IteratorUtils.h
    include/core-utils/ListUtils.h
    include/core-utils/MathUtils.h
    include/core-utils/StringUtils.h
    include/core-utils/TypeUtils.h

    src/Exceptions.cpp
    src/FileUtils.cpp
    src/ListUtils.cpp
    src/StringUtils.cpp
)
target_include_directories(core-utils
    PUBLIC
    include
    PRIVATE
    include/core-utils
)
target_link_libraries(core-utils
    PUBLIC
    glm
    spdlog
)

set_target_properties(core-utils PROPERTIES FOLDER "3rdParty")