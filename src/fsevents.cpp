#include <codecvt>
#include <iostream>
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
	void fsew_callback(ConstFSEventStreamRef stream_ref, void *callback_info, size_t num_events, void *event_paths,
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
	}

	void FSEventsWatcher::stop() {
		if (!running)
			return;

		running = false;
		if (stream != nullptr) {
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

	void FSEventsWatcher::callback(size_t num_events, const char **event_paths, const FSEventStreamEventFlags *flags,
	                               const FSEventStreamEventId *ids) {

	}
}


/*
static void callback(ConstFSEventStreamRef stream_ref,
					 void *callback_info,
					 size_t num_events,
					 void *event_paths,
					 const FSEventStreamEventFlags event_flags[],
					 const FSEventStreamEventId event_ids[]) {
	const char **paths = reinterpret_cast<const char **>(event_paths);
	std::cout << "num_events: " << num_events << "\n";
	for (size_t i = 0; i < num_events; ++i) {
		auto flags = event_flags[i];
		std::cout << i << ": \"" << paths[i] << "\" " << flags << ' ' << event_ids[i] << '\n';
		if (flags & kFSEventStreamEventFlagItemCreated)
			std::cout << "    created\n";
		if (flags & kFSEventStreamEventFlagItemRemoved)
			std::cout << "    removed\n";
		if (flags & kFSEventStreamEventFlagItemRenamed)
			std::cout << "    renamed\n";
		if (flags & kFSEventStreamEventFlagItemModified)
			std::cout << "    modified\n";
		if (flags & kFSEventStreamEventFlagItemChangeOwner)
			std::cout << "    owner changed\n";
		if (flags & kFSEventStreamEventFlagItemCloned)
			std::cout << "    cloned\n";
	}
}

int main() {
	std::vector<std::string> paths {"./src", "./include"};
	auto stream = FSEventStreamCreate(kCFAllocatorDefault, callback, nullptr, getArrayRef(paths),
		kFSEventStreamEventIdSinceNow, 0.5, kFSEventStreamCreateFlagFileEvents);
	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	FSEventStreamStart(stream);
	CFRunLoopRun();
}

//*/