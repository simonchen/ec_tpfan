#include <map>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <windows.h>

#include "ec.hpp"
#include "driver.hpp"

EmbeddedController::EmbeddedController(
    BYTE scPort,
    BYTE dataPort,
    BYTE endianness,
    UINT16 retry,
    UINT16 timeout)
{
    this->scPort = scPort;
    this->dataPort = dataPort;
    this->endianness = endianness;
    this->retry = retry;
    this->timeout = timeout;

    this->driver = Driver();
    if (this->driver.initialize())
        this->driverLoaded = TRUE;

    this->driverFileExist = driver.driverFileExist;
}

VOID EmbeddedController::close()
{
    this->driver.deinitialize();
    this->driverLoaded = FALSE;
}

EC_DUMP EmbeddedController::dump()
{
    EC_DUMP _dump;
    for (UINT16 column = 0x00; column <= 0xF0; column += 0x10)
        for (UINT16 row = 0x00; row <= 0x0F; row++)
        {
            UINT16 address = column + row;
            _dump.insert(std::pair<BYTE, BYTE>(address, this->readByte(address)));
        }

    return _dump;
}

VOID EmbeddedController::printDump()
{
    std::stringstream stream;
    stream << std::hex << std::uppercase << std::setfill('0')
           << " # | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F" << std::endl
           << "---|------------------------------------------------" << std::endl
           << "00 | ";

    for (auto const &[address, value] : this->dump())
    {
        UINT16 nextAddress = address + 0x01;
        stream << std::setw(2) << (UINT16)value << " ";
        if (nextAddress % 0x10 == 0x00) // End of row
            stream << std::endl
                   << nextAddress << " | ";
    }

    std::string result = stream.str();
    std::cout << std::endl
              << result.substr(0, result.size() - 7) // Removing last 7 characters
              << std::endl;
}

VOID EmbeddedController::saveDump(std::string output)
{
    std::ofstream file(output, std::ios::out | std::ios::binary);
    if (file)
    {
        for (auto const &[address, value] : this->dump())
            file << this->readByte(address);
        file.close();
    }
}

BYTE EmbeddedController::readByte(BYTE bRegister)
{
    BYTE result = 0x00;
    this->operation(READ, bRegister, &result);
    return result;
}

WORD EmbeddedController::readWord(BYTE bRegister)
{
    BYTE firstByte = 0x00;
    BYTE secondByte = 0x00;
    WORD result = 0x00;

    if (this->operation(READ, bRegister, &firstByte) &&
        this->operation(READ, bRegister + 0x01, &secondByte))
    {
        if (endianness == BIG_ENDIAN)
            std::swap(firstByte, secondByte);
        result = firstByte | (secondByte << 8);
    }

    return result;
}

DWORD EmbeddedController::readDword(BYTE bRegister)
{
    BYTE firstByte = 0x00;
    BYTE secondByte = 0x00;
    BYTE thirdByte = 0x00;
    BYTE fourthByte = 0x00;
    DWORD result = 0x00;

    if (this->operation(READ, bRegister, &firstByte) &&
        this->operation(READ, bRegister + 0x01, &secondByte) &&
        this->operation(READ, bRegister + 0x02, &thirdByte) &&
        this->operation(READ, bRegister + 0x03, &fourthByte))
    {
        if (endianness == BIG_ENDIAN)
        {
            std::swap(firstByte, fourthByte);
            std::swap(secondByte, thirdByte);
        }
        result = firstByte |
                 (secondByte << 8) |
                 (thirdByte << 16) |
                 (fourthByte << 24);
    }

    return result;
}

BOOL EmbeddedController::writeByte(BYTE bRegister, BYTE value)
{
    return this->operation(WRITE, bRegister, &value);
}

BOOL EmbeddedController::writeWord(BYTE bRegister, WORD value)
{
    BYTE firstByte = value & 0xFF;
    BYTE secondByte = value >> 8;

    if (endianness == BIG_ENDIAN)
        std::swap(firstByte, secondByte);

    if (this->operation(WRITE, bRegister, &firstByte) &&
        this->operation(WRITE, bRegister + 0x01, &secondByte))
        return TRUE;
    return FALSE;
}

