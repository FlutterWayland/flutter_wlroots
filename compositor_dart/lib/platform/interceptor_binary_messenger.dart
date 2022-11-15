import 'dart:ui';

import 'package:flutter/services.dart';

class InterceptorBinaryMessenger extends BinaryMessenger {
  InterceptorBinaryMessenger(this.delegate);

  final BinaryMessenger delegate;

  final Map<String, MessageHandler> _inboundHandlers = {};

  @override
  Future<ByteData?> handlePlatformMessage(String channel, ByteData? data, PlatformMessageResponseCallback? callback) {
    Future<ByteData?>? result;
    if (_inboundHandlers.containsKey(channel)) {
      result = _inboundHandlers[channel]!(data);
    }
    result ??= Future.value(null);
    if (callback != null) {
      result.then((value) {
        callback(value);
        return value;
      });
    }
    return result;
  }

  @override
  void setMessageHandler(String channel, MessageHandler? handler) {
    if (handler == null) {
      _inboundHandlers.remove(channel);
    } else {
      _inboundHandlers[channel] = handler;
    }
    delegate.setMessageHandler(channel, handler);
  }

  final Map<String, MessageHandler> _outboundHandlers = {};

  final Set<Future<ByteData?>> _pendingOutbound = {};

  @override
  Future<ByteData?>? send(String channel, ByteData? message) {
    Future<ByteData?>? result;
    final MessageHandler? handler = _outboundHandlers[channel];
    if (handler == null) {
      result = delegate.send(channel, message);
    } else {
      result = handler(message);
    }
    if (result != null) {
      _pendingOutbound.add(result);
      result.catchError((_err) {}).whenComplete(() => _pendingOutbound.remove(result));
    }
    return result;
  }

  void setOutgoingInterceptor(String channel, MessageHandler? handler) {
    if (handler == null) {
      _outboundHandlers.remove(channel);
    } else {
      _outboundHandlers[channel] = handler;
    }
  }
}
