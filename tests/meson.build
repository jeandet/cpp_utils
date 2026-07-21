catch2_dep = dependency('catch2-with-main', version:'>3.0.0', required : true)
cpp_compiler = meson.get_compiler('cpp')

too_many_members_fragment = '''
#include "reflexion/reflection.hpp"
struct thirty_two_members
{
    int a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15,
        a16,a17,a18,a19,a20,a21,a22,a23,a24,a25,a26,a27,a28,a29,a30,a31;
};
constexpr auto n = cpp_utils::reflexion::count_members<thirty_two_members>;
int main() { return static_cast<int>(n); }
'''
assert(not cpp_compiler.compiles(too_many_members_fragment, include_directories : cpp_utils_inc,
    name : 'count_members must reject types with more than 31 members'),
    'cpp_utils::reflexion::count_members should fail to compile for >31-member types, but it compiled')

if get_option('qt')
  qt5 = import('qt6')
  qt5_dep = dependency('qt6', modules : ['Core', 'Widgets', 'Gui'])
else
  qt5_dep = disabler()
endif

if get_option('threading')
  bshoshany_thread_pool_dep = dependency('bshoshany-thread-pool')
else
  bshoshany_thread_pool_dep = disabler()
endif

tests = [
    {
        'name':'TreePrint',
        'sources': ['trees/test_print.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TreePrintQt',
        'sources': ['trees/test_print_qt.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, qt5_dep]
    },
    {
        'name':'TestMemberDetector',
        'sources': ['types/member_detector.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestMethodDetector',
        'sources': ['types/method_detector.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestTypeDetector',
        'sources': ['types/type_detector.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestSmartPointers',
        'sources': ['types/smart_pointers.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestQtTypesDetector',
        'sources': ['types/qt_types_detector.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, qt5_dep]
    },
    {
        'name':'TestConcepts',
        'sources': ['types/concepts.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestContainersTraits',
        'sources': ['containers/test_containers_traits.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestQtContainersTraits',
        'sources': ['containers/test_containers_traits_qt.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, qt5_dep]
    },
    {
        'name':'TestContainersAlgs',
        'sources': ['containers/test_containers_algs.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestNomap',
        'sources': ['containers/test_nomap.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestNoInitVector',
        'sources': ['containers/test_no_init_vector.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestStringsAlgs',
        'sources': ['strings/test_strings_algorithms.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestEndianness',
        'sources': ['endianness/test_endianness.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestLifetime',
        'sources': ['lifetime/test_scope_leaving_guards.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestMemoryMappedFile',
        'sources': ['io/test_memory_mapped_file.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestBufferView',
        'sources': ['io/test_buffer_view.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestDeserialization',
        'sources': ['serde/test_deserialization.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestSerialization',
        'sources': ['serde/test_serialization.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestCountMembers',
        'sources': ['reflexion/test_count_members.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestBoundedString',
        'sources': ['serde/test_bounded_string.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestDynamicArrayBytes',
        'sources': ['serde/test_dynamic_array_bytes.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestDynamicArray',
        'sources': ['serde/test_dynamic_array.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestUnusedField',
        'sources': ['serde/test_unused_field.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestRuntimeSize',
        'sources': ['serde/test_runtime_size.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestScaledField',
        'sources': ['serde/test_scaled_field.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestVisitor',
        'sources': ['types/test_visitor.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestDtypeDispatch',
        'sources': ['types/test_dtype_dispatch.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestParallelFor',
        'sources': ['threading/test_parallel_for.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, bshoshany_thread_pool_dep]
    },
    {
        'name':'TestParallelChunks',
        'sources': ['threading/test_parallel_chunks.cpp'],
        'deps': [cpp_utils_dep, catch2_dep, bshoshany_thread_pool_dep]
    },
    {
        'name':'TestFuzzyMatch',
        'sources': ['strings/test_fuzzy_match.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
    {
        'name':'TestNdShape',
        'sources': ['containers/test_nd_shape.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    },
]

foreach unit_test : tests
  test_exe = executable(unit_test['name'],unit_test['sources'],
                          dependencies : unit_test['deps']
						  )
  test('Test-' + unit_test['name'], test_exe, args:['-o','@0@_out.xml'.format(unit_test['name']), '-r', 'xml'])
endforeach
