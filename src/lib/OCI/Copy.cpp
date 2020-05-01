#include <OCI/Copy.hpp>
#include <future>
#include <vector>

void OCI::Copy( const std::string& rsrc, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  auto source        = src->copy();
  auto manifest_list = Manifest< Schema2::ManifestList >( source.get(), rsrc, target );

  if ( manifest_list.schemaVersion == 1 ) { // Fall back to Schema1
    auto image_manifest = Manifest< Schema1::ImageManifest >( source.get(), rsrc, target );

    Copy( image_manifest, target, source.get(), dest );
  } else {
    Copy( manifest_list, target, source.get(), dest );
  }
}

void OCI::Copy( const Schema1::ImageManifest& image_manifest, const std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  (void)src;
  (void)target;

  for ( auto const& layer: image_manifest.fsLayers ) {
    if ( layer.first == "blobSum" ) {
      if ( not dest->hasBlob( image_manifest, layer.second ) ) {
        std::cout << "Destintaion doesn't have layer" << std::endl;
      }
    }
  }

  std::cout << "Test is successful and Post Schema1::ImageManifest to OCI::Base::Client::putManifest" << std::endl;
}

void OCI::Copy( Schema2::ManifestList& manifest_list, std::string const& target, OCI::Base::Client* src, OCI::Base::Client* dest ) {
  bool success = true;
  std::vector< std::future< bool > > processes;

  processes.reserve( manifest_list.manifests.size() );

  for ( auto& manifest: manifest_list.manifests ) {
    processes.push_back( std::async( std::launch::async, [&]() -> bool {
      auto source         = src->copy();
      auto image_manifest = Manifest< Schema2::ImageManifest >( source.get(), manifest_list.name, manifest.digest );

      return Copy( image_manifest, manifest.digest, source.get(), dest );
    } ) );
  }

  for ( auto& process: processes ) {
    process.wait();
    success = success and process.get();
  }

  if ( success ) {
    std::cout << "All ImageManifests for ManifestList: " << target << " have been put to target Client." << std::endl;
    dest->putManifest( manifest_list, target );
  }
}

auto OCI::Copy( Schema2::ImageManifest const& image_manifest, std::string& target, OCI::Base::Client* src, OCI::Base::Client* dest ) -> bool {
  bool retVal              = true;
  auto dest_image_manifest = Manifest< Schema2::ImageManifest >( dest, image_manifest.name, target );
  std::vector< std::future< bool > > processes;

  processes.reserve( image_manifest.layers.size() );

  for ( auto const& layer: image_manifest.layers ) {
    processes.push_back( std::async( std::launch::async, [&]() -> bool {
      bool lretVal     = true;
      auto source      = src->copy();
      auto destination = dest->copy();

      if ( source->hasBlob( image_manifest, target, layer.digest ) and not destination->hasBlob( image_manifest, target, layer.digest ) ) {
        uint64_t data_sent = 0;
        std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
          data_sent += data_length;

          return destination->putBlob( image_manifest, target, layer.digest, layer.size, data, data_length );
        };

        lretVal = source->fetchBlob( image_manifest.name, layer.digest, call_back );
      }

      return lretVal;
    } ) );
  }

  for ( auto& process : processes ) {
    process.wait();
    retVal = retVal and process.get();
  }

  if ( retVal ) {
    uint64_t data_sent = 0;
    std::function< bool( const char *, uint64_t ) > call_back = [&]( const char *data, uint64_t data_length ) -> bool {
      data_sent += data_length;

      return dest->putBlob( image_manifest, target, image_manifest.config.digest, image_manifest.config.size, data, data_length );
    };

    retVal = retVal and src->fetchBlob( image_manifest.name, image_manifest.config.digest, call_back );

    if ( retVal ) {
      std::cout << "All images for ImageManifest: " << target << " have been put to target Client." << std::endl;
      retVal = dest->putManifest( image_manifest, target );
    }
  }

  return retVal;
}
