#include <iostream>
#include <unistd.h>

#include "serial_comm.h"

using std::cout;
using std::endl;
using std::string;

SerialComm::SerialComm(const string &serial_port, int serial_baudrate, int serial_timeout)
{
    serial_port_ = serial_port;
    serial_baudrate_ = serial_baudrate;
    serial_timeout_ = serial_timeout;
}

SerialComm::~SerialComm()
{
    ser_.close();
}

bool SerialComm::init(void)
{
    ser_.setPort(serial_port_);
    ser_.setBaudrate(serial_baudrate_);
    serial::Timeout to = serial::Timeout::simpleTimeout(serial_timeout_);
    ser_.setTimeout(to);
    ser_.open();
    ser_.flush();

    if (ser_.isOpen())
    {
        cout << "Serial Port initialized" << endl;
        return true;
    }
    else
    {
        return false;
    }
}

bool SerialComm::readStateFromserial(state &st)
{
    if (ser_.available())
    {
        std::string data;
        std::string c;

        // cout << "Reading from serial port" << endl;
        c = "";
        // synchronisation
        while (c != "$")
        {
            c = ser_.read(1);
        }
        data += c;
        do
        {
            c = ser_.read(1);
            if ((c.length()) && (c != "$") && (c != " ") && (c != "\t") && (c != "\n"))
                data += c;
            // cout << "read " << c.length() << " bytes" << endl;
        } while (c != "\n");

        // cout << "data length: " << data.size() << endl;

        if (data.size() > 2)
        {
            // cout << "Read: " << data << endl;
            sscanf(data.c_str(), "$%f,%f,%f,%f,%f,%f,%d#", &st.x_pos, &st.y_pos, &st.z_pos, &st.x_ref, &st.y_ref, &st.z_ref, &st.tool_pos);
            return true;
        }
        else
            return false;
    }
    return false;
}

bool SerialComm::readSerialACK(void)
{
    while (!ser_.available())
        usleep(10000);

    std::string data;
    std::string c;

    // cout << "Reading from serial port" << endl;
    c = "";
    // synchronisation
    while (c != "$")
    {
        c = ser_.read(1);
    }
    data += c;
    do
    {
        c = ser_.read(1);
        if ((c.length()) && (c != "$") && (c != " ") && (c != "\t") && (c != "\n"))
            data += c;
        // cout << "read " << c.length() << " bytes" << endl;
    } while (c != "\n");

    if (data.substr(0, 4) == "$ack")
    {
        return true;
    }
    else
    {
        printf("BAD ACK. Received: %s\n", data.substr(0, 4).c_str());
        return false;
    }
    return false;
}

void SerialComm::sendXYPosRef(state &st)
{
    char buff[255];
    int count = 5;

    // cout << "Sending XY ref pos. x:" << st.x_ref << " y:" << st.y_ref  << endl;
    sprintf(buff, "$xy,%.3f,%.3f\n", st.x_ref, st.y_ref);
    ser_.write(string(buff));
    while (!readSerialACK() && count)
    {
        usleep(10000);
        count--;
    }
    if (!count)
        cout << "Didn't receive ACK after positining command!!!!!" << endl;
}

void SerialComm::sendPosRefs(state &st)
{
    char buff[255];
    int count = 3;

    cout << "Sending ref pos. x:" << st.x_ref << " y:" << st.y_ref << " z:" << st.z_ref << " tool:" << st.tool_pos << endl;
    sprintf(buff, "$p,%.3f,%.3f,%.3f,%d\n", st.x_ref, st.y_ref, st.z_ref, st.tool_pos);
    ser_.write(string(buff));
    while (!readSerialACK() && count)
    {
        usleep(10000);
        count--;
    }
    if (!count)
        cout << "Didn't receive ACK after positining command!!!!!" << endl;
}