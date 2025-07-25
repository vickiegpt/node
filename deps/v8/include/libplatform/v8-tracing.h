// Copyright 2016 the V8 project authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8_LIBPLATFORM_V8_TRACING_H_
#define V8_LIBPLATFORM_V8_TRACING_H_

#include <atomic>
#include <fstream>
#include <memory>
#include <unordered_set>
#include <vector>

#include "libplatform/libplatform-export.h"
#include "v8-platform.h"  // NOLINT(build/include_directory)
#include "v8-tracing-base.h"  // NOLINT(build/include_directory)

namespace perfetto {
namespace trace_processor {
class TraceProcessorStorage;
}
class TracingSession;
}  // namespace perfetto

namespace v8 {

// Add typedef for V8TracingController
using V8TracingController = v8::TracingController;

namespace base {
#ifndef V8_BASE_MUTEX_FORWARD_DECLARED
#define V8_BASE_MUTEX_FORWARD_DECLARED
class Mutex;
#endif
}  // namespace base

namespace platform {
namespace tracing {

class TraceEventListener;

// TraceObject class is already defined in v8-tracing-base.h
// We need to be in the right namespace

#if 0
class V8_PLATFORM_EXPORT TraceObject {
 public:
  union ArgValue {
    uint64_t as_uint;
    int64_t as_int;
    double as_double;
    const void* as_pointer;
    const char* as_string;
  };

  TraceObject() = default;
  ~TraceObject();
  void Initialize(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags, int64_t timestamp, int64_t cpu_timestamp);
  void UpdateDuration(int64_t timestamp, int64_t cpu_timestamp);
  void InitializeForTesting(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags, int pid, int tid, int64_t ts, int64_t tts,
      uint64_t duration, uint64_t cpu_duration);

  int pid() const { return pid_; }
  int tid() const { return tid_; }
  char phase() const { return phase_; }
  const uint8_t* category_enabled_flag() const {
    return category_enabled_flag_;
  }
  const char* name() const { return name_; }
  const char* scope() const { return scope_; }
  uint64_t id() const { return id_; }
  uint64_t bind_id() const { return bind_id_; }
  int num_args() const { return num_args_; }
  const char** arg_names() { return arg_names_; }
  uint8_t* arg_types() { return arg_types_; }
  ArgValue* arg_values() { return arg_values_; }
  std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables() {
    return arg_convertables_;
  }
  unsigned int flags() const { return flags_; }
  int64_t ts() { return ts_; }
  int64_t tts() { return tts_; }
  uint64_t duration() { return duration_; }
  uint64_t cpu_duration() { return cpu_duration_; }

 private:
  int pid_;
  int tid_;
  char phase_;
  const char* name_;
  const char* scope_;
  const uint8_t* category_enabled_flag_;
  uint64_t id_;
  uint64_t bind_id_;
  int num_args_ = 0;
  const char* arg_names_[kTraceMaxNumArgs];
  uint8_t arg_types_[kTraceMaxNumArgs];
  ArgValue arg_values_[kTraceMaxNumArgs];
  std::unique_ptr<v8::ConvertableToTraceFormat>
      arg_convertables_[kTraceMaxNumArgs];
  char* parameter_copy_storage_ = nullptr;
  unsigned int flags_;
  int64_t ts_;
  int64_t tts_;
  uint64_t duration_;
  uint64_t cpu_duration_;

  // Disallow copy and assign
  TraceObject(const TraceObject&) = delete;
  void operator=(const TraceObject&) = delete;
};
#endif

#if 0
class V8_PLATFORM_EXPORT TraceWriter {
 public:
  TraceWriter() = default;
  virtual ~TraceWriter() = default;
  virtual void AppendTraceEvent(TraceObject* trace_event) = 0;
  virtual void Flush() = 0;

  static TraceWriter* CreateJSONTraceWriter(std::ostream& stream);
  static TraceWriter* CreateJSONTraceWriter(std::ostream& stream,
                                            const std::string& tag);

  static TraceWriter* CreateSystemInstrumentationTraceWriter();

 private:
  // Disallow copy and assign
  TraceWriter(const TraceWriter&) = delete;
  void operator=(const TraceWriter&) = delete;
};
#endif

#if 0
class V8_PLATFORM_EXPORT TraceBufferChunk {
 public:
  explicit TraceBufferChunk(uint32_t seq);

  void Reset(uint32_t new_seq);
  bool IsFull() const { return next_free_ == kChunkSize; }
  TraceObject* AddTraceEvent(size_t* event_index);
  TraceObject* GetEventAt(size_t index) { return &chunk_[index]; }

  uint32_t seq() const { return seq_; }
  size_t size() const { return next_free_; }

  static const size_t kChunkSize = 64;

 private:
  size_t next_free_ = 0;
  TraceObject chunk_[kChunkSize];
  uint32_t seq_;

  // Disallow copy and assign
  TraceBufferChunk(const TraceBufferChunk&) = delete;
  void operator=(const TraceBufferChunk&) = delete;
};
#endif

class V8_PLATFORM_EXPORT TraceBuffer {
 public:
  TraceBuffer() = default;
  virtual ~TraceBuffer() = default;

