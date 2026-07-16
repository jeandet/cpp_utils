conf_data = configuration_data()
conf_data.set('CPP_UTILS_VERSION', meson.project_version())
configure_file(input : files('../../config.h.in'),
               output : 'config.h',
               install : true,
               install_dir : 'cpp_utils',
               configuration : conf_data)
