cpp_compiler = meson.get_compiler('cpp')

concepts_test_fragment = '''
#include <concepts>

template<typename T>
concept Integral = std::is_integral_v<T>;

int test(Integral auto&& v)
{
  return v;
}

int main()
{
  return test(42);
}
'''

conf_data = configuration_data()
conf_data.set('CPP_UTILS_VERSION', meson.project_version())
if cpp_compiler.compiles(concepts_test_fragment)
    conf_data.set('CPP_UTILS_CONCEPTS_SUPPORTED', true)
else
    warning('Concepts are not supported by the compiler')
endif


if(target_machine.endian() == 'big')
    conf_data.set('CPP_UTILS_BIG_ENDIAN', true)
else
    conf_data.set('CPP_UTILS_LITTLE_ENDIAN', true)
endif
configure_file(input : files('../../config.h.in'),
               output : 'config.h',
               install : true,
               install_dir : 'cpp_utils',
               configuration : conf_data)
