project('websocket', 'c', version: '0.0.1',
default_options: [
    'warning_level=3',
    'debug=true',
    'optimization=3',
    'c_std=gnu11',
])

inc_dir = include_directories('include')

executable(
    meson.project_name(),
['src/server.c'],
include_directories: inc_dir,
build_by_default: true,
)
