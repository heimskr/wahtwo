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

namespace Wahtwo {
	void fsew_callback(ConstFSEventStreamRef, void *callback_info, size_t num_events, void *event_paths,
	                   const FSEventStreamEventFlags event_flags[], const FSEventStreamEventId event_ids[]) {
		reinterpret_cast<Wahtwo::FSEventsWatcher *>(callback_info)->callback(num_events,
			reinterpret_cast<const char **>(event_paths), event_flags, event_ids);
	}

	FSEventsWatcher::FSEventsWatcher(const std::vector<std::string> &paths_) {
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
		stream = FSEventStreamCreate(kCFAllocatorDefault, fsew_callback, context.get(), getArrayRef(paths),
			kFSEventStreamEventIdSinceNow, 0.5, kFSEventStreamCreateFlagFileEvents);
		worker = std::thread([this] {
			FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
			FSEventStreamStart(stream);
			CFRunLoopRun();
		});
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
		cfPaths = getArrayRef(paths = paths_);
	}

	void FSEventsWatcher::callback(size_t num_events, const char **paths, const FSEventStreamEventFlags *event_flags,
	                               const FSEventStreamEventId *event_ids) {
		for (size_t i = 0; i < num_events; ++i) {
			std::filesystem::path path(paths[i]);
			if (!filter || filter(path)) {
				const auto flags = event_flags[i];
				const auto ids = event_ids[i];
				const bool removed = (flags & kFSEventStreamEventFlagItemRemoved) != 0;
				if (!removed && (flags & kFSEventStreamEventFlagItemCreated) != 0 && onCreate)
					onCreate(path);
				if (removed && onRemove)
					onRemove(path);
				if ((flags & kFSEventStreamEventFlagItemRenamed) != 0 && onRename)
					onRename(path);
				if (!removed && (flags & kFSEventStreamEventFlagItemModified) != 0 && onModify)
					onModify(path);
				if ((flags & kFSEventStreamEventFlagItemChangeOwner) != 0 && onOwnerChange)
					onOwnerChange(path);
				if ((flags & kFSEventStreamEventFlagItemCloned) != 0 && onClone)
					onClone(path);
			}
		}
	}
}
