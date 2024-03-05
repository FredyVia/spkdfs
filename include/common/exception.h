#ifndef EXCEPTION_H
#define EXCEPTION_H
#include <exception>
#include <memory>
#include <sstream>
#include <string>

#include "service.pb.h"
namespace spkdfs {
  class MessageException : public std::exception {
    enum ERROR_CODE code;
    std::string info;

  public:
    MessageException(const ErrorMessage& e) : code(e.code()), info(e.message()) {}
    MessageException(enum ERROR_CODE code, const std::string& info) : code(code), info(info) {}
    ErrorMessage errorMessage() const {
      ErrorMessage errMsg;
      errMsg.set_code(code);
      errMsg.set_message(info);
      return errMsg;
    }
  };
}  // namespace spkdfs
#endif