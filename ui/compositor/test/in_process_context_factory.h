// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_FACTORY_H_
#define UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_FACTORY_H_

#include "ui/compositor/compositor.h"

namespace webkit {
namespace gpu {
class ContextProviderInProcess;
}
}

namespace ui {

class InProcessContextFactory : public ContextFactory {
 public:
  InProcessContextFactory();
  virtual ~InProcessContextFactory();

  // ContextFactory implementation
  virtual scoped_ptr<cc::OutputSurface> CreateOutputSurface(
      Compositor* compositor,
      bool software_fallback) OVERRIDE;

  virtual scoped_refptr<Reflector> CreateReflector(
      Compositor* mirrored_compositor,
      Layer* mirroring_layer) OVERRIDE;
  virtual void RemoveReflector(scoped_refptr<Reflector> reflector) OVERRIDE;

  virtual scoped_refptr<cc::ContextProvider>
      OffscreenCompositorContextProvider() OVERRIDE;
  virtual scoped_refptr<cc::ContextProvider> SharedMainThreadContextProvider()
      OVERRIDE;
  virtual void RemoveCompositor(Compositor* compositor) OVERRIDE;
  virtual bool DoesCreateTestContexts() OVERRIDE;

 private:
  scoped_refptr<webkit::gpu::ContextProviderInProcess>
      offscreen_compositor_contexts_;
  scoped_refptr<webkit::gpu::ContextProviderInProcess>
      shared_main_thread_contexts_;

  DISALLOW_COPY_AND_ASSIGN(InProcessContextFactory);
};

}  // namespace ui

#endif  // UI_COMPOSITOR_TEST_IN_PROCESS_CONTEXT_FACTORY_H_
