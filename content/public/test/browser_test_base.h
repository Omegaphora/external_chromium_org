// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_BROWSER_TEST_BASE_H_
#define CONTENT_PUBLIC_TEST_BROWSER_TEST_BASE_H_

#include "base/callback.h"
#include "base/compiler_specific.h"
#include "base/threading/thread.h"
#include "net/test/spawned_test_server/spawned_test_server.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace base {
class CommandLine;
class FilePath;
}

namespace net {
namespace test_server {
class EmbeddedTestServer;
}

class RuleBasedHostResolverProc;
}  // namespace net

namespace content {

class BrowserTestBase : public testing::Test {
 public:
  BrowserTestBase();
  virtual ~BrowserTestBase();

  // We do this so we can be used in a Task.
  void AddRef() {}
  void Release() {}

  // Configures everything for an in process browser test, then invokes
  // BrowserMain. BrowserMain ends up invoking RunTestOnMainThreadLoop.
  virtual void SetUp() OVERRIDE;

  // Restores state configured in SetUp.
  virtual void TearDown() OVERRIDE;

  // Override this to add any custom setup code that needs to be done on the
  // main thread after the browser is created and just before calling
  // RunTestOnMainThread().
  virtual void SetUpOnMainThread() {}

  // Override this to add any custom teardown code that needs to be done on the
  // main thread right after RunTestOnMainThread().
  virtual void TearDownOnMainThread() {}

  // Override this to add command line flags specific to your test.
  virtual void SetUpCommandLine(base::CommandLine* command_line) {}

  // Returns the host resolver being used for the tests. Subclasses might want
  // to configure it inside tests.
  net::RuleBasedHostResolverProc* host_resolver() {
    return rule_based_resolver_.get();
  }

 protected:
  // We need these special methods because SetUp is the bottom of the stack
  // that winds up calling your test method, so it is not always an option
  // to do what you want by overriding it and calling the superclass version.
  //
  // Override this for things you would normally override SetUp for. It will be
  // called before your individual test fixture method is run, but after most
  // of the overhead initialization has occured.
  virtual void SetUpInProcessBrowserTestFixture() {}

  // Override this for things you would normally override TearDown for.
  virtual void TearDownInProcessBrowserTestFixture() {}

  // Override this rather than TestBody.
  virtual void RunTestOnMainThread() = 0;

  // This is invoked from main after browser_init/browser_main have completed.
  // This prepares for the test by creating a new browser, runs the test
  // (RunTestOnMainThread), quits the browsers and returns.
  virtual void RunTestOnMainThreadLoop() = 0;

  // Returns the testing server. Guaranteed to be non-NULL.
  // TODO(phajdan.jr): Remove test_server accessor (http://crbug.com/96594).
  const net::SpawnedTestServer* test_server() const {
    return test_server_.get();
  }
  net::SpawnedTestServer* test_server() { return test_server_.get(); }

  // Returns the embedded test server. Guaranteed to be non-NULL.
  const net::test_server::EmbeddedTestServer* embedded_test_server() const {
    return embedded_test_server_.get();
  }
  net::test_server::EmbeddedTestServer* embedded_test_server() {
    return embedded_test_server_.get();
  }

#if defined(OS_POSIX)
  // This is only needed by a test that raises SIGTERM to ensure that a specific
  // codepath is taken.
  void DisableSIGTERMHandling() {
    handle_sigterm_ = false;
  }
#endif

  // This function is meant only for classes that directly derive from this
  // class to construct the test server in their constructor. They might need to
  // call this after setting up the paths. Actual test cases should never call
  // this.
  // |test_server_base| is the path, relative to src, to give to the test HTTP
  // server.
  void CreateTestServer(const base::FilePath& test_server_base);

  // When the test is running in --single-process mode, runs the given task on
  // the in-process renderer thread. A nested message loop is run until it
  // returns.
  void PostTaskToInProcessRendererAndWait(const base::Closure& task);

  // Call this before SetUp() to cause the test to generate pixel output.
  void EnablePixelOutput();

  // Call this before SetUp() to not use GL, but use software compositing
  // instead.
  void UseSoftwareCompositing();

  // Returns true if the test will be using GL acceleration via OSMesa.
  bool UsingOSMesa() const;

 private:
  void ProxyRunTestOnMainThreadLoop();

  // Testing server, started on demand.
  scoped_ptr<net::SpawnedTestServer> test_server_;

  // Embedded test server, cheap to create, started on demand.
  scoped_ptr<net::test_server::EmbeddedTestServer> embedded_test_server_;

  // Host resolver used during tests.
  scoped_refptr<net::RuleBasedHostResolverProc> rule_based_resolver_;

  // When true, the compositor will produce pixel output that can be read back
  // for pixel tests.
  bool enable_pixel_output_;

  // When true, do compositing with the software backend instead of using GL.
  bool use_software_compositing_;

#if defined(OS_POSIX)
  bool handle_sigterm_;
#endif
};

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_BROWSER_TEST_BASE_H_
