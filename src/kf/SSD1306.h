#pragma once

#include <Wire.h>


namespace kf {

/// OLED дисплей SSD1306 (128x64)
struct SSD1306 {

public:

    using u8 = uint8_t;

private:

    /// Ширина дисплея в пикселях
    static constexpr u8 screen_width{128};

    /// Высота дисплея в пикселях
    static constexpr u8 screen_height{64};

    /// Максимальный индекс столбца
    static constexpr u8 max_x{screen_width - 1};

    /// Количество страниц (высота/8)
    static constexpr u8 pages{(screen_height + 7) / 8};

    /// Максимальный индекс страницы
    static constexpr u8 max_page{pages - 1};

    /// Размер буфера дисплея
    static constexpr int buffer_size{screen_width * pages};

public:

    /// буфер дисплея (1024 байта)
    u8 buffer[buffer_size]{};

private:

    /// I2C адрес дисплея
    const u8 address;

public:

    /// Конструктор с настройкой адреса
    explicit SSD1306(u8 address = 0x3C) :
        address{address} {}

    [[nodiscard]] constexpr u8 width() const { return screen_width; } // NOLINT(*-convert-member-functions-to-static)

    [[nodiscard]] constexpr u8 height() const { return screen_height; } // NOLINT(*-convert-member-functions-to-static)

    /// Инициализация дисплея
    [[nodiscard]] bool init() const {
        static constexpr u8 init_commands[] = {
            CommandMode,
            DisplayOff,                 // Выключение для безопасной конфигурации
            ClockDiv, 0x80,             // Установка делителя частоты
            ChargePump, 0x14,           // Активация внутреннего преобразователя
            AddressingMode, Horizontal, // Горизонтальный режим адресации
            Contrast, 0x7F,             // Контраст по умолчанию 127
            SetVcomDetect, 0x40,        // Напряжение VCOM
            NormalH, NormalV,           // Нормальная ориентация дисплея
            DisplayOn,                  // Включение дисплея
            SetComPins, 0x12,           // Конфигурация выводов (128x64)
            SetMultiplex, 0x3F          // Мультиплексирование (64 строки)
        };
        if (not Wire.begin()) { return false; }

        Wire.beginTransmission(address);

        const auto written = Wire.write(init_commands, sizeof(init_commands));
        if (sizeof(init_commands) != written) { return false; }

        const u8 end_transmission_code = Wire.endTransmission();
        return 0 == end_transmission_code;
    }

    /// Установка контрастности
    void setContrast(u8 value) const {
        Wire.beginTransmission(address);
        Wire.write(CommandMode);
        Wire.write(Contrast);
        Wire.write(value);
        Wire.endTransmission();
    }

    /// Включение/выключение питания
    void setPower(bool on) {
        sendCommand(on ? DisplayOn : DisplayOff);
    }

    /// Отражение по горизонтали
    void flipHorizontal(bool flip) {
        sendCommand(flip ? FlipH : NormalH);
    }

    /// Отражение по вертикали
    void flipVertical(bool flip) {
        sendCommand(flip ? FlipV : NormalV);
    }

    /// Инверсия цветов
    void invert(bool invert) {
        sendCommand(invert ? InvertDisplay : NormalDisplay);
    }

    /// Отправить буфер на дисплей
    void flush() {
        static constexpr auto packet_size = 64; // Была замечена максимальная производительность на ESP32

        static constexpr u8 set_area_commands[] = {
            CommandMode,
            ColumnAddr, 0, max_x,
            PageAddr, 0, max_page,
        };

        Wire.beginTransmission(address);
        Wire.write(set_area_commands, sizeof(set_area_commands));
        Wire.endTransmission();

        for (u8 *p = buffer, *end = buffer + buffer_size; p < end; p += packet_size) {
            Wire.beginTransmission(address);
            Wire.write(Command::DataMode);
            Wire.write(p, packet_size);
            Wire.endTransmission();
        }
    }

private:

    /// Команды управления SSD1306
    enum Command : u8 {
        /// Выключение дисплея
        DisplayOff = 0xAE,
        /// Включение дисплея
        DisplayOn = 0xAF,

        /// Режим команд
        CommandMode = 0x00,
        /// Режим одной команды
        OneCommandMode = 0x80,
        /// Режим данных
        DataMode = 0x40,

        /// Установка режима адресации
        AddressingMode = 0x20,
        /// Горизонтальная адресация
        Horizontal = 0x00,
        /// Вертикальная адресация
        Vertical = 0x01,

        /// Обычная вертикальная ориентация
        NormalV = 0xC8,
        /// Отраженная вертикальная ориентация
        FlipV = 0xC0,
        /// Обычная горизонтальная ориентация
        NormalH = 0xA1,
        /// Отраженная горизонтальная ориентация
        FlipH = 0xA0,

        /// Установка контрастности
        Contrast = 0x81,
        /// Настройка пинов COM
        SetComPins = 0xDA,
        /// Настройка VCOM
        SetVcomDetect = 0xDB,
        /// Делитель частоты
        ClockDiv = 0xD5,
        /// Установка мультиплексирования
        SetMultiplex = 0xA8,
        /// Установка столбцов
        ColumnAddr = 0x21,
        /// Установка страниц
        PageAddr = 0x22,
        /// Управление charge pump
        ChargePump = 0x8D,

        /// Обычный режим отображения
        NormalDisplay = 0xA6,
        /// Инверсный режим отображения
        InvertDisplay = 0xA7
    };

    /// Отправка одиночной команды
    void sendCommand(Command command) const {
        Wire.beginTransmission(address);
        Wire.write(OneCommandMode);
        Wire.write(static_cast<u8>(command));
        Wire.endTransmission();
    }
};
} // namespace kf