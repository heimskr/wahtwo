## Wahtwō

Wahtwō (from the [Proto-Germanic word](https://en.wiktionary.org/wiki/Reconstruction:Proto-Germanic/wahtw%C5%8D) for "watch" or "vigil") is a C++ wrapper for FSEvents (on macOS) and inotify (on Linux).

### Example

Whenever a .cpp file in the current directory is modified, this example will print its name.  
Whenever a .py file is created in the current directory, the program will quit.  
Only files with 'a' in the name are monitored.

```c++
Wahtwo::Watcher watcher({"."}, true);

watcher.filter = [](const std::filesystem::path &path) {
    return path.stem().string().find('a') != std::string::npos;
};

watcher.onModify = [](const std::filesystem::path &path) {
    if (path.extension() == ".cpp")
        std::cout << path << std::endl;
};

watcher.onCreate = [&](const std::filesystem::path &path) {
    if (path.extension() == ".py")
        watcher.stop();
};

watcher.start();
```