#include <string>

#include <catch2/catch_test_macros.hpp>

#include "mcp/json/json.hpp"

TEST_CASE("JSON document switches cleanly between parsed and mutable backing stores",
          "[json][document]") {
  mcp::json::Document doc;
  REQUIRE(doc.parse(R"({"old":true})"));
  REQUIRE(doc.root()["old"].get_bool(false));

  doc.create_object();
  doc.root().set_member("fresh", true);

  const std::string dumped = doc.dump(false);
  REQUIRE(dumped.find("fresh") != std::string::npos);
  REQUIRE(dumped.find("old") == std::string::npos);
}

TEST_CASE("JSON document parse clears previous mutable backing store", "[json][document]") {
  mcp::json::Document doc;
  doc.create_object();
  doc.root().set_member("old", true);

  REQUIRE(doc.parse(R"({"fresh":true})"));

  const std::string dumped = doc.dump(false);
  REQUIRE(dumped.find("fresh") != std::string::npos);
  REQUIRE(dumped.find("old") == std::string::npos);
}

TEST_CASE("JSON mutable arrays can copy parsed and mutable values", "[json][value]") {
  mcp::json::Document parsed;
  REQUIRE(parsed.parse(R"({"from":"parsed"})"));

  mcp::json::Document mutable_doc;
  mutable_doc.create_object();
  mutable_doc.root().set_member("from", "mutable");

  mcp::json::Document arr;
  arr.create_array();
  arr.root().push_back(parsed.root());
  arr.root().push_back(mutable_doc.root());

  const std::string dumped = arr.dump(false);
  REQUIRE(dumped.find("parsed") != std::string::npos);
  REQUIRE(dumped.find("mutable") != std::string::npos);
}
