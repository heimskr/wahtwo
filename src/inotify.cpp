#include <iostream>

#include <csignal>
#include <stdexcept>
#include <string>
#include <vector>

#include "wahtwo/Watcher.h"

namespace Wahtwo {
	static constexpr size_t BUFFER_SIZE = sizeof(inotify_event) + NAME_MAX + 1;

	INotifyWatcher::INotifyWatcher(const std::vector<std::string> &paths_, bool): paths(paths_) {}

	INotifyWatcher::~INotifyWatcher() {
		if (running)
			stop();
	}

	void INotifyWatcher::start() {
		if (running)
			stop();

		running = true;

		fd = inotify_init1(IN_CLOEXEC);
		if (fd == -1)
			throw Error("inotify_init failed", errno);

		for (const auto &path: paths) {
			const int wd = inotify_add_watch(fd, path.c_str(),
				IN_MODIFY | IN_MOVE_SELF | IN_ATTRIB | IN_CREATE | IN_DELETE_SELF | IN_MOVE | IN_DELETE);
			if (wd == -1)
				throw Error("inotify_add_watch failed", errno);
			watchDescriptors.emplace(wd, path);
		}

		ssize_t count = -2;
		std::cerr << "Starting.\n";

		auto buffer = std::make_unique<char[]>(BUFFER_SIZE);
		const auto *event = reinterpret_cast<const inotify_event *>(buffer.get());

		while (0 < (count = ::read(fd, buffer.get(), BUFFER_SIZE))) {
			const auto &wd_path = watchDescriptors.at(event->wd);
			const auto mask = event->mask;
			const std::filesystem::path path =
				std::string(reinterpret_cast<const char *>(event) + offsetof(inotify_event, name), event->len);
			if (onAny)
				onAny(path);
			if ((mask & IN_CREATE) != 0 && onCreate)
				onCreate(wd_path, path);
			if ((mask & IN_DELETE_SELF) != 0 && onRemoveSelf)
				onRemoveSelf(path);
			if ((mask & IN_DELETE) != 0 && onRemoveChild)
				onRemoveChild(wd_path, path);
			if ((mask & IN_MOVE_SELF) != 0 && onRenameSelf)
				onRenameSelf(wd_path);
			if ((mask & IN_MOVE) != 0 && onRenameChild)
				onRenameChild(wd_path, path);
			if ((mask & IN_MODIFY) != 0 && onModify)
				onModify(path);
			if ((mask & IN_ATTRIB) != 0 && onAttributes)
				onAttributes(path);
		}

		if (count == -1)
			throw Error("read failed", errno);

		::close(fd);
		fd = -1;

		std::cerr << "Done. count[" << count << "]\n";
	}

	void INotifyWatcher::stop() {
		if (!running)
			return;

		std::cerr << "Stopping.\n";
		running = false;
		// TODO
	}
}
