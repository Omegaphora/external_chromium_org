// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/rappor/rappor_pref_names.h"

namespace rappor {
namespace prefs {

// A randomly generated number, which determines cohort data is reported for.
const char kRapporCohort[] = "rappor.cohort";

// A base-64 encoded, randomly generated byte string, which is used as a seed
// for redacting collected data.
// Important: This value should remain secret at the client, and never be
// reported on the network, or to the server.
const char kRapporSecret[] = "rappor.secret";

}  // namespace prefs
}  // namespace rappor
