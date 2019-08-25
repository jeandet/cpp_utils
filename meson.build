project(
  'cpp_utils',
  'cpp',
  default_options : ['cpp_std=c++17']
)

cpp_utils_inc = include_directories(['include'])

cpp_utils_dep = declare_dependency(
	include_directories : cpp_utils_inc
)

cppcheck = find_program('cppcheck', required : false)

if cppcheck.found()
  run_target('cppcheck',
              command : [cppcheck, '--enable=all',
              '--project=' + join_paths(meson.build_root(), 'compile_commands.json')]
              )
endif

subdir('tests')
