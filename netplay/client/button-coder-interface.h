#ifndef BUTTON_CODER_INTERFACE_H_
#define BUTTON_CODER_INTERFACE_H_

#include "base/netplayServiceProto.pb.h"

// Abstract interface for encoding and decoding buttons to and from a KeyStatePB
// object.
template <typename ButtonsType>
class ButtonCoderInterface {
 public:
  virtual ~ButtonCoderInterface() {}
  virtual bool EncodeButtons(const ButtonsType& buttons_in,
                             KeyStatePB* buttons_out) const = 0;
  virtual bool DecodeButtons(const KeyStatePB& buttons_in,
                             ButtonsType* buttons_out) const = 0;
};

#endif  // BUTTON_CODER_INTERFACE_H_
