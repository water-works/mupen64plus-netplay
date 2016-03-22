#include "mupen64.h"

#include "base/netplayServiceProto.grpc.pb.h"
#include "client/button-coder-interface.h"

class Mupen64ButtonCoder : public ButtonCoderInterface<BUTTONS> {
 public:
  bool EncodeButtons(const BUTTONS& buttons_in,
                     KeyStatePB* buttons_out) const override;
  bool DecodeButtons(const KeyStatePB& buttons_in,
                     BUTTONS* buttons_out) const override;
};
