#pragma once

#include <cerrno>
#include <string.h>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace Wahtwo {
	class WatcherBase {
		public:
			using Callback = std::function<void(const std::filesystem::path &)>;
			using MultiCallback = std::function<void(const std::filesystem::path &, const std::filesystem::path &)>;
			using Filter = std::function<bool(const std::filesystem::path &)>;

			Filter filter;
			Callback onCreate;
			Callback onRemoveSelf;
			MultiCallback onRemoveChild;
			Callback onRenameSelf;
			MultiCallback onRenameChild;
			Callback onModify;
			Callback onAttributes;
			/** FSEvents (macOS) only. */
			Callback onClone;
			Callback onAny;

			virtual ~WatcherBase() = default;

			virtual void start() = 0;
			virtual void stop() = 0;
			virtual bool isRunning() const = 0;
			virtual const std::vector<std::string> & getPaths() = 0;

		protected:
			WatcherBase() = default;
	};

	class Error: public std::runtime_error {
		public:
			Error(const std::string &what_, int error_): std::runtime_error(what_), error(error_) {
#ifdef __linux__
				errorString = strerror_l(error, uselocale(locale_t(0)));
#else
				errorString = strerror(error);
#endif
				description = what_ + ": " + errorString;
			}

			int error = -1;

			const char * what() const noexcept final {
				return description.c_str();
			}

		private:
			std::string description;
			char *errorString = nullptr;
	};
}

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>

namespace Wahtwo {
	void fsew_callback(ConstFSEventStreamRef, void *, size_t, void *, const FSEventStreamEventFlags *,
	                   const FSEventStreamEventId *);

	class FSEventsWatcher: public WatcherBase {
		public:
			FSEventsWatcher(std::vector<std::string> paths, bool subfiles = true);
			~FSEventsWatcher() override;

			void start() override;
			void stop() override;
			bool isRunning() const override { return running; }
			const std::vector<std::string> & getPaths() override { return pathStrings; }

		private:
			bool subfiles;
			bool running = false;
			std::vector<std::string> pathStrings;
			std::set<std::filesystem::path> paths;
			CFArrayRef cfPaths = nullptr;
			FSEventStreamRef stream = nullptr;
			std::unique_ptr<FSEventStreamContext> context;

			void setPaths(std::vector<std::string>);
			void callback(size_t num_events, const char **event_paths, const FSEventStreamEventFlags *flags, const FSEventStreamEventId *ids);

			friend void fsew_callback(ConstFSEventStreamRef, void *, size_t, void *, const FSEventStreamEventFlags *, const FSEventStreamEventId *);
	};

	using Watcher = FSEventsWatcher;
}
#elif defined(__linux__)
#include <sys/inotify.h>

namespace Wahtwo {
	class INotifyWatcher: public WatcherBase {
		public:
			INotifyWatcher(std::vector<std::string> paths, bool subfiles = true);
			~INotifyWatcher() override;

			void start() override;
			void stop() override;
			bool isRunning() const override { return running; }
			const std::vector<std::string> & getPaths() override { return paths; }

		private:
			constexpr static uint32_t MASK = IN_MODIFY | IN_MOVE_SELF | IN_ATTRIB | IN_CREATE | IN_DELETE_SELF | IN_MOVE | IN_DELETE;
			std::vector<std::string> paths;
			std::map<int, std::string> watchPaths;
			std::map<std::string, int> watchDescriptors;
			int controlPipe[2] {-1, -1};
			int fd = -1;
			bool subfiles = true;
			bool running = false;

			void addRecursive(const std::filesystem::path &, bool add_watch = true);
			void addWatch(const std::string &path);
	};

	using Watcher = INotifyWatcher;
}
#endif
