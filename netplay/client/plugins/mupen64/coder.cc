#include "client/plugins/mupen64/coder.h"

bool Mupen64ButtonCoder::EncodeButtons(const BUTTONS& in,
                                       KeyStatePB* out) const {
  out->set_right_d_pad(in.R_DPAD == 1);
  out->set_left_d_pad(in.L_DPAD == 1);
  out->set_down_d_pad(in.D_DPAD == 1);
  out->set_up_d_pad(in.U_DPAD == 1);
  out->set_start_button(in.START_BUTTON == 1);
  out->set_z_trigger(in.Z_TRIG == 1);
  out->set_b_button(in.B_BUTTON == 1);
  out->set_a_button(in.A_BUTTON == 1);

  out->set_right_d_pad(in.R_DPAD == 1);
  out->set_right_c_button(in.R_CBUTTON == 1);
  out->set_left_c_button(in.L_CBUTTON == 1);
  out->set_down_c_button(in.D_CBUTTON == 1);
  out->set_up_c_button(in.U_CBUTTON == 1);
  out->set_left_trigger(in.L_TRIG == 1);
  out->set_right_trigger(in.R_TRIG == 1);
  out->set_reserved_1(in.Reserved1 == 1);
  out->set_reserved_2(in.Reserved2 == 1);

  out->set_x_axis(in.X_AXIS);
  out->set_y_axis(in.Y_AXIS);

  return true;
}

bool Mupen64ButtonCoder::DecodeButtons(const KeyStatePB& in,
                                       BUTTONS* out) const {
  out->R_DPAD = in.right_d_pad();
  out->L_DPAD = in.left_d_pad();
  out->D_DPAD = in.down_d_pad();
  out->U_DPAD = in.up_d_pad();
  out->START_BUTTON = in.start_button();
  out->Z_TRIG = in.z_trigger();
  out->B_BUTTON = in.b_button();
  out->A_BUTTON = in.a_button();

  out->R_CBUTTON = in.right_c_button();
  out->L_CBUTTON = in.left_c_button();
  out->D_CBUTTON = in.down_c_button();
  out->U_CBUTTON = in.up_c_button();
  out->R_TRIG = in.right_trigger();
  out->L_TRIG = in.left_trigger();
  out->Reserved1 = in.reserved_1();
  out->Reserved2 = in.reserved_2();

  out->X_AXIS = in.x_axis();
  out->Y_AXIS = in.y_axis();

  return true;
}
