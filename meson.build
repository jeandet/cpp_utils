project(
  'cpp_utils',
  'cpp',
  meson_version: '>= 1.2.0',
  default_options : ['cpp_std=c++23']
)

hedley_dep = dependency('hedley', main : true, fallback : ['hedley', 'hedley_dep'])

cpp_utils_inc = include_directories(['include', '.', 'include/cpp_utils'])


headers = files(
    [
        'include/cpp_utils/containers/algorithms.hpp',
        'include/cpp_utils/containers/traits.hpp',
        'include/cpp_utils/containers/no_init_vector.hpp',
        'include/cpp_utils/containers/nomap.hpp',
        'include/cpp_utils/cpp_utils.hpp',
        'include/cpp_utils/cpp_utils_qt/cpp_utils_qt.hpp',
        'include/cpp_utils/cpp_utils_qt/types/converters.hpp',
        'include/cpp_utils/cpp_utils_qt/types/pointers.hpp',
        'include/cpp_utils/cpp_utils_qt/trees/traits.hpp',
        'include/cpp_utils/endianness/endianness.hpp',
        'include/cpp_utils/lifetime/scope_leaving_guards.hpp',
        'include/cpp_utils/reflexion/reflection.hpp',
        'include/cpp_utils/strings/algorithms.hpp',
        'include/cpp_utils/trees/algorithms.hpp',
        'include/cpp_utils/trees/node.hpp',
        'include/cpp_utils/trees/traits.hpp',
        'include/cpp_utils/types/concepts.hpp',
        'include/cpp_utils/types/detectors.hpp',
        'include/cpp_utils/types/pointers.hpp',
        'include/cpp_utils/types/qt_types.hpp',
        'include/cpp_utils/types/strings.hpp'
    ]
)

cpp_utils_dep = declare_dependency(
        include_directories : cpp_utils_inc,
        dependencies: [hedley_dep],
        extra_files: headers
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


if get_option('with_tests')
    subdir('tests')
endif
