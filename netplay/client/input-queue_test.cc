#include "input-queue.h"

#include <algorithm>
#include <sstream>
#include <thread>
#include <vector>

#include "client/test-utils.h"

#include "gtest/gtest.h"

using std::string;
using std::stringstream;
using std::vector;

// -----------------------------------------------------------------------------
// InputQueue

class InputQueueTest : public ::testing::Test {
 protected:
  typedef InputQueue<string> StringQueue;

  InputQueueTest()
      : local_queue_(StringQueue::MakeLocalQueue(kDelayFrames)),
        remote_queue_(StringQueue::MakeRemoteQueue(kDelayFrames)) {
    TRACED_CALL(SkipFrames(kDelayFrames));
  }

  // Produces num_frames into the queue in random order. Frame data is a string
  // of the format "frame <frame number>"
  void ProduceFrames(int num_frames, bool* success) {
    // Shuffle the input frames
    vector<int> frames;
    for (int i = 0; i < num_frames; ++i) {
      frames.push_back(i);
    }
    std::random_shuffle(frames.begin(), frames.end());

    // Produce frames
    for (const int frame_num : frames) {
      stringstream delayed_frame;
      delayed_frame << "frame " << frame_num + kDelayFrames;
      if (!local_queue_->PutButtons(frame_num, delayed_frame.str())) {
        *success = false;
      }

      // Don't insert frame data for frames earlier than the initial delay
      // period into the remote queue.
      if (frame_num >= kDelayFrames) {
        stringstream undelayed_frame;
        undelayed_frame << "frame " << frame_num;
        if (!remote_queue_->PutButtons(frame_num, undelayed_frame.str())) {
          *success = false;
        }
      }
    }
    *success = true;
  }

  void ConsumeFrames(int num_frames, bool* success) {
    // Consume frames
    for (int i = kDelayFrames; i < num_frames + kDelayFrames; ++i) {
      // Local queue
      string frame;
      if (local_queue_->GetButtons(i, StringQueue::kBlockForever, &frame) !=
          StringQueue::GetButtonsStatus::SUCCESS) {
        *success = false;
        return;
      }
      stringstream ss_local;
      ss_local << "frame " << i;
      EXPECT_EQ(ss_local.str(), frame);

      // Remote queue
      const int undelayed_frame_num = i - kDelayFrames;
      if (undelayed_frame_num >= kDelayFrames) {
        if (remote_queue_->GetButtons(undelayed_frame_num,
                                      StringQueue::kBlockForever, &frame) !=
            StringQueue::GetButtonsStatus::SUCCESS) {
          *success = false;
          return;
        }
        stringstream ss_remote;
        ss_remote << "frame " << undelayed_frame_num;
        EXPECT_EQ(ss_remote.str(), frame);
      }
    }
    *success = true;
  }

  void SkipFrames(int skipped_frames) {
    for (int i = 0; i < skipped_frames; ++i) {
      string frame = "default not empty";
      ASSERT_EQ(
          StringQueue::GetButtonsStatus::SUCCESS,
          local_queue_->GetButtons(i, StringQueue::kBlockForever, &frame));
      EXPECT_EQ("", frame);

      frame = "default not empty";
      ASSERT_EQ(
          StringQueue::GetButtonsStatus::SUCCESS,
          remote_queue_->GetButtons(i, StringQueue::kBlockForever, &frame));
      EXPECT_EQ("", frame);
    }
  }

  static const int kDelayFrames;
  std::unique_ptr<StringQueue> local_queue_;
  std::unique_ptr<StringQueue> remote_queue_;
};

const int InputQueueTest::kDelayFrames = 5;

TEST_F(InputQueueTest, DelayFramesOnlyAppliedToRemoteQueues) {
  EXPECT_EQ(kDelayFrames, local_queue_->delay_frames());
  EXPECT_EQ(0, remote_queue_->delay_frames());
}

