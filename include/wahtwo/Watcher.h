#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Wahtwo {
	class WatcherBase {
		public:
			virtual ~WatcherBase() = default;

			virtual void start() = 0;
			virtual void stop() = 0;
			virtual bool isRunning() const = 0;

			virtual const std::vector<std::string> & getPaths() = 0;

		protected:
			WatcherBase() = default;
	};
}

#ifdef __APPLE__
#include <CoreServices/CoreServices.h>


namespace Wahtwo {
	void fsew_callback(ConstFSEventStreamRef, void *, size_t, void *, const FSEventStreamEventFlags *,
	                   const FSEventStreamEventId *);

	class FSEventsWatcher: public WatcherBase {
		public:
			FSEventsWatcher(const std::vector<std::string> &paths);
			~FSEventsWatcher() override;

			void start() override;
			void stop() override;
			bool isRunning() const override { return running; }
			const std::vector<std::string> & getPaths() override { return paths; }

		private:
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
namespace Wahtwo {
	using Watcher = INotifyWatcher;
}
#endif
