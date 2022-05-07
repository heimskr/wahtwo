#include <codecvt>
#include <stdexcept>
#include <string>
#include <vector>

#include "wahtwo/Watcher.h"

static CFArrayRef getArrayRef(const std::vector<std::string> &strings) {
	std::vector<CFStringRef> string_refs;
	string_refs.reserve(strings.size());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;

	for (const auto &string: strings) {
		auto wide = converter.from_bytes(string);
		string_refs.push_back(CFStringCreateWithCharacters(kCFAllocatorDefault,
			reinterpret_cast<const UniChar *>(wide.data()), wide.size()));
	}

	return CFArrayCreate(nullptr, reinterpret_cast<const void **>(string_refs.data()), string_refs.size(), nullptr);
}

static bool is_subpath(const std::filesystem::path &basepath, const std::filesystem::path &subpath) {
	// https://stackoverflow.com/a/70888233/227663
	for (auto base = basepath.begin(), sub = subpath.begin(), end = basepath.end(), subend = subpath.end();
		 base != end; ++base, ++sub)
		if (sub == subend || *base != *sub)
			return false;
	return true;
}

namespace Wahtwo {
	void fsew_callback(ConstFSEventStreamRef, void *callback_info, size_t num_events, void *event_paths,
	                   const FSEventStreamEventFlags event_flags[], const FSEventStreamEventId event_ids[]) {
		reinterpret_cast<Wahtwo::FSEventsWatcher *>(callback_info)->callback(num_events,
			reinterpret_cast<const char **>(event_paths), event_flags, event_ids);
	}

	FSEventsWatcher::FSEventsWatcher(const std::vector<std::string> &paths_, bool subfiles_): subfiles(subfiles_) {
		setPaths(paths_);
	}

	FSEventsWatcher::~FSEventsWatcher() {
		if (running)
			stop();

		if (cfPaths != nullptr)
			CFRelease(cfPaths);
	}

	void FSEventsWatcher::start() {
		if (running || stream != nullptr)
			throw std::runtime_error("Can't start FSEventsWatcher: already running");
		running = true;
		context = std::unique_ptr<FSEventStreamContext>(new FSEventStreamContext {0, this, nullptr, nullptr, nullptr});
		stream = FSEventStreamCreate(kCFAllocatorDefault, fsew_callback, context.get(), getArrayRef(pathStrings),
			kFSEventStreamEventIdSinceNow, 0.5,
			subfiles? kFSEventStreamCreateFlagFileEvents : kFSEventStreamCreateFlagNone);
		FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
		FSEventStreamStart(stream);
		CFRunLoopRun();
	}

	void FSEventsWatcher::stop() {
		if (!running)
			return;

		running = false;
		if (stream != nullptr) {
			FSEventStreamUnscheduleFromRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			FSEventStreamInvalidate(stream);
			FSEventStreamRelease(stream);
			stream = nullptr;
		} else
			throw std::runtime_error("Can't stop FSEventsWatcher: stream is null");
	}

	void FSEventsWatcher::setPaths(const std::vector<std::string> &paths_) {
		if (cfPaths != nullptr)
			CFRelease(cfPaths);
		cfPaths = getArrayRef(pathStrings = paths_);
		paths.clear();
		for (const auto &string: pathStrings)
			try {
				paths.insert(std::filesystem::canonical(string));
			} catch (const std::filesystem::filesystem_error &) {
				paths.insert(string);
			}
	}

	void FSEventsWatcher::callback(size_t num_events, const char **event_paths,
	                               const FSEventStreamEventFlags *event_flags, const FSEventStreamEventId *) {
		for (size_t i = 0; i < num_events; ++i) {
			std::filesystem::path path(event_paths[i]);
			if (!filter || filter(path)) {
				const auto flags = event_flags[i];
				const bool removed = (flags & kFSEventStreamEventFlagItemRemoved) != 0;
				if (!removed && (flags & kFSEventStreamEventFlagItemCreated) != 0 && onCreate)
					onCreate(path);
				if (removed) {
					// This is so convoluted. inotify is better.
					if (paths.contains(path)) {
						if (onRemoveSelf)
							onRemoveSelf(path);
					} else if (onRemoveChild)
						for (const auto &watched: paths)
							if (is_subpath(watched, path)) {
								onRemoveChild(watched, path);
								break;
							}
				}
				if ((flags & kFSEventStreamEventFlagItemRenamed) != 0) {
					if (paths.contains(path)) {
						if (onRenameSelf)
							onRenameSelf(path);
					} else if (onRenameChild)
						for (const auto &watched: paths)
							if (is_subpath(watched, path)) {
								onRenameChild(watched, path);
								break;
							}
				}
				if (!removed && (flags & kFSEventStreamEventFlagItemModified) != 0 && onModify)
					onModify(path);
				if ((flags & kFSEventStreamEventFlagItemChangeOwner) != 0 && onAttributes)
					onAttributes(path);
				if ((flags & kFSEventStreamEventFlagItemCloned) != 0 && onClone)
					onClone(path);
				if (onAny)
					onAny(path);
			}
		}
	}
}
