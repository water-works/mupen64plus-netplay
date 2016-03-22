#ifndef INPUT_QUEUE_H_
#define INPUT_QUEUE_H_

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <ratio>

// Blocking queue object that will be used to communicate button values received
// from the server to the client. This queue is designed to receive and emit
// values by frame and to handle delayed input value reading.
template <typename ButtonsType>
class InputQueue {
 public:
  // Make an input queue suitable for use recording inputs from a local input
  // source. In particular, the returned queue applies a delay of delay_frames
  // to all inputs passed to PutButtons.
  static InputQueue* MakeLocalQueue(int delay_frames);

  // Make an input queue suitable for use recording inputs from a remote input
  // source. In particular, the returned queue applies no additional delay to
  // the inputs passed to PutButtons. The queue assumes the inputs were already
  // adjusted for delay by the client that sent them.
  static InputQueue* MakeRemoteQueue(int delay_frames);

  // Record the given button presses in the queue. The buttons will be available
  // to GetButtons at frame (frame + delay_frames_).
  //
  // This method returns true on success and false when:
  //  - frame is greater than one plus
  bool PutButtons(int frame, const ButtonsType& buttons);

  // Get the buttons associated with the given frame, accounting for delay. If
  // frame is less than delay_frames_, return a default-constructed ButtonsType.
  // Arguments are:
  //  - frame: delay-adjusted frame number
  //  - timeout_micros: number of microseconds to wait.
  //  - buttons: output pointer. Only populated if this method returns true. If
  //    frame is less than the initial frame delay, populated using the default
  //    constructor for ButtonsType.
  // Returns true and populates *buttons on success, returns false otherwise.
  static const int kBlockForever;
  static const int kReturnImmediately;
  enum class GetButtonsStatus {
    SUCCESS = 0,
    UNEXPECTED_FRAME,
    TIMEOUT
  };
  GetButtonsStatus GetButtons(int frame, int timeout_micros,
                              ButtonsType* buttons);

  // Get the number of frames of button data waiting to be read.
  size_t QueueSize();

  // Get the number of delay frames for this queue.
  int delay_frames() const { return delay_frames_; }

 private:
  typedef std::unique_lock<std::mutex> UniqueLock;
  typedef std::lock_guard<std::mutex> LockGuard;
  typedef std::chrono::duration<int, std::micro> Microseconds;

  InputQueue(int delay_frames, int initial_frame_delay);

  const int delay_frames_;
  const int initial_frame_delay_;

  // Mutable state

  // The latest frame for which button data has been requested. All attempts to
  // put buttons with frame less than this will fail.
  int latest_frame_requested_;
  // Map from frame number to buttons for that frame, sorted in order from least
  // to greatest frame number.
  std::map<int, ButtonsType> frame_buttons_;

  // These protect latest_frame_requested_ and frame_buttons_.
  std::mutex m_;
  std::condition_variable cv_;
};

#include "input-queue.hpp"

#endif  // INPUT_QUEUE_H
