#include <iostream>
#include <string>
#include <vector>

#include "wahtwo/Watcher.h"

int main() {
	const std::vector<std::string> paths {"./src", "./include"};

	Wahtwo::Watcher watcher(paths);
	watcher.filter   = [](const auto &path) { return path.extension() == ".cpp"; };
	watcher.onCreate = [](const auto &path) { std::cout << path << " created\n"; };
	watcher.onRemove = [](const auto &path) { std::cout << path << " removed\n"; };
	watcher.onRename = [](const auto &path) { std::cout << path << " renamed\n"; };
	watcher.onModify = [](const auto &path) { std::cout << path << " modified\n"; };
	watcher.onOwnerChange = [](const auto &path) { std::cout << path << " changed owner\n"; };
	watcher.onClone = [](const auto &path) { std::cout << path << " cloned\n"; };
	watcher.start();
	watcher.worker.join();
}

