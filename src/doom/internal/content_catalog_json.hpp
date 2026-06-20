#pragma once

#include "doom/internal/content_catalog.hpp"
#include "doom/internal/json_types.hpp"

namespace dmcp::json_serializers {

json_builder content_catalog_payload(const dmcp::content_catalog& catalog);

}  // namespace dmcp::json_serializers
