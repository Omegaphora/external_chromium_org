// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_
#define CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_

#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"

namespace webkit_glue {
struct ResourceResponseInfo;
}

namespace content {

class BlinkPlatformImpl;

class WebURLLoaderImpl : public blink::WebURLLoader {
 public:
  explicit WebURLLoaderImpl(BlinkPlatformImpl* platform);
  virtual ~WebURLLoaderImpl();

  static blink::WebURLError CreateError(const blink::WebURL& unreachable_url,
                                        bool stale_copy_in_cache,
                                        int reason);
  CONTENT_EXPORT static void PopulateURLResponse(
      const GURL& url,
      const webkit_glue::ResourceResponseInfo& info,
      blink::WebURLResponse* response);

  // WebURLLoader methods:
  virtual void loadSynchronously(
      const blink::WebURLRequest& request,
      blink::WebURLResponse& response,
      blink::WebURLError& error,
      blink::WebData& data);
  virtual void loadAsynchronously(
      const blink::WebURLRequest& request,
      blink::WebURLLoaderClient* client);
  virtual void cancel();
  virtual void setDefersLoading(bool value);
  virtual void didChangePriority(blink::WebURLRequest::Priority new_priority);

 private:
  class Context;
  scoped_refptr<Context> context_;
  BlinkPlatformImpl* platform_;
};

}  // namespace content

#endif  // CONTENT_CHILD_WEB_URL_LOADER_IMPL_H_
