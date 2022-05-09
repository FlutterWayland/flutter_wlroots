import 'package:compositor_dart/keyboard/keyboard_client_controller.dart';

abstract class KeyboardController {
  void setShown(bool shown);
  void setClient(KeyboardClientController? client);
}

//class ValueChangedKeyboardController