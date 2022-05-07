#include <codecvt>
#include <iostream>
#include <string>
#include <vector>

#include <CoreServices/CoreServices.h>

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

CFArrayRef getArrayRef(const std::vector<std::string> &strings) {
	std::vector<CFStringRef> string_refs;
	string_refs.reserve(strings.size());
	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;

	for (const auto &string: strings) {
		auto wide = converter.from_bytes(string);
		string_refs.push_back(CFStringCreateWithCharacters(kCFAllocatorDefault,
			reinterpret_cast<const UniChar *>(wide.data()), wide.size()));
	}

	return CFArrayCreate(nullptr, (const void **) string_refs.data(), string_refs.size(), nullptr);
}

int main() {
	std::vector<std::string> paths {"./src", "./include"};
	auto stream = FSEventStreamCreate(kCFAllocatorDefault, callback, nullptr, getArrayRef(paths),
		// kFSEventStreamEventIdSinceNow, 0.5, kFSEventStreamCreateFlagNone);
		kFSEventStreamEventIdSinceNow, 0.5, kFSEventStreamCreateFlagFileEvents);
	FSEventStreamScheduleWithRunLoop(stream, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
	FSEventStreamStart(stream);
	CFRunLoopRun();
}

