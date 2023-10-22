/************************************************************************
 * by Salvador Dominguez (Saga Robotics Ltc)
 *
 ************************************************************************/
#include <getopt.h>

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <arpa/inet.h>
#include <ctype.h>

#include <iostream>
#include <vector>
#include <ctime>
#include <sstream>     

#include <serial/serial.h>
#include <pthread.h>
#include <mutex>

// For configuration file
#include "yaml-cpp/yaml.h"

#include "config.h"

#include "WebsocketServer.h"

#include <thread>
#include <asio/io_service.hpp>

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

bool done = false;
string configFile;
config cfg;

string pck_path = "";

std::mutex mutex;    // Mutex to synchronise access to shared variable
state machine_state; // This is a shared variable

std::vector<std::string> stringSplit(const std::string &s, signed char separator);

void sigsegv_handler(int sig)
{
  printf("Captured CTRL+C\n");

  done = true;
}

void error(const char *msg)
{
  perror(msg);
  exit(1);
}

bool loadConfigFile(const string &file)
{
  YAML::Node doc = YAML::LoadFile(file.c_str());

  try
  {
    cfg.serial_port = doc["serial_port"].as<string>();
    cout << "serial_port:" << cfg.serial_port << endl;
  }
  catch (YAML::InvalidScalar &ex)
  {
    cout << "Calibration file does not contain a tag for serial_port value or it is invalid." << endl;
    return false;
  }

  try
  {
    cfg.serial_baudrate = doc["serial_baudrate"].as<int>();
    cout << "serial_baudrate:" << cfg.serial_baudrate << endl;
  }
  catch (YAML::InvalidScalar &ex)
  {
    cout << "Calibration file does not contain a tag for serial_baudrate value or it is invalid." << endl;
    return false;
  }

  try
  {
    cfg.serial_timeout = doc["serial_timeout"].as<int>();
    cout << "serial_timeout:" << cfg.serial_timeout << endl;
  }
  catch (YAML::InvalidScalar &ex)
  {
    cout << "Calibration file does not contain a tag for serial_timeout value or it is invalid." << endl;
    return false;
  }

  try
  {
    cfg.websocket_port = doc["websocket_port"].as<int>();
    cout << "websocket_port:" << cfg.websocket_port << endl;
  }
  catch (YAML::InvalidScalar &ex)
  {
    cout << "Calibration file does not contain a tag for websocket_port value or it is invalid." << endl;
    return false;
  }

  return true;
}

void print_help(void)
{
  static const char *help =
      "Usage: xyz_machine [OPTIONS]\n"
      "Options are:\n"
      " -f, --file CONFIG_FILE  Loads parameters from configuration file\n"
      " -h, --help                        Display this help\n";

  printf("%s", help);
}

