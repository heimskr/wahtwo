#include <iostream>
#include <string>
#include <vector>

#include "wahtwo/Watcher.h"

int main() {
	Wahtwo::Watcher watcher({"src", "include"});
	watcher.filter        = [](const auto &path) { return path.extension() == ".cpp";  };
	watcher.onRemoveSelf  = [](const auto &path) { std::cout << path << " removed\n";  };
	watcher.onRenameSelf  = [](const auto &path) { std::cout << path << " renamed\n";  };
	watcher.onCreate      = [](const auto &path) { std::cout << path << " created\n";  };
	watcher.onModify      = [](const auto &path) { std::cout << path << " modified\n"; };
	watcher.onClone       = [](const auto &path) { std::cout << path << " cloned\n";   };
	watcher.onAttributes  = [](const auto &path) { std::cout << path << " changed attributes\n"; };
	watcher.onRemoveChild = [](const auto &dir, const auto &child) {
		std::cout << child << " removed in " << dir << '\n';
	};
	watcher.onRenameChild = [](const auto &dir, const auto &child) {
		std::cout << child << " renamed in " << dir << '\n';
	};
	watcher.onAny = [&watcher](const auto &path) {
		if (path.string().find("stop") != std::string::npos) {
			std::cout << "Stopping watcher.\n";
			watcher.stop();
		}
	};
	watcher.start();
}

