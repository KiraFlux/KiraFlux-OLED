#pragma once

#include <Wire.h>


namespace kf {

struct SSD1306 {

public:

    using u8 = uint8_t; // removed Rustify dependency

    static constexpr u8 width = 128;
    static constexpr u8 height = 64;

private:

    static constexpr u8 max_x = width - 1;
    static constexpr u8 pages = (height + 7) / 8;
    static constexpr u8 max_page = pages - 1;

public:

    static constexpr auto buffer_size = width * pages;

    u8 buffer[buffer_size]{};

private:
    const u8 address;

public:

    explicit SSD1306(u8 address = 0x3C) :
        address(address) {}

    void init() const {
        Wire.begin();

        Wire.beginTransmission(address);

        static constexpr u8 init_commands[] = {
            CommandMode,
            DisplayOff,
            ClockDiv, 0x80,
            ChargePump, 0x14,
            AddressingMode, Horizontal,
            NormalH,
            NormalV,
            Contrast, 0x7F,
            SetVcomDetect, 0x40,
            NormalDisplay,
            DisplayOn,
            SetComPins, 0x12,
            SetMultiplex, 0x3F,
            ColumnAddr, 0, max_x,
            PageAddr, 0, max_page
        };

        Wire.write(init_commands, sizeof(init_commands));
        Wire.endTransmission();
    }

    void setContrast(u8 value) const {
        Wire.beginTransmission(address);
        Wire.write(CommandMode);
        Wire.write(Contrast);
        Wire.write(value);
        Wire.endTransmission();
    }

    void setPower(bool on) {
        sendCommand(on ? DisplayOn : DisplayOff);
    }

    void flipH(bool flip) {
        sendCommand(flip ? FlipH : NormalH);
    }

    void flipV(bool flip) {
        sendCommand(flip ? FlipV : NormalV);
    }

    void invert(bool invert) {
        sendCommand(invert ? InvertDisplay : NormalDisplay);
    }

    void update() {
        static constexpr auto max_i2c_packet = 64;

        Wire.beginTransmission(address);
        Wire.write(CommandMode);
        Wire.write(ColumnAddr);
        Wire.write(0);
        Wire.write(max_x);
        Wire.write(PageAddr);
        Wire.write(0);
        Wire.write(max_page);
        Wire.endTransmission();

        for (auto *p = buffer, *end = buffer + buffer_size; p < end; p += max_i2c_packet) {
            Wire.beginTransmission(address);
            Wire.write(DataMode);
            Wire.write(p, max_i2c_packet);
            Wire.endTransmission();
        }
    }

private:

    enum Command : u8 {
        DisplayOff = 0xAE,
        DisplayOn = 0xAF,

        CommandMode = 0x00,
        OneCommandMode = 0x80,
        DataMode = 0x40,

        AddressingMode = 0x20,
        Horizontal = 0x00,
        Vertical = 0x01,

        NormalV = 0xC8,
        FlipV = 0xC0,
        NormalH = 0xA1,
        FlipH = 0xA0,

        Contrast = 0x81,
        SetComPins = 0xDA,
        SetVcomDetect = 0xDB,
        ClockDiv = 0xD5,
        SetMultiplex = 0xA8,
        ColumnAddr = 0x21,
        PageAddr = 0x22,
        ChargePump = 0x8D,

        NormalDisplay = 0xA6,
        InvertDisplay = 0xA7
    };

    void sendCommand(Command command) const {
        Wire.beginTransmission(address);
        Wire.write(OneCommandMode);
        Wire.write(static_cast<u8>(command));
        Wire.endTransmission();
    }
};
}