/* Return -1 on error, 0 on success */
int parse_command_line(int argc, char *const argv[])
{
  int option;
  bool config_file_loaded = false;

  struct option longopts[] = {
      {"file", required_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {NULL, 0, NULL, 0}};

  while ((option = getopt_long(argc, argv, "f:s:h", longopts, NULL)) != -1)
  {
    switch (char(option))
    {
    case 'h':
      print_help();
      return -1;
      break;
    case 'f':
      if (optarg)
      {
        configFile = optarg;
        printf("config file %s\n", optarg);

        //--------- LOAD PARAMETERS FROM CONFIGURATION FILE -----------
        if (!loadConfigFile(configFile))
        {
          cout << "Couldn't load configuration parameters" << endl;
          print_help();
          return -1;
        }
        else
          config_file_loaded = true;

        std::vector<std::string> folders = stringSplit(configFile, '/');

        for (int s = 0; s < (int(folders.size()) - 2); s++)
        {
          pck_path += folders[s] + "/";
        }
        cout << "package_path:" << pck_path << endl;
      }
      else
      {
        printf("No parameters configuration file has been especified. Please set a valid yaml configuration file\n");
        print_help();
        return -1;
      }
      break;
    }
  }
  if (!config_file_loaded)
  {
    printf("No configuration file has been especified. Please set valid YAML configuration file\n");
    print_help();
    return -1;
  }
  return 0;
}

std::string datetime()
{
  time_t rawtime;
  struct tm *timeinfo;
  char buffer[80];

  time(&rawtime);
  timeinfo = localtime(&rawtime);

  strftime(buffer, 80, "%Y-%m-%d-%H-%M-%S", timeinfo);
  return std::string(buffer);
}

std::vector<std::string> stringSplit(const std::string &s, signed char separator)
{
  std::vector<std::string> output;

  std::string::size_type prev_pos = 0, pos = 0;

  while ((pos = s.find(separator, pos)) != std::string::npos)
  {
    std::string substring(s.substr(prev_pos, pos - prev_pos));

    output.push_back(substring);

    prev_pos = ++pos;
  }

  output.push_back(s.substr(prev_pos, pos - prev_pos)); // Last word

  return output;
}

bool readStateFromserial(serial::Serial &ser, state &st)
{
  if (ser.available())
  {
    std::string data;
    std::string c;

    // cout << "Reading from serial port" << endl;
    c = "";
    // synchronisation
    while (c != "$")
    {
      c = ser.read(1);
    }
    data += c;
    do
    {
      c = ser.read(1);
      if ((c.length()) && (c != "$") && (c != " ") && (c != "\t") && (c != "\n"))
        data += c;
    } while (c != "\n");

    if (data.size() > 2)
    {
      // cout << "Read: " << data << endl;
      sscanf(data.c_str(), "$%f,%f,%f,%d#", &st.x_pos, &st.y_pos, &st.z_pos, &st.tool_pos);
      return true;
    }
    else
      return false;
  }
  return false;
}

void printState(const state &st)
{
  cout << "*****************************************" << endl;
  cout << "x_pos[mm]:" << st.x_pos << endl;
  cout << "y_pos[mm]:" << st.y_pos << endl;
  cout << "z_pos[mm]:" << st.z_pos << endl;
  cout << "tool_pos[]:" << st.tool_pos << endl;
}

void *websocket_server_thread(void *arg)
{
  printf("This is websocket_server_thread()\n");

    //Deploy a websocket server here and wait for connections sending the state at an specific rate
	//Create the event loop for the main thread, and the WebSocket server
	asio::io_service mainEventLoop;
	WebsocketServer server;
	
	//Register our network callbacks, ensuring the logic is run on the main thread's event loop
	server.connect([&mainEventLoop, &server](ClientConnection conn)
	{
		mainEventLoop.post([conn, &server]()
		{
			std::clog << "Connection opened." << std::endl;
			std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;
			
			//Send a hello message to the client
			//server.sendMessage(conn, "hello", Json::Value());
		});
	});
  
	server.disconnect([&mainEventLoop, &server](ClientConnection conn)
	{
		mainEventLoop.post([conn, &server]()
		{
			std::clog << "Connection closed." << std::endl;
			std::clog << "There are now " << server.numConnections() << " open connections." << std::endl;
		});
	});

	server.message("message", [&mainEventLoop, &server](ClientConnection conn, const Json::Value& args)
	{
		mainEventLoop.post([conn, args, &server]()
		{
			//std::clog << "message handler on the main thread" << std::endl;
			//std::clog << "Message payload:" << std::endl;
			//for (auto key : args.getMemberNames()) {
			//	std::clog << "\t" << key << ": " << args[key].asString() << std::endl;
			//}
			
			//Echo the message pack to the client
			server.sendMessage(conn, "message", args);
		});
	});
	
	//Start the networking thread
	std::thread serverThread([&server]() {
    try {
 		   server.run(cfg.websocket_port);
    } catch (websocketpp::exception const & e) {
        std::cout << e.what() << std::endl;
    } catch (...) {
        std::cout << "other exception" << std::endl;
    }
	});
	
	//Start a keyboard input thread that reads from stdin
	std::thread inputThread([&server, &mainEventLoop]()
	{
		string input;
    state state_copy;

		while (!done)
		{
      if (mutex.try_lock())
      {
        state_copy = machine_state;
        printState(state_copy);
        mutex.unlock();

			  //Broadcast the input to all connected clients (is sent on the network thread)
			  Json::Value payload;
			  payload["x_pos"] = std::to_string(state_copy.x_pos);
			  payload["y_pos"] = std::to_string(state_copy.y_pos);
			  payload["z_pos"] = std::to_string(state_copy.z_pos);
			  payload["tool_pos"] = std::to_string(state_copy.tool_pos);

			  server.broadcastMessage("machineState", payload);
			
			  //Debug output on the main thread
			  //mainEventLoop.post([]() {
				//  std::clog << "Got machine state" << std::endl;
			  //});
      }
      usleep(100000);
		}
    cout << "Exiting client thread" << endl;
    mainEventLoop.stop();
	});
	
	//Start the event loop for the main thread
	asio::io_service::work work(mainEventLoop);
	mainEventLoop.run();

  cout << "Exiting websocket thread" << endl;
  pthread_exit((void *)"'Exit from Websocket thread'");
}

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
  //--------------------------------------------------------------------------------

  pthread_create(&ws_thread, NULL, websocket_server_thread, NULL);

  signal(SIGINT, &sigsegv_handler);

  // ------------- Serial port stuff ------------
  serial::Serial ser_;

  try
  {
    ser_.setPort(cfg.serial_port);
    ser_.setBaudrate(cfg.serial_baudrate);
    serial::Timeout to = serial::Timeout::simpleTimeout(cfg.serial_timeout);
    ser_.setTimeout(to);
    ser_.open();
    ser_.flush();
  }
  catch (serial::IOException &e)
  {
    cout << "Unable to open port " << endl;
    exit(-1);
  }

  if (ser_.isOpen())
  {
    cout << "Serial Port initialized" << endl;
  }
  else
  {
    exit(-1);
  }

  while (!done)
  {
    if (mutex.try_lock())
    {
      while (readStateFromserial(ser_, machine_state))
      {
        //cout << "Got state from machine" << endl;
      }

      mutex.unlock();
    }
    else
    {
      // can't get lock to modify 'job_shared'
      // but there is some other work to do
    }

    usleep(100000);
  }

  ser_.close();

  // Waiting for websocket thread to finish
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
