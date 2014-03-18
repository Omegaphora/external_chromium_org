// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/renderer/aw_render_frame_observer.h"

#include "android_webview/common/print_messages.h"
#include "android_webview/renderer/print_web_view_helper.h"
#include "content/public/renderer/render_frame.h"

namespace android_webview {

AwRenderFrameObserver::AwRenderFrameObserver(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
}

AwRenderFrameObserver::~AwRenderFrameObserver() {
}

bool AwRenderFrameObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(AwRenderFrameObserver, message)
    IPC_MESSAGE_HANDLER(PrintMsg_PrintNodeUnderContextMenu,
                        OnPrintNodeUnderContextMenu)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

void AwRenderFrameObserver::OnPrintNodeUnderContextMenu() {
  printing::PrintWebViewHelper* helper =
      printing::PrintWebViewHelper::Get(render_frame()->GetRenderView());
  if (helper)
    helper->PrintNode(render_frame()->GetContextMenuNode());
}

}  // namespace android_webview
