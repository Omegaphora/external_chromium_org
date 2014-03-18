// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WV_TEST_LICENSE_SERVER_CONFIG_H_
#define CHROME_BROWSER_MEDIA_WV_TEST_LICENSE_SERVER_CONFIG_H_

#include "chrome/browser/media/test_license_server_config.h"

// License configuration to run the Widevine test license server.
class WVTestLicenseServerConfig : public TestLicenseServerConfig {
 public:
  WVTestLicenseServerConfig();
  virtual ~WVTestLicenseServerConfig();

  virtual std::string GetServerURL() OVERRIDE;

  virtual bool GetServerCommandLine(base::CommandLine* command_line) OVERRIDE;

  virtual bool IsPlatformSupported() OVERRIDE;

 private:
  // Server port. The port value should be set by calling SelectServerPort().
  uint16 port_;

  // Retrieves the path for the WV license server root:
  // third_party/widevine/test/license_server/
  void GetLicenseServerRootPath(base::FilePath* path);

  // Retrieves the path for the WV license server:
  // <license_server_root_path>/<platform>/
  void GetLicenseServerPath(base::FilePath* path);

  // Sets the server port to a randomly available port within a limited range.
  bool SelectServerPort();

  DISALLOW_COPY_AND_ASSIGN(WVTestLicenseServerConfig);
};

#endif  // CHROME_BROWSER_MEDIA_WV_TEST_LICENSE_SERVER_CONFIG_H_
