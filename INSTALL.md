
# Configuring the Build
Build with default configuration:
```
cmake -B <build_directory>
```
Build with specified install location:
```
cmake -B <build_directory> -DCMAKE_INSTALL_PREFIX=<install_path>
```
Build the static version of the library only:
```
cmake -B <build_directory> -DCLOGGER_BUILD_STATIC=ON -DCLOGGER_BUILD_SHARED=OFF
```
View and configure all build options with `ncurses` interface:
```
ccmake -B <build_directory>
```

# Building the library
```
cmake --build <build_directory>
```

# Installing the library
```
cmake --build <build_directory> -t install
```
