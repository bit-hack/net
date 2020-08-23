#pragma once
#include <cassert>
#include <mutex>
#include <thread>

struct thread_t {

  thread_t() : _active(false), _thread() {}

  // spin up a new thread to execute the run method
  void start() {
    if (!_thread) {
      _active = true;
      _thread.reset(new std::thread(trampoline, this));
    }
  }

  // halt an active thread and release it
  void stop() {
    if (_thread) {
      _active = false;
      if (_thread->joinable())
        _thread->join();
      _thread.reset();
    }
  }

  bool active() const {
    return _active;
  }

  virtual void run() = 0;

protected:
  bool _active;
  std::unique_ptr<std::thread> _thread;

  static void trampoline(thread_t *self) {
    assert(self);
    while (self->active()) {
      self->run();
    }
  }
};