  virtual TraceObject* AddTraceEvent(uint64_t* handle) = 0;
  virtual TraceObject* GetEventByHandle(uint64_t handle) = 0;
  virtual bool Flush() = 0;

  static const size_t kRingBufferChunks = 1024;

  static TraceBuffer* CreateTraceBufferRingBuffer(size_t max_chunks,
                                                  TraceWriter* trace_writer);

 private:
  // Disallow copy and assign
  TraceBuffer(const TraceBuffer&) = delete;
  void operator=(const TraceBuffer&) = delete;
};

// Options determines how the trace buffer stores data.
enum TraceRecordMode {
  // Record until the trace buffer is full.
  RECORD_UNTIL_FULL,

  // Record until the user ends the trace. The trace buffer is a fixed size
  // and we use it as a ring buffer during recording.
  RECORD_CONTINUOUSLY,

  // Record until the trace buffer is full, but with a huge buffer size.
  RECORD_AS_MUCH_AS_POSSIBLE,

  // Echo to console. Events are discarded.
  ECHO_TO_CONSOLE,
};

class V8_PLATFORM_EXPORT TraceConfig {
 public:
  typedef std::vector<std::string> StringList;

  static TraceConfig* CreateDefaultTraceConfig();

  TraceConfig() : enable_systrace_(false), enable_argument_filter_(false) {}
  TraceRecordMode GetTraceRecordMode() const { return record_mode_; }
  const StringList& GetEnabledCategories() const {
    return included_categories_;
  }
  bool IsSystraceEnabled() const { return enable_systrace_; }
  bool IsArgumentFilterEnabled() const { return enable_argument_filter_; }

  void SetTraceRecordMode(TraceRecordMode mode) { record_mode_ = mode; }
  void EnableSystrace() { enable_systrace_ = true; }
  void EnableArgumentFilter() { enable_argument_filter_ = true; }

  void AddIncludedCategory(const char* included_category);

  bool IsCategoryGroupEnabled(const uint8_t* category_group_enabled) const;

 private:
  TraceRecordMode record_mode_;
  bool enable_systrace_ : 1;
  bool enable_argument_filter_ : 1;
  StringList included_categories_;

  // Disallow copy and assign
  TraceConfig(const TraceConfig&) = delete;
  void operator=(const TraceConfig&) = delete;
};

#if defined(V8_USE_PERFETTO)
class V8_PLATFORM_EXPORT TracingController
    : public V8TracingController,
      public perfetto::TrackEventSessionObserver {
#else
class V8_PLATFORM_EXPORT TracingController : public V8TracingController {
#endif
 public:
  TracingController();
  ~TracingController() override;

#if defined(V8_USE_PERFETTO)
  // perfetto::TrackEventSessionObserver
  void OnSetup(const perfetto::DataSourceBase::SetupArgs&) override;
  void OnStart(const perfetto::DataSourceBase::StartArgs&) override;
  void OnStop(const perfetto::DataSourceBase::StopArgs&) override;
#endif

  // Takes ownership of |trace_buffer|.
  void Initialize(TraceBuffer* trace_buffer);

  // v8::TracingController implementation.
  const uint8_t* GetCategoryGroupEnabled(const char* category_group) override;
  uint64_t AddTraceEvent(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags) override;
  uint64_t AddTraceEventWithTimestamp(
      char phase, const uint8_t* category_enabled_flag, const char* name,
      const char* scope, uint64_t id, uint64_t bind_id, int32_t num_args,
      const char** arg_names, const uint8_t* arg_types,
      const uint64_t* arg_values,
      std::unique_ptr<v8::ConvertableToTraceFormat>* arg_convertables,
      unsigned int flags, int64_t timestamp) override;
  void UpdateTraceEventDuration(const uint8_t* category_enabled_flag,
                                const char* name, uint64_t handle) override;

  void AddTraceStateObserver(
      v8::TracingController::TraceStateObserver* observer) override;
  void RemoveTraceStateObserver(
      v8::TracingController::TraceStateObserver* observer) override;

  void StartTracing(TraceConfig* trace_config);
  void StopTracing();

  static const char* GetCategoryGroupName(const uint8_t* category_enabled_flag);

 protected:
  virtual int64_t CurrentTimestampMicroseconds();
  virtual int64_t CurrentCpuTimestampMicroseconds();

 private:
  void UpdateCategoryState();

  std::unique_ptr<TraceBuffer> trace_buffer_;
  std::unique_ptr<TraceConfig> trace_config_;
  std::unique_ptr<base::Mutex> mutex_;
  std::unordered_set<v8::TracingController::TraceStateObserver*> observers_;
  std::atomic_bool recording_{false};
#if defined(V8_USE_PERFETTO)
  std::atomic_bool perfetto_recording_{false};
#endif

  // Disallow copy and assign
  TracingController(const TracingController&) = delete;
  void operator=(const TracingController&) = delete;
};

}  // namespace tracing
}  // namespace platform
}  // namespace v8

#endif  // V8_LIBPLATFORM_V8_TRACING_H_
