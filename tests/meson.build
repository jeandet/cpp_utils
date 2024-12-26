catch2_dep = dependency('catch2-with-main', version:'>3.0.0', required : true)

if get_option('qt')
  qt5 = import('qt6')
  qt5_dep = dependency('qt6', modules : ['Core', 'Widgets', 'Gui'])
else
  qt5_dep = disabler()
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
]

foreach unit_test : tests
  test_exe = executable(unit_test['name'],unit_test['sources'],
                          dependencies : unit_test['deps']
						  )
  test('Test-' + unit_test['name'], test_exe, args:['-o','@0@_out.xml'.format(unit_test['name']), '-r', 'xml'])
endforeach
