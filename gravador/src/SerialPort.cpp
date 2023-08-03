#include "SerialPort.h"

#include <stdexcept>

SerialPort::SerialPort(
    const std::string &nome, int baudrate, int bits,
    sp_flowcontrol flowcontrol, sp_parity parity,
    int stopbits) : mPorta(nullptr), mNomePorta(nome)
{
    sp_port *novaPorta;
    sp_return erro;
    if((erro = sp_get_port_by_name(nome.c_str(), &novaPorta)) != SP_OK)
        throw std::runtime_error("Port not found");

    if((erro = sp_open(novaPorta, SP_MODE_READ_WRITE)) != SP_OK)
        throw std::runtime_error("Can't open port");

    sp_port_config *config;
    sp_new_config(&config);
    sp_set_config_baudrate(config, baudrate);
    sp_set_config_bits(config, bits);
    sp_set_config_flowcontrol(config, flowcontrol);
    sp_set_config_parity(config, parity);
    sp_set_config_stopbits(config, stopbits);
    erro = sp_set_config(novaPorta, config);
    sp_free_config(config);
    if(erro != SP_OK)
    {
        sp_close(novaPorta);
        sp_free_port(novaPorta);
        throw std::runtime_error("Unable to configure port");
    }

    mPorta = novaPorta;
}

SerialPort::~SerialPort()
{
    if(mPorta != nullptr)
    {
        sp_close(mPorta);
        sp_free_port(mPorta);
    }
}

uint8_t SerialPort::read()
{
    uint8_t valor;
    sp_blocking_read(mPorta, &valor, 1, 0);
    return valor;
}

size_t SerialPort::read(void *buf, size_t quantidade, unsigned int timeout)
{
    return sp_blocking_read(mPorta, buf, quantidade, timeout);
}

size_t SerialPort::read_non_blocking(void *buf, size_t quantidade)
{
    return sp_nonblocking_read(mPorta, buf, quantidade);
}

size_t SerialPort::write(void *buf, size_t quantidade, unsigned int timeout)
{
    return sp_blocking_write(mPorta, buf, quantidade, timeout);
}

size_t SerialPort::write_non_blocking(void *buf, size_t quantidade)
{
    return sp_nonblocking_write(mPorta, buf, quantidade);
}

const std::string &SerialPort::portName() const
{
    return mNomePorta;
}