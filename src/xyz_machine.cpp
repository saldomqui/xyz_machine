/************************************************************************
 * by Salvador Dominguez (Saga Robotics Ltc)
 *
 ************************************************************************/
#include "main.h"

int main(int argc, char **argv)
{
  pthread_t ws_thread;
  void *ret_join;
  int ret;

  cout << "Main program starts" << endl;

  //------------- PARSING ARGUMENTS ------------------------------------------------
  if (parse_command_line(argc, argv) < 0)
  {
    error("Failed to load configuration parameters file. Exiting...");
    exit(-1);
  }
  //--------- CTRL+C TERMINATION SIGNAL HANDLER --
  signal(SIGINT, &sigsegv_handler);

  //------------- WEBSOCKET THREAD ------------------------------------------------
  pthread_create(&ws_thread, NULL, websocket_server_thread, NULL);

  // ------------- SERIAL PORT COMM ------------
  ser_comm = new SerialComm(cfg.serial_port, cfg.serial_baudrate, cfg.serial_timeout);

  if (!ser_comm->init())
  {
    cout << "Unable to open port " << endl;
    exit(-1);
  }

  pending_send = false;

  //---------- MAIN SERIAL PORT READ LOOP -----------------
  while (!done)
  {
    if (mutex.try_lock())
    {
      if (pending_send)
      {
        ser_comm->sendXYPosRef(machine_state_send);
        pending_send = false;
      }
      ser_comm->readStateFromserial(machine_state_rx);
      mutex.unlock();
    }
    else
    {
      cout << "No serial data" << endl;
    }
    usleep(10000);
  }

  delete ser_comm;

  cout << "waiting for websocket thread to finish" << endl;
  ret = pthread_join(ws_thread, &ret_join);
  if (ret != 0)
  {
    perror("pthread_join failed");
    exit(EXIT_FAILURE);
  }
  printf("Thread joined, it returned %s\n", (char *)ret_join);

  return 0;
}
