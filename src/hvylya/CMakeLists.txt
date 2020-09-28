# Specify sources we build.
file (GLOB HVYLYA_SOURCES core/*.cpp core/*.h filters/*.cpp filters/*.h filters/fm/*.cpp filters/fm/*.h pipelines/async/*.cpp pipelines/async/*.h)
set_property (SOURCE ${HVYLYA_SOURCES} PROPERTY LABELS Hvylya)

add_library (hvylya STATIC ${HVYLYA_SOURCES})
set_property (TARGET hvylya PROPERTY LABELS Hvylya)

# Add tracking dependency for our sub-project.
add_dependencies (Hvylya hvylya)

add_subdirectory (core/tests)
add_subdirectory (filters/tests)
add_subdirectory (pipelines/async/tests)
