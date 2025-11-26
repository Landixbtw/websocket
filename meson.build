project('websocket', 'c', version: '0.0.1',
default_options: [
    'warning_level=3',
    'debug=true',
    'optimization=3',
    'c_std=gnu11',
])

inc_dir = include_directories('include')

executable(
   'server', 
['src/server.c', 'src/utils.c'],
include_directories: inc_dir,
build_by_default: true,
)

executable(
  'client',
['src/client.c', 'src/utils.c'],
include_directories: inc_dir,
build_by_default: true,
)

python = import('python').find_installation('python3')

# Define the 'format' target
run_target(
  'format',
  command : [
    python, 
    # Use files() so Meson knows exactly where the script is
    files('scripts/format.py'), 
    # Pass the absolute path to your project root
    meson.project_source_root()
  ]
)

run_target(
  'clang-check',
  command : [
    python, 
    files('scripts/format.py'), 
    meson.project_source_root(), 
    '--check'
  ]
)
