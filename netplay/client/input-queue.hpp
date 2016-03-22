// included by input-queue.h

#include <iostream>

#include "glog/logging.h"

// -----------------------------------------------------------------------------
// InputQueue

template <typename ButtonsType>
const int InputQueue<ButtonsType>::kBlockForever = -1;

template <typename ButtonsType>
const int InputQueue<ButtonsType>::kReturnImmediately = 0;

template <typename ButtonsType>
InputQueue<ButtonsType>::InputQueue(int delay_frames, int initial_frame_delay)
    : delay_frames_(delay_frames),
      initial_frame_delay_(initial_frame_delay),
      latest_frame_requested_(-1) {}

// static
template <typename ButtonsType>
InputQueue<ButtonsType>* InputQueue<ButtonsType>::MakeLocalQueue(
    int delay_frames) {
  return new InputQueue<ButtonsType>(delay_frames, delay_frames);
}

// static
template <typename ButtonsType>
InputQueue<ButtonsType>* InputQueue<ButtonsType>::MakeRemoteQueue(
    int delay_frames) {
  return new InputQueue<ButtonsType>(0, delay_frames);
}

template <typename ButtonsType>
bool InputQueue<ButtonsType>::PutButtons(int frame,
                                         const ButtonsType& buttons) {
  if (frame < 0) {
    LOG(ERROR) << "PutButtons: Attempted to put buttons into invalid frame "
               << frame;
    return false;
  }

  const int delayed_frame = frame + delay_frames_;

  if (delayed_frame < initial_frame_delay_) {
    LOG(ERROR) << "PutButtons: Attempted to put buttons for delayed frame "
               << delayed_frame
               << ", which is less than the initial delay period of "
               << initial_frame_delay_;
    return false;
  }

  // Implement insertion while holding m_.
  {
    LockGuard guard(m_);
    // We now hold a lock on latest_frame_requested_ and frame_buttons_.

    // Reject all button data for frames that we've already read.
    if (delayed_frame <= latest_frame_requested_) {
      LOG(ERROR)
          << "PutButtons: Attempted to put buttons for delayed frame "
          << delayed_frame
          << ", which has already been requested. The latest frame requested "
             "is "
          << latest_frame_requested_;
      return false;
    }

    // Reject button data for frames we already have,
    if (frame_buttons_.find(delayed_frame) != frame_buttons_.end()) {
      LOG(ERROR) << "PutButtons: Attempted to put duplicate buttons for "
                    "delayed frame "
                 << delayed_frame;
      return false;
    }

    // Insert the button data.
    frame_buttons_.insert({delayed_frame, buttons});
  }

  cv_.notify_all();

  return true;
}

template <typename ButtonsType>
typename InputQueue<ButtonsType>::GetButtonsStatus
InputQueue<ButtonsType>::GetButtons(int frame, int timeout_micros,
                                    ButtonsType* buttons) {
  VLOG(3) << "Requesting buttons for frame " << frame << " with a timeout of "
          << static_cast<double>(timeout_micros) / 1000000 << " seconds";

  // Perform the implementation while holding m_.
  UniqueLock lock(m_);
  // We now hold a lock on latest_frame_requested_ and frame_buttons_.

  // Enforce that frames come in one at a time and in order.
  const int expected_frame = latest_frame_requested_ + 1;
  if (frame != expected_frame) {
    LOG(ERROR)
        << "Attempted to get buttons from unexpected frame. Requested frame "
        << frame << " and expected frame " << expected_frame;
    return GetButtonsStatus::UNEXPECTED_FRAME;
  }

  // If we're requesting buttons from a frame earlier than this queue's delay,
  // emit a default-constructed ButtonsType object.
  if (frame < initial_frame_delay_) {
    latest_frame_requested_ = frame;
    *buttons = ButtonsType();
    return GetButtonsStatus::SUCCESS;
  }

  // Wait for requested frame to appear.
  const auto have_buttons_for_frame = [this, frame] {
    return frame_buttons_.find(frame) != frame_buttons_.end();
  };
  if (timeout_micros == kBlockForever) {
    cv_.wait(lock, have_buttons_for_frame);
  } else {
    Microseconds timeout(timeout_micros);
    if (!cv_.wait_for(lock, timeout, have_buttons_for_frame)) {
      if (timeout_micros > 0) {
        LOG(ERROR) << "Timed out waiting for buttons. Timeout is "
                   << static_cast<double>(timeout_micros) / 1000000
                   << " seconds.";
      }
      return GetButtonsStatus::TIMEOUT;
    }
  }
  // We now hold a lock on latest_frame_requested_ and frame_buttons_.

  *buttons = frame_buttons_.find(frame)->second;
  frame_buttons_.erase(frame);
  latest_frame_requested_ = frame;

  return GetButtonsStatus::SUCCESS;
}

template <typename ButtonsType>
size_t InputQueue<ButtonsType>::QueueSize() {
  LockGuard lock(m_);
  // We now have a lock on frame_buttons_
  return frame_buttons_.size();
}
