#ifndef SERIAL_COMM_H
#define SERIAL_COMM_H

#include <serial/serial.h>

#include "config.h"

class SerialComm{
  public:
    SerialComm(const string &serial_port, int serial_baudrate, int serial_timeout);
    ~SerialComm();
    bool init(void);
    bool readStateFromserial(state &st);
    void sendPosRefs(state &st);
    void sendXYPosRef(state &st);
    bool readSerialACK(void);
  private:
    serial::Serial ser_;
    string serial_port_;
    int serial_baudrate_;
    int serial_timeout_;
};

#endif //SERIAL_COMM_H