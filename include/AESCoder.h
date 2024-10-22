#ifndef _AESCODER_H
#define _AESCODER_H

#include <iostream>
#include <string>

#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>

class CNet;

class AESCoder {
  friend class CNet;
public:
  // 加密函数
  static std::string aesEncrypt(const std::string &plain_text);
  // 解密函数
  static std::string aesDecrypt(const std::string &cipher_text);

private:
  static const char *KEY;
  static const char *IV;

private:
  static void char2Byte(CryptoPP::byte *bytes, const char *str);
};

#endif