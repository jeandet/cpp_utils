project(
  'cpp_utils',
  'cpp',
  default_options : ['cpp_std=c++17']
)

hedley_dep = dependency('hedley', main : true, fallback : ['hedley', 'hedley_dep'])

cpp_utils_inc = include_directories(['include','.'])

cpp_utils_dep = declare_dependency(
        include_directories : cpp_utils_inc,
        dependencies: [hedley_dep]
)

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
if(target_machine.endian() == 'big')
    conf_data.set('CPP_UTILS_BIG_ENDIAN', true)
else
    conf_data.set('CPP_UTILS_LITTLE_ENDIAN', true)
endif
configure_file(input : 'config.h.in',
               output : 'config.h',
               format : 'cmake',
               install : true,
               install_dir : 'cpp_utils',
               configuration : conf_data)

cppcheck = find_program('cppcheck', required : false)

if cppcheck.found()
  run_target('cppcheck',
              command : [cppcheck, '--enable=all',
              '--project=' + join_paths(meson.build_root(), 'compile_commands.json')]
              )
endif

subdir('tests')
