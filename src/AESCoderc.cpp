#include "AESCoder.h"


void AESCoder::char2Byte(CryptoPP::byte *bytes, const char *str) {
  int len = std::strlen(str);
  for (int i = 0; i < len; ++i) {
    bytes[i] = static_cast<CryptoPP::byte>(str[i]);
  }
}

std::string AESCoder::aesEncrypt(const std::string &plain_text) {
  std::string cipher_text;

  // 先将密钥进行数据类型转换
  CryptoPP::byte key_b[CryptoPP::AES::DEFAULT_KEYLENGTH],
      iv_b[CryptoPP::AES::DEFAULT_KEYLENGTH];
  char2Byte(key_b, KEY), char2Byte(iv_b, IV);

  // 加密字节流
  CryptoPP::AES::Encryption aes_encryption(key_b,
                                           CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Encryption cbc_encryption(aes_encryption,
                                                               iv_b);
  CryptoPP::StreamTransformationFilter stf_encryptor(
      cbc_encryption, new CryptoPP::StringSink(cipher_text));
  stf_encryptor.Put(reinterpret_cast<const unsigned char *>(plain_text.c_str()),
                    plain_text.length());
  stf_encryptor.MessageEnd();

  return cipher_text;
}

std::string AESCoder::aesDecrypt(const std::string &cipher_text) {
  std::string decrypted_text;

  // 先将密钥进行数据类型转换
  CryptoPP::byte key_b[CryptoPP::AES::DEFAULT_KEYLENGTH],
      iv_b[CryptoPP::AES::DEFAULT_KEYLENGTH];
  char2Byte(key_b, KEY), char2Byte(iv_b, IV);

  // 解密字节流
  CryptoPP::AES::Decryption aes_decryption(key_b,
                                           CryptoPP::AES::DEFAULT_KEYLENGTH);
  CryptoPP::CBC_Mode_ExternalCipher::Decryption cbc_decryption(aes_decryption,
                                                               iv_b);
  CryptoPP::StreamTransformationFilter stf_decryptor(
      cbc_decryption, new CryptoPP::StringSink(decrypted_text));
  stf_decryptor.Put(
      reinterpret_cast<const unsigned char *>(cipher_text.c_str()),
      cipher_text.size());
  stf_decryptor.MessageEnd();

  return decrypted_text;
}

const char *AESCoder::KEY = "EasyPalm20241015";
const char *AESCoder::IV = "20241015EasyPalm";