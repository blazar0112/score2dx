get_property(HEADERS GLOBAL PROPERTY PROP_HEADERS)
set_property(GLOBAL PROPERTY
    PROP_HEADERS
    ${HEADERS}
)

get_property(SOURCES GLOBAL PROPERTY PROP_SOURCES)
set_property(GLOBAL PROPERTY
    PROP_SOURCES
    ${SOURCES}
)

get_property(PUBLIC_HEADERS GLOBAL PROPERTY PROP_PUBLIC_HEADERS)
set_property(GLOBAL PROPERTY
    PROP_PUBLIC_HEADERS
    ${PUBLIC_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/IsDuration.hxx
)

get_property(TEST_SOURCES GLOBAL PROPERTY PROP_TEST_SOURCES)
set_property(GLOBAL PROPERTY
    PROP_TEST_SOURCES
    ${TEST_SOURCES}
)
