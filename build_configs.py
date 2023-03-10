release = ["-DCMAKE_BUILD_TYPE=Release"]
debug = ["-DCMAKE_BUILD_TYPE=Debug"]
# USE_GLIBCXX_DEBUG is not compatible with USE_LP (see issue983).
glibcxx_debug = ["-DCMAKE_BUILD_TYPE=Debug", "-DUSE_LP=NO", "-DUSE_GLIBCXX_DEBUG=YES"]
minimal = ["-DCMAKE_BUILD_TYPE=Release", "-DDISABLE_PLUGINS_BY_DEFAULT=YES"]

# Create our own build. By default, it starts with all plugins
# disabled, and then we re-enable only those we need.
# For more info on plugins, see "Manual and Custom Builds" in
# https://www.fast-downward.org/ObtainingAndRunningFastDownward
ipc2023 = ["-DCMAKE_BUILD_TYPE=Release",
           "-DDISABLE_PLUGINS_BY_DEFAULT=YES",
           "-DPLUGIN_PLUGIN_EAGER_GREEDY_ENABLED=True",
           "-DPLUGIN_PDBS_ENABLED=True"]

DEFAULT = "release"
DEBUG = "debug"
