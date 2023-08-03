#ifndef SERIALPORT_H
#define SERIALPORT_H

#include <iostream>
#include <string>

#include <libserialport_internal.h>
#include <libserialport.h>

class SerialPort
{
public:
    SerialPort(
        const std::string &nome,
        int baudrate = 9600,
        int bits = 8,
        sp_flowcontrol flowcontrol = SP_FLOWCONTROL_NONE,
        sp_parity parity = SP_PARITY_NONE,
        int stopbits = 1
    );
    ~SerialPort();
    SerialPort(const SerialPort &) = delete;
    SerialPort& operator=(const SerialPort &) = delete;
    SerialPort(SerialPort &&) = default;
    SerialPort& operator=(SerialPort &&) = default;

    uint8_t read();
    size_t read(void *buf, size_t quantidade, unsigned int timeout = 0);
    size_t read_non_blocking(void *buf, size_t quantidade);
    size_t write(void *buf, size_t quantidade, unsigned int timeout = 0);
    size_t write_non_blocking(void *buf, size_t quantidade);

    const std::string &portName() const;
private:
    sp_port *mPorta;
    std::string mNomePorta;
};

#endif  // SERIALPORT_H