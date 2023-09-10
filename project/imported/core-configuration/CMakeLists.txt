add_library(configuration STATIC)
target_sources(configuration
    PRIVATE

    include/configuration/Configuration.h
    include/configuration/Entry.h
    include/configuration/Types.h

    src/Configuration.cpp
    src/Entry.cpp
)
target_include_directories(configuration
    PUBLIC
    include
    PRIVATE
    include/configuration
)
target_link_libraries(configuration
    PUBLIC
    core-utils
    json
)

set_target_properties(configuration PROPERTIES FOLDER "3rdParty")
