#pragma once

#include <Wire.h>

namespace kf {

/// @brief OLED дисплей SSD1306 (128x64)
struct SSD1306 {

public:
    using u8 = uint8_t;

private:
    /// @brief Ширина дисплея в пикселях
    static constexpr u8 screen_width{128};

    /// @brief Высота дисплея в пикселях
    static constexpr u8 screen_height{64};

    /// @brief Максимальный индекс столбца
    static constexpr u8 max_x{screen_width - 1};

    /// @brief Количество страниц (высота/8)
    static constexpr u8 pages{(screen_height + 7) / 8};

    /// @brief Максимальный индекс страницы
    static constexpr u8 max_page{pages - 1};

    /// @brief Размер буфера дисплея
    static constexpr int buffer_size{screen_width * pages};

public:
    /// @brief буфер дисплея (1024 байта)
    u8 buffer[buffer_size]{};

private:
    /// @brief I2C адрес дисплея
    const u8 address;

public:
    /// @brief Конструктор с настройкой адреса
    explicit SSD1306(u8 address = 0x3C) :
        address{address} {}

    [[nodiscard]] constexpr u8 width() const { return screen_width; }// NOLINT(*-convert-member-functions-to-static)

    [[nodiscard]] constexpr u8 height() const { return screen_height; }// NOLINT(*-convert-member-functions-to-static)

    /// @brief Инициализация дисплея
    [[nodiscard]] bool init() const {
        static constexpr u8 init_commands[] = {
            CommandMode,

            // Выключение для безопасной конфигурации
            DisplayOff,

            // Установка делителя частоты
            ClockDiv, 0x80,

            // Активация внутреннего преобразователя
            ChargePump, 0x14,

            // Горизонтальный режим адресации
            AddressingMode, Horizontal,

            // Контраст по умолчанию 127
            Contrast, 0x7F,

            // Напряжение VCOM
            SetVcomDetect, 0x40,

            // Нормальная ориентация дисплея
            NormalH, NormalV,

            // Включение дисплея
            DisplayOn,

            // Конфигурация выводов (128x64)
            SetComPins, 0x12,

            // Мультиплексирование (64 строки)
            SetMultiplex, 0x3F};

        if (not Wire.begin()) { return false; }

        Wire.beginTransmission(address);

        const auto written = Wire.write(init_commands, sizeof(init_commands));
        if (sizeof(init_commands) != written) { return false; }

        const u8 end_transmission_code = Wire.endTransmission();
        return 0 == end_transmission_code;
    }

    /// @brief Установка контрастности
    void setContrast(u8 value) const {
        Wire.beginTransmission(address);
        Wire.write(CommandMode);
        Wire.write(Contrast);
        Wire.write(value);
        Wire.endTransmission();
    }

    /// @brief Включение/выключение питания
    void setPower(bool on) {
        sendCommand(on ? DisplayOn : DisplayOff);
    }

    /// @brief Отражение по горизонтали
    void flipHorizontal(bool flip) {
        sendCommand(flip ? FlipH : NormalH);
    }

    /// @brief Отражение по вертикали
    void flipVertical(bool flip) {
        sendCommand(flip ? FlipV : NormalV);
    }

    /// @brief Инверсия цветов
    void invert(bool invert) {
        sendCommand(invert ? InvertDisplay : NormalDisplay);
    }

    /// @brief Отправить буфер на дисплей
    void flush() {
        static constexpr auto packet_size = 64;// Была замечена максимальная производительность на ESP32

        static constexpr u8 set_area_commands[] = {
            CommandMode,
            // Установка окна на весь дисплей
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
    /// @brief Команды управления SSD1306
    enum Command : u8 {
        /// @brief Выключение дисплея
        DisplayOff = 0xAE,

        /// @brief Включение дисплея
        DisplayOn = 0xAF,

        //

        /// @brief Режим команд
        CommandMode = 0x00,

        /// @brief Режим одной команды
        OneCommandMode = 0x80,

        /// @brief Режим данных
        DataMode = 0x40,

        //

        /// @brief Установка режима адресации
        AddressingMode = 0x20,

        /// @brief Горизонтальная адресация
        Horizontal = 0x00,

        /// @brief Вертикальная адресация
        Vertical = 0x01,

        //

        /// @brief Обычная вертикальная ориентация
        NormalV = 0xC8,

        /// @brief Отраженная вертикальная ориентация
        FlipV = 0xC0,

        /// @brief Обычная горизонтальная ориентация
        NormalH = 0xA1,

        /// @brief Отраженная горизонтальная ориентация
        FlipH = 0xA0,

        //

        /// @brief Установка контрастности
        Contrast = 0x81,

        /// @brief Настройка пинов COM
        SetComPins = 0xDA,

        /// @brief Настройка VCOM
        SetVcomDetect = 0xDB,

        /// @brief Делитель частоты
        ClockDiv = 0xD5,

        /// @brief Установка мультиплексирования
        SetMultiplex = 0xA8,

        /// @brief Установка столбцов
        ColumnAddr = 0x21,

        /// @brief Установка страниц
        PageAddr = 0x22,

        /// @brief Управление charge pump
        ChargePump = 0x8D,

        // Режимы отображения

        /// @brief Обычный режим отображения
        NormalDisplay = 0xA6,

        /// @brief Инверсный режим отображения
        InvertDisplay = 0xA7
    };

    /// @brief Отправка одиночной команды
    void sendCommand(Command command) const {
        Wire.beginTransmission(address);
        Wire.write(OneCommandMode);
        Wire.write(static_cast<u8>(command));
        Wire.endTransmission();
    }
};

}// namespace kf