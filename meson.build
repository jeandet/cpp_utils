project(
  'cpp_utils',
  'cpp',
  meson_version: '>= 1.2.0',
  version : '0.1.0',
  default_options : ['cpp_std=c++20', 'buildtype=release']
)

hedley_dep = dependency('hedley', main : true, fallback : ['hedley', 'hedley_dep'])

cpp_utils_inc = include_directories(['include', '.', 'include/cpp_utils'])

# Emscripten disables C++ exceptions by default; cpp_utils (e.g. nomap, dtype_dispatch) throws
# on invalid input, so anything depending on cpp_utils_dep needs exceptions re-enabled for both
# compiling and linking when targeting it.
if meson.get_compiler('cpp').get_id() == 'emscripten'
    cpp_utils_emscripten_args = ['-fexceptions']
else
    cpp_utils_emscripten_args = []
endif

subdir('include/cpp_utils')

headers = files(
    [
        'include/cpp_utils/containers/algorithms.hpp',
        'include/cpp_utils/containers/nd_shape.hpp',
        'include/cpp_utils/containers/traits.hpp',
        'include/cpp_utils/containers/no_init_vector.hpp',
        'include/cpp_utils/containers/nomap.hpp',
        'include/cpp_utils/cpp_utils.hpp',
        'include/cpp_utils/cpp_utils_qt/cpp_utils_qt.hpp',
        'include/cpp_utils/cpp_utils_qt/types/converters.hpp',
        'include/cpp_utils/cpp_utils_qt/types/pointers.hpp',
        'include/cpp_utils/cpp_utils_qt/trees/traits.hpp',
        'include/cpp_utils/endianness/endianness.hpp',
        'include/cpp_utils/io/buffer_view.hpp',
        'include/cpp_utils/io/memory_mapped_file.hpp',
        'include/cpp_utils/io/owned_buffer.hpp',
        'include/cpp_utils/io/sequential_writer.hpp',
        'include/cpp_utils/lifetime/scope_leaving_guards.hpp',
        'include/cpp_utils/reflexion/enum_name.hpp',
        'include/cpp_utils/reflexion/field_name.hpp',
        'include/cpp_utils/reflexion/reflection.hpp',
        'include/cpp_utils/serde/context.hpp',
        'include/cpp_utils/serde/deserialization.hpp',
        'include/cpp_utils/serde/serialization.hpp',
        'include/cpp_utils/serde/runtime_size.hpp',
        'include/cpp_utils/serde/special_fields.hpp',
        'include/cpp_utils/serde/serde.hpp',
        'include/cpp_utils/strings/algorithms.hpp',
        'include/cpp_utils/strings/fuzzy_match.hpp',
        'include/cpp_utils/strings/traits.hpp',
        'include/cpp_utils/threading/parallel_chunks.hpp',
        'include/cpp_utils/threading/parallel_for.hpp',
        'include/cpp_utils/trees/algorithms.hpp',
        'include/cpp_utils/trees/node.hpp',
        'include/cpp_utils/trees/traits.hpp',
        'include/cpp_utils/types/concepts.hpp',
        'include/cpp_utils/types/detectors.hpp',
        'include/cpp_utils/types/dtype_dispatch.hpp',
        'include/cpp_utils/types/integers.hpp',
        'include/cpp_utils/types/pointers.hpp',
        'include/cpp_utils/types/strings.hpp',
        'include/cpp_utils/types/visitor.hpp'
    ]
)

cpp_utils_project_extra_files = files(
  '.github/workflows/CI.yml',
  '.github/workflows/tests-with-coverage.yml'
)
cpp_utils_dep = declare_dependency(
        include_directories : cpp_utils_inc,
        dependencies: [hedley_dep],
        compile_args: cpp_utils_emscripten_args,
        link_args: cpp_utils_emscripten_args,
        extra_files: [headers, cpp_utils_project_extra_files]
)
meson.override_dependency('cpp_utils', cpp_utils_dep)

# Header-only: installing is copying the header tree. Headers include each other with relative
# paths, so the include root is the only include path consumers need (plus hedley.h).
install_subdir('include/cpp_utils',
               install_dir : get_option('includedir'),
               exclude_files : ['meson.build'])
import('pkgconfig').generate(
    name : 'cpp_utils',
    description : 'Header-only C++20 utilities (containers, io, reflexion, serde); needs hedley.h',
    url : 'https://github.com/jeandet/cpp_utils',
    install_dir : get_option('datadir') / 'pkgconfig',
)



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
