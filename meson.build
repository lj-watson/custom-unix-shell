project(
  'shs',
  'c',
  default_options : ['c_std=c17', 'warning_level=3'],
)
add_global_arguments('-D_DEFAULT_SOURCE', language : 'c')

inc = include_directories('include')

subdir('include')
subdir('src')

shs_lib = shared_library(
  'shs',
  shs_sources,
  include_directories : inc,
)

shs_exe = executable(
  'shs',
  'src/main.c',
  include_directories : inc,
  link_with : [shs_lib]
)