TEST_F(InputQueueTest, PutButtonsIntoEmptyQueueInterleaved) {
  string frame_data;

  // Local queue
  ASSERT_TRUE(local_queue_->PutButtons(0, "frame 0"));
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(5, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 0", frame_data);

  ASSERT_TRUE(local_queue_->PutButtons(1, "frame 1"));
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(6, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 1", frame_data);

  // Remote queue
  ASSERT_TRUE(remote_queue_->PutButtons(5, "frame 0"));
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      remote_queue_->GetButtons(5, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 0", frame_data);

  ASSERT_TRUE(remote_queue_->PutButtons(6, "frame 1"));
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      remote_queue_->GetButtons(6, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 1", frame_data);
}

TEST_F(InputQueueTest, PutButtonsIntoEmptyQueueBuffered) {
  string frame_data;

  // Local queue
  ASSERT_TRUE(local_queue_->PutButtons(0, "frame 0"));
  ASSERT_TRUE(local_queue_->PutButtons(2, "frame 2"));
  ASSERT_TRUE(local_queue_->PutButtons(1, "frame 1"));

  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(5, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 0", frame_data);
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(6, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 1", frame_data);
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(7, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 2", frame_data);

  // Remote queue
  ASSERT_TRUE(remote_queue_->PutButtons(5, "frame 0"));
  ASSERT_TRUE(remote_queue_->PutButtons(7, "frame 2"));
  ASSERT_TRUE(remote_queue_->PutButtons(6, "frame 1"));

  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      remote_queue_->GetButtons(5, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 0", frame_data);
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      remote_queue_->GetButtons(6, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 1", frame_data);
  ASSERT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      remote_queue_->GetButtons(7, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ("frame 2", frame_data);
}

TEST_F(InputQueueTest, GetButtonsUnexpectedFrame) {
  string frame_data;

  ASSERT_TRUE(local_queue_->PutButtons(0, "frame 0"));
  EXPECT_EQ(
      StringQueue::GetButtonsStatus::UNEXPECTED_FRAME,
      local_queue_->GetButtons(4, StringQueue::kBlockForever, &frame_data));
  EXPECT_EQ(
      StringQueue::GetButtonsStatus::UNEXPECTED_FRAME,
      local_queue_->GetButtons(6, StringQueue::kBlockForever, &frame_data));

  EXPECT_EQ(
      StringQueue::GetButtonsStatus::SUCCESS,
      local_queue_->GetButtons(5, StringQueue::kBlockForever, &frame_data));
}

TEST_F(InputQueueTest, PutButtonsDuplicateFrame) {
  // Local queue
  ASSERT_TRUE(local_queue_->PutButtons(0, "frame 0"));
  EXPECT_FALSE(local_queue_->PutButtons(0, "frame 0"));

  // Remote queue
  ASSERT_TRUE(remote_queue_->PutButtons(5, "frame 0"));
  EXPECT_FALSE(remote_queue_->PutButtons(5, "frame 0"));
}

TEST_F(InputQueueTest, RemoteQueuePutButtonsBeforeInitialDelayTime) {
  std::unique_ptr<StringQueue> queue(
      StringQueue::MakeRemoteQueue(kDelayFrames));

  for (int i = 0; i < kDelayFrames; ++i) {
    EXPECT_FALSE(queue->PutButtons(i, "frame"));
  }
}

TEST_F(InputQueueTest, GetButtonsWithTimeout) {
  string frame_data;
  ASSERT_EQ(StringQueue::GetButtonsStatus::TIMEOUT,
            local_queue_->GetButtons(5, 1E6 / 10, &frame_data));
}

TEST_F(InputQueueTest, GetButtonsWithZeroTimeout) {
  string frame_data;

  ASSERT_TRUE(local_queue_->PutButtons(0, "frame 0"));
  ASSERT_EQ(StringQueue::GetButtonsStatus::SUCCESS,
            local_queue_->GetButtons(5, 0, &frame_data));
  EXPECT_EQ("frame 0", frame_data);
}

TEST_F(InputQueueTest, ThreadingTortureTest) {
  typedef std::remove_reference<decltype(*this)>::type TestType;

  // Simulate two minutes worth of frame data input, assuming a 60pfs input rate
  const int kNumFrames = 60 * 120;
  bool producer_success = false, consumer_success = false;
  std::thread producer(&TestType::ProduceFrames, this, kNumFrames,
                       &producer_success);
  std::thread consumer(&TestType::ConsumeFrames, this, kNumFrames,
                       &consumer_success);

  producer.join();
  consumer.join();

  EXPECT_TRUE(producer_success);
  EXPECT_TRUE(consumer_success);
}

TEST_F(InputQueueTest, EmptyQueueSize) {
  std::unique_ptr<StringQueue> input_queue(
      StringQueue::MakeLocalQueue(kDelayFrames));
  EXPECT_EQ(0, input_queue->QueueSize());
}
