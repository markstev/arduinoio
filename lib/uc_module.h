#ifndef JDUINO_UC_UC_MODULE_H_
#define JDUINO_UC_UC_MODULE_H_
#include "message.h"

namespace arduinoio {

class UCModule {
 public:
  // The Module may raise a message to be sent to other modules.
  virtual const Message* Tick() = 0;

  // The Module may or may not accept the message.
  // If it does, it must make a copy.
  virtual bool AcceptMessage(const Message &message) = 0;

  virtual ~UCModule()  {}
};

}  // namespace arduinoio

#endif  // JDUINO_UC_UC_MODULE_H_
