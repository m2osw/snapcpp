/*
  Copyright (c) DataStax, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "monitor_reporting.hpp"

using namespace datastax;
using namespace datastax::internal::core;

namespace datastax { namespace internal { namespace core {

MonitorReporting* create_monitor_reporting(const String& client_id, const String& session_id,
                                           const Config& config) {
  return new NopMonitorReporting();
}

}}} // namespace datastax::internal::core
