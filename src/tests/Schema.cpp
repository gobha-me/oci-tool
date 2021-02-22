#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"
#include <OCI/Schema1.hpp>
#include <OCI/Schema2.hpp>

// CMAKE_CPP_EXTRA=${SRC_LIB_DIR}/OCI/Schema1.cpp;${SRC_LIB_DIR}/OCI/Schema2.cpp

TEST_CASE( "Schema1" ) {}

TEST_CASE( "Schema2" ) {
  auto ml_json = R"(
{
  "manifests": [
    {
      "digest": "sha256:d001a8b9d98bfccba9cd5058784d5e5220a85ba950c06abda49837be81ffeebc",
      "mediaType": "application/vnd.docker.distribution.manifest.v2+json",
      "platform": {
        "architecture": "amd64",
        "os": "linux"
      },
      "size": 1160
    }
  ],
  "mediaType": "application/vnd.docker.distribution.manifest.list.v2+json",
  "name": "openshift4/apb-base",
  "originDomain": "registry.redhat.io",
  "raw_str": "{\n    \"manifests\": [\n        {\n            \"digest\": \"sha256:d001a8b9d98bfccba9cd5058784d5e5220a85ba950c06abda49837be81ffeebc\",\n            \"mediaType\": \"application/vnd.docker.distribution.manifest.v2+json\",\n            \"platform\": {\n                \"architecture\": \"amd64\",\n                \"os\": \"linux\"\n            },\n            \"size\": 1160\n        }\n    ],\n    \"mediaType\": \"application/vnd.docker.distribution.manifest.list.v2+json\",\n    \"schemaVersion\": 2\n}",
  "requestedTarget": "4.1",
  "schemaVersion": 2
})"_json;

  auto im_json = R"(
{
  "config": {
    "digest": "sha256:2dd757fce966780e5575d67aef821f91ae4daf40fa6b4c11ce8447ceb6a29c19",
    "mediaType": "application/vnd.docker.container.image.v1+json",
    "size": 5154
  },
  "layers": [
    {
      "digest": "sha256:bb13d92caffa705f32b8a7f9f661e07ddede310c6ccfa78fb53a49539740e29b",
      "mediaType": "application/vnd.docker.image.rootfs.diff.tar.gzip",
      "size": 76241735
    },
    {
      "digest": "sha256:455ea8ab06218495bbbcb14b750a0d644897b24f8c5dcf9e8698e27882583412",
      "mediaType": "application/vnd.docker.image.rootfs.diff.tar.gzip",
      "size": 1613
    },
    {
      "digest": "sha256:935ce2f796a92f0b6fa7762d7bab7d252ddb59d03cd3afba42d3785b571854f1",
      "mediaType": "application/vnd.docker.image.rootfs.diff.tar.gzip",
      "size": 3494835
    },
    {
      "digest": "sha256:8b5e2b6f2e05dc02479437d1414063ba908bbdc22df2830e6d52ba758d7d74e1",
      "mediaType": "application/vnd.docker.image.rootfs.diff.tar.gzip",
      "size": 72812309
    }
  ],
  "mediaType": "application/vnd.docker.distribution.manifest.v2+json",
  "name": "openshift4/apb-base",
  "originDomain": "registry.redhat.io",
  "raw_str": "{\n   \"schemaVersion\": 2,\n   \"mediaType\": \"application/vnd.docker.distribution.manifest.v2+json\",\n   \"config\": {\n      \"mediaType\": \"application/vnd.docker.container.image.v1+json\",\n      \"size\": 5154,\n      \"digest\": \"sha256:2dd757fce966780e5575d67aef821f91ae4daf40fa6b4c11ce8447ceb6a29c19\"\n   },\n   \"layers\": [\n      {\n         \"mediaType\": \"application/vnd.docker.image.rootfs.diff.tar.gzip\",\n         \"size\": 76241735,\n         \"digest\": \"sha256:bb13d92caffa705f32b8a7f9f661e07ddede310c6ccfa78fb53a49539740e29b\"\n      },\n      {\n         \"mediaType\": \"application/vnd.docker.image.rootfs.diff.tar.gzip\",\n         \"size\": 1613,\n         \"digest\": \"sha256:455ea8ab06218495bbbcb14b750a0d644897b24f8c5dcf9e8698e27882583412\"\n      },\n      {\n         \"mediaType\": \"application/vnd.docker.image.rootfs.diff.tar.gzip\",\n         \"size\": 3494835,\n         \"digest\": \"sha256:935ce2f796a92f0b6fa7762d7bab7d252ddb59d03cd3afba42d3785b571854f1\"\n      },\n      {\n         \"mediaType\": \"application/vnd.docker.image.rootfs.diff.tar.gzip\",\n         \"size\": 72812309,\n         \"digest\": \"sha256:8b5e2b6f2e05dc02479437d1414063ba908bbdc22df2830e6d52ba758d7d74e1\"\n      }\n   ]\n}",
  "requestedDigest": "sha256:d001a8b9d98bfccba9cd5058784d5e5220a85ba950c06abda49837be81ffeebc",
  "requestedTarget": "4.1",
  "schemaVersion": 2
})"_json;

  auto ml = ml_json.get< OCI::Schema2::ManifestList >();
  auto im = im_json.get< OCI::Schema2::ImageManifest >();

  REQUIRE( ml.mediaType == "application/vnd.docker.distribution.manifest.list.v2+json" );
  REQUIRE( ml.schemaVersion == 2 );
  REQUIRE( ml.originDomain == "" );
  REQUIRE( ml.name == "" );
  REQUIRE( ml.requestedTarget == "" );

  REQUIRE( ml.manifests[ 0 ].digest == "sha256:d001a8b9d98bfccba9cd5058784d5e5220a85ba950c06abda49837be81ffeebc" );
  REQUIRE( ml.manifests[ 0 ].mediaType == "application/vnd.docker.distribution.manifest.v2+json" );
  REQUIRE( ml.manifests[ 0 ].size == 1160 );
  REQUIRE( ml.manifests[ 0 ].platform.architecture == "amd64" );
  REQUIRE( ml.manifests[ 0 ].platform.os == "linux" );
}
