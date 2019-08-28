catch2_dep = dependency('catch2', required : true, fallback:['catch2','catch2_dep'])

if get_option('qt')
  qt5 = import('qt5')
  qt5_dep = dependency('qt5', modules : ['Core', 'Widgets', 'Gui'])
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
        'name':'TestContainers',
        'sources': ['containers/test_containers.cpp'],
        'deps': [cpp_utils_dep, catch2_dep]
    }
]

foreach unit_test : tests
  test_exe = executable(unit_test['name'],unit_test['sources'],
                          dependencies : unit_test['deps']
						  )
  test('Test-' + unit_test['name'], test_exe)
endforeach
