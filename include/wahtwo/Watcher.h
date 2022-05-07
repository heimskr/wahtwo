#pragma once

#include <cerrno>
#include <string.h>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace Wahtwo {
	class WatcherBase {
		public:
			using Callback = std::function<void(const std::filesystem::path &)>;
			using MultiCallback = std::function<void(const std::filesystem::path &, const std::filesystem::path &)>;
			using Filter = std::function<bool(const std::filesystem::path &)>;

			Filter filter;
			MultiCallback onCreate;
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
				errorString = strerror_l(errno, uselocale(locale_t(0)));
				description = what_ + ": " + errorString;
			}

			int error;

		private:
			std::string description;
			char *errorString = nullptr;

			const char * what() const throw() {
				return description.c_str();
			}
	};
}

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>


namespace Wahtwo {
	void fsew_callback(ConstFSEventStreamRef, void *, size_t, void *, const FSEventStreamEventFlags *,
	                   const FSEventStreamEventId *);

	class FSEventsWatcher: public WatcherBase {
		public:
			FSEventsWatcher(const std::vector<std::string> &paths, bool subfiles = true);
			~FSEventsWatcher() override;

			void start() override;
			void stop() override;
			bool isRunning() const override { return running; }
			const std::vector<std::string> & getPaths() override { return paths; }

		private:
			bool subfiles;
			bool running = false;
			std::vector<std::string> paths;
			CFArrayRef cfPaths = nullptr;
			FSEventStreamRef stream = nullptr;
			std::unique_ptr<FSEventStreamContext> context;

			void setPaths(const std::vector<std::string> &);
			void callback(size_t num_events, const char **event_paths, const FSEventStreamEventFlags *flags,
			              const FSEventStreamEventId *ids);

			friend void fsew_callback(ConstFSEventStreamRef, void *, size_t, void *, const FSEventStreamEventFlags *,
			                          const FSEventStreamEventId *);
	};

	using Watcher = FSEventsWatcher;
}
#elif defined(__linux__)
#include <sys/inotify.h>

namespace Wahtwo {
	class INotifyWatcher: public WatcherBase {
		public:
			INotifyWatcher(const std::vector<std::string> &paths, bool = true);
			~INotifyWatcher() override;

			void start() override;
			void stop() override;
			bool isRunning() const override { return running; }
			const std::vector<std::string> & getPaths() override { return paths; }

		private:
			bool running = false;
			std::vector<std::string> paths;
			int fd;
			std::map<int, std::string> watchDescriptors;
	};

	using Watcher = INotifyWatcher;
}
#endif
