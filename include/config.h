
#ifndef CONFIG_H
#define CONFIG_H

#include <string.h>

using std::string;

typedef struct config_
{
  string serial_port;
  int serial_baudrate;
  int serial_timeout;
  int websocket_port;
} config;

typedef struct state_
{
  float x_pos, y_pos, z_pos;
  float x_ref, y_ref, z_ref;
  int tool_pos;
} state;
#endif
