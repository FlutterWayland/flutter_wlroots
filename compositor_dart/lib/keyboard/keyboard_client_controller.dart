import 'package:flutter/material.dart';

/// This class represents a single client of a keyboard.
/// It enables the keyboard to perform actions on the text content of the client
/// text field.
class KeyboardClientController extends ValueNotifier<TextEditingValue> {
  final int connectionId;

  KeyboardClientController({
    required this.connectionId,
    value = TextEditingValue.empty,
  }) : super(value);

  /// The current string the user is editing.
  String get text => value.text;

  /// Setting this will notify all the listeners of this [TextEditingController]
  /// that they need to update (it calls [notifyListeners]). For this reason,
  /// this value should only be set between frames, e.g. in response to user
  /// actions, not during the build, layout, or paint phases.
  ///
  /// This property can be set from a listener added to this
  /// [TextEditingController]; however, one should not also set [selection]
  /// in a separate statement. To change both the [text] and the [selection]
  /// change the controller's [value].
  set text(String newText) {
    value = value.copyWith(
      text: newText,
      selection: const TextSelection.collapsed(offset: -1),
      composing: TextRange.empty,
    );
  }

  @override
  set value(TextEditingValue newValue) {
    assert(
      !newValue.composing.isValid || newValue.isComposingRangeValid,
      'New TextEditingValue $newValue has an invalid non-empty composing range '
      '${newValue.composing}. It is recommended to use a valid composing range, '
      'even for readonly text fields',
    );
    super.value = newValue;
  }

  /// The currently selected [text].
  ///
  /// If the selection is collapsed, then this property gives the offset of the
  /// cursor within the text.
  TextSelection get selection => value.selection;

  /// Setting this will notify all the listeners of this [TextEditingController]
  /// that they need to update (it calls [notifyListeners]). For this reason,
  /// this value should only be set between frames, e.g. in response to user
  /// actions, not during the build, layout, or paint phases.
  ///
  /// This property can be set from a listener added to this
  /// [TextEditingController]; however, one should not also set [text]
  /// in a separate statement. To change both the [text] and the [selection]
  /// change the controller's [value].
  ///
  /// If the new selection is of non-zero length, or is outside the composing
  /// range, the composing range is cleared.
  set selection(TextSelection newSelection) {
    if (!isSelectionWithinTextBounds(newSelection)) throw FlutterError('invalid text selection: $newSelection');
    final TextRange newComposing =
        newSelection.isCollapsed && _isSelectionWithinComposingRange(newSelection) ? value.composing : TextRange.empty;
    value = value.copyWith(selection: newSelection, composing: newComposing);
  }

  /// Set the [value] to empty.
  ///
  /// After calling this function, [text] will be the empty string and the
  /// selection will be collapsed at zero offset.
  ///
  /// Calling this will notify all the listeners of this [TextEditingController]
  /// that they need to update (it calls [notifyListeners]). For this reason,
  /// this method should only be called between frames, e.g. in response to user
  /// actions, not during the build, layout, or paint phases.
  void clear() {
    value = const TextEditingValue(selection: TextSelection.collapsed(offset: 0));
  }

  /// Set the composing region to an empty range.
  ///
  /// The composing region is the range of text that is still being composed.
  /// Calling this function indicates that the user is done composing that
  /// region.
  ///
  /// Calling this will notify all the listeners of this [TextEditingController]
  /// that they need to update (it calls [notifyListeners]). For this reason,
  /// this method should only be called between frames, e.g. in response to user
  /// actions, not during the build, layout, or paint phases.
  void clearComposing() {
    value = value.copyWith(composing: TextRange.empty);
  }

  /// Check that the [selection] is inside of the bounds of [text].
  bool isSelectionWithinTextBounds(TextSelection selection) {
    return selection.start <= text.length && selection.end <= text.length;
  }

  /// Check that the [selection] is inside of the composing range.
  bool _isSelectionWithinComposingRange(TextSelection selection) {
    return selection.start >= value.composing.start && selection.end <= value.composing.end;
  }

  /// Deletes one character according to common text cursor semantics:
  /// * If a region is selected, it will be deleted.
  /// * If the cursor has a preceeding character, it will be deleted.
  void deleteOne() {
    // Cursor at start, do nothing.
    if (selection.baseOffset == 0 && selection.extentOffset == 0) return;

    String newText;

    if (selection.baseOffset == selection.extentOffset) {
      // There is no selection.
      var chars = CharacterRange.at(text, selection.baseOffset);
      chars.moveBack();
      chars.replaceRange(Characters.empty);
      newText = chars.source.string;
    } else {
      // There IS a selection. Delete that.
      newText = selection.textBefore(text) + selection.textAfter(text);
    }

    value = TextEditingValue(
      text: newText,
      selection: selection.copyWith(
        extentOffset: selection.baseOffset,
      ),
    );
  }

  void addText(String insertText) {
    String newText = selection.textBefore(text) + insertText + selection.textAfter(text);
    value = TextEditingValue(
      text: newText,
      selection: selection.copyWith(
        baseOffset: selection.baseOffset + insertText.length,
        extentOffset: selection.baseOffset + insertText.length,
      ),
    );
  }
}
