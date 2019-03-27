// Copyright 2019 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef TINK_SUBTLE_AES_GCM_HKDF_STREAM_SEGMENT_DECRYPTER_H_
#define TINK_SUBTLE_AES_GCM_HKDF_STREAM_SEGMENT_DECRYPTER_H_

#include <vector>

#include "absl/strings/string_view.h"
#include "openssl/aead.h"
#include "tink/subtle/common_enums.h"
#include "tink/subtle/stream_segment_decrypter.h"
#include "tink/util/status.h"
#include "tink/util/statusor.h"

namespace crypto {
namespace tink {
namespace subtle {

// StreamSegmentDecrypter for streaming decryption using AES-GCM with HKDF as
// key derivation function.
//
// Each ciphertext uses a new AES-GCM key that is derived from the key
// derivation key, a randomly chosen salt of the same size as the key and a
// nonce prefix.
//
// The format of a ciphertext is
//   header || segment_0 || segment_1 || ... || segment_k.
// where:
//  - segment_i is the i-th segment of the ciphertext.
//  - the size of segment_1 .. segment_{k-1} is get_ciphertext_segment_size()
//  - segment_0 is shorter, so that segment_0, the header and other information
//    of size get_ciphertext_offset() align with get_ciphertext_segment_size().
//
// The format of the header is
//   header_size || salt || nonce_prefix
// where
//  - header_size is 1 byte determining the size of the header
//  - salt is a salt used in the key derivation
//  - nonce_prefix is the prefix of the nonce
//
// In principle header_size is redundant information, since the length of the
// header can be determined from the key size.

class AesGcmHkdfStreamSegmentDecrypter : public StreamSegmentDecrypter {
 public:
  // All sizes are in bytes.
  struct Params {
    std::string ikm;
    HashType hkdf_hash;
    int derived_key_size;
    int first_segment_offset;
    int ciphertext_segment_size;
    std::string associated_data;
  };

  // A factory.
  static util::StatusOr<std::unique_ptr<StreamSegmentDecrypter>>
      New(const Params& params);

  // Overridden methods of StreamSegmentDecrypter.
  util::Status Init(const std::vector<uint8_t>& header) override;

  util::Status DecryptSegment(
      const std::vector<uint8_t>& ciphertext,
      int64_t segment_number,
      bool is_last_segment,
      std::vector<uint8_t>* plaintext_buffer) override;

  int get_header_size() const override {
    return header_size_;
  }

  int get_plaintext_segment_size() const override;

  int get_ciphertext_segment_size() const override {
    return ciphertext_segment_size_;
  }
  int get_ciphertext_offset() const override {
    return ciphertext_offset_;
  }
  ~AesGcmHkdfStreamSegmentDecrypter() override {}
  util::Status InitCtx(absl::string_view key_value);


 private:
  AesGcmHkdfStreamSegmentDecrypter() {}

  // Parameters set upon decrypter creation.
  // All sizes are in bytes.
  std::string ikm_;
  HashType hkdf_hash_;
  int header_size_;
  int ciphertext_segment_size_;
  int ciphertext_offset_;
  int derived_key_size_;
  std::string associated_data_;
  bool is_initialized_;

  // Parameters set when initializing with data from stream header.
  std::vector<uint8_t> salt_;
  std::vector<uint8_t> nonce_prefix_;
  std::string key_value_;
  bssl::ScopedEVP_AEAD_CTX ctx_;
};

}  // namespace subtle
}  // namespace tink
}  // namespace crypto

#endif  // TINK_SUBTLE_AES_GCM_HKDF_STREAM_SEGMENT_DECRYPTER_H_
