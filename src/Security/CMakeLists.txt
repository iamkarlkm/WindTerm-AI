# Security Module
add_library(WindTermSecurity STATIC
    CredentialManager.h
    CredentialManager.cpp
)

target_link_libraries(WindTermSecurity PUBLIC
    Qt5::Core
)

target_include_directories(WindTermSecurity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