BOOL EmbeddedController::writeDword(BYTE bRegister, DWORD value)
{
    BYTE firstByte = value & 0xFF;
    BYTE secondByte = (value >> 8) & 0xFF;
    BYTE thirdByte = (value >> 16) & 0xFF;
    BYTE fourthByte = value >> 24;

    if (endianness == BIG_ENDIAN)
    {
        std::swap(firstByte, fourthByte);
        std::swap(secondByte, thirdByte);
    }

    if (this->operation(WRITE, bRegister, &firstByte) &&
        this->operation(WRITE, bRegister + 0x01, &secondByte) &&
        this->operation(WRITE, bRegister + 0x02, &thirdByte) &&
        this->operation(WRITE, bRegister + 0x03, &fourthByte))
        return TRUE;
    return FALSE;
}

BOOL EmbeddedController::operation(BYTE mode, BYTE bRegister, BYTE *value)
{
    BOOL isRead = mode == READ;
    BYTE operationType = isRead ? RD_EC : WR_EC;

    for (UINT16 i = 0; i < this->retry; i++)
        if (this->status(EC_IBF)) // Wait until IBF is free
        {
            this->driver.writeIoPortByte(this->scPort, operationType); // Write operation type to the Status/Command port
            if (this->status(EC_IBF))                                  // Wait until IBF is free
            {
                this->driver.writeIoPortByte(this->dataPort, bRegister); // Write register address to the Data port
                if (this->status(EC_IBF))                                // Wait until IBF is free
                    if (isRead)
                    {
                        if (this->status(EC_OBF)) // Wait until OBF is full
                        {
                            *value = this->driver.readIoPortByte(this->dataPort); // Read from the Data port
                            return TRUE;
                        }
                    }
                    else
                    {
                        this->driver.writeIoPortByte(this->dataPort, *value); // Write to the Data port
                        return TRUE;
                    }
            }
        }

    return FALSE;
}

BOOL EmbeddedController::status(BYTE flag)
{
    BOOL done = flag == EC_OBF ? 0x01 : 0x00;
    for (UINT16 i = 0; i < this->timeout; i++)
    {
        BYTE result = this->driver.readIoPortByte(this->scPort);
        // First and second bit of returned value represent
        // the status of OBF and IBF flags respectively
        if (((done ? ~result : result) & flag) == 0)
            return TRUE;
    }

    return FALSE;
}


int main(int argc, char** argv)
{
    int port = 0, value = 0;
    if (argc < 2)
    {
        printf("Usage: [-dump] [-rpm] [-r {port}] [-w {port} {value}]\r\n\r\n \
        -dump show data from EC register from 0x00~0xff \n \
        -rpm show Fan RPM \n \
        -r read byte data from port (e.g, -r 0x2f | this will read byte from 0x2f) \n \
        -w write byte data to port (e.g, -w 0x2f 0x80 | this will write 0x80 to 0x2f) ");
        return 0;
    }
    else
    {
        EmbeddedController ec = EmbeddedController(EC_SC, EC_DATA, LITTLE_ENDIAN, 5, 100);
        if (!ec.driverFileExist || !ec.driverLoaded)
        {
            printf("Failed to load driver!\n");
            ec.close();
            return 0;
        }
        if (strcmp(argv[1],"-dump") == 0)
        {
            ec.printDump();
        }
        else if (strcmp(argv[1],"-r") == 0 && argc > 2)
        {
            sscanf_s(argv[2], "%02x", &port);
            byte r = ec.readByte(0x2f);
            printf("0x%x\n", r);
        }
        else if (strcmp(argv[1], "-w") == 0 && argc > 3)
        {
            sscanf_s(argv[2], "0x%02x", &port);
            sscanf_s(argv[3], "0x%02x", &value);
            if (ec.writeByte(port, value))
                printf("succeeded\n");
            else
                printf("failed\n");

        }
        else if (strcmp(argv[1], "-rpm") == 0) 
        {
            DWORD rpm = ec.readWord(0x84);
            printf("Fan RPM: %u", rpm);
        }
        ec.close();
    }
    

    // Making sure driver file loaded successfully
    /*
    if (ec.driverFileExist && ec.driverLoaded)
    {
        // Your rest of code palces in here
        // ...
        ec.printDump();
        BYTE ac = ec.readByte(0x2f);
        //ac = ec.readByte(0x2f);
        int fanSpeed = ec.readWord(0x84);
        //ec.writeByte(0x2f, 0x80);
        //BYTE ac_new = ec.readByte(0x2f);

        // Free up the resources at the end
        ec.close();
    }
    */

    return 0;
}