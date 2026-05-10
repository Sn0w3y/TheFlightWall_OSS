/*
Purpose: Render flight info on a HUB75 RGB LED matrix panel via ESP32 I2S/DMA.
Responsibilities:
- Initialize the HUB75 panel based on HardwareConfiguration.
- Render a bordered, three-line flight card and a minimal loading screen.
- Cycle through multiple flights at a configurable interval.
Inputs: FlightInfo list; UserConfiguration (colors/brightness), TimingConfiguration (cycle),
        HardwareConfiguration (panel dimensions, pin map, FM6126A flag).
Outputs: Visual output to the HUB75 panel (DMA refresh runs in the background).
*/
#include "adapters/NeoMatrixDisplay.h"

#include <Adafruit_GFX.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include "config/UserConfiguration.h"
#include "config/HardwareConfiguration.h"
#include "config/TimingConfiguration.h"

NeoMatrixDisplay::NeoMatrixDisplay() {}

NeoMatrixDisplay::~NeoMatrixDisplay()
{
    if (_matrix)
    {
        delete _matrix;
        _matrix = nullptr;
    }
}

bool NeoMatrixDisplay::initialize()
{
    _matrixWidth = HardwareConfiguration::DISPLAY_MATRIX_WIDTH;
    _matrixHeight = HardwareConfiguration::DISPLAY_MATRIX_HEIGHT;

    HUB75_I2S_CFG::i2s_pins pins = {
        HardwareConfiguration::HUB75_R1, HardwareConfiguration::HUB75_G1, HardwareConfiguration::HUB75_B1,
        HardwareConfiguration::HUB75_R2, HardwareConfiguration::HUB75_G2, HardwareConfiguration::HUB75_B2,
        HardwareConfiguration::HUB75_A, HardwareConfiguration::HUB75_B, HardwareConfiguration::HUB75_C,
        HardwareConfiguration::HUB75_D, HardwareConfiguration::HUB75_E,
        HardwareConfiguration::HUB75_LAT, HardwareConfiguration::HUB75_OE, HardwareConfiguration::HUB75_CLK};

    HUB75_I2S_CFG mxconfig(
        HardwareConfiguration::DISPLAY_PANEL_WIDTH,
        HardwareConfiguration::DISPLAY_PANEL_HEIGHT,
        HardwareConfiguration::DISPLAY_PANEL_CHAIN,
        pins);

    if (HardwareConfiguration::HUB75_USES_FM6126A)
    {
        mxconfig.driver = HUB75_I2S_CFG::FM6126A;
    }

    _matrix = new MatrixPanel_I2S_DMA(mxconfig);
    if (!_matrix->begin())
    {
        Serial.println("NeoMatrixDisplay: HUB75 begin() failed");
        return false;
    }
    _matrix->setBrightness8(UserConfiguration::DISPLAY_BRIGHTNESS);
    _matrix->setTextWrap(false);
    _matrix->setTextSize(1);
    clear();
    _currentFlightIndex = 0;
    _lastCycleMs = millis();
    return true;
}

void NeoMatrixDisplay::clear()
{
    if (_matrix)
    {
        _matrix->fillScreen(0);
    }
}

String NeoMatrixDisplay::makeFlightLine(const FlightInfo &f)
{
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : f.operator_icao);
    if (airline.length() == 0)
    {
        airline = f.operator_code;
    }
    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    String route = origin + "-" + dest;
    String type = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;
    String ident = f.ident.length() ? f.ident : f.ident_icao;
    String line = airline;
    if (ident.length())
    {
        line += " ";
        line += ident;
    }
    if (type.length())
    {
        line += " ";
        line += type;
    }
    if (route.length() > 1)
    {
        line += " ";
        line += route;
    }
    return line;
}

void NeoMatrixDisplay::drawTextLine(int16_t x, int16_t y, const String &text, uint16_t color)
{
    _matrix->setCursor(x, y);
    _matrix->setTextColor(color);
    for (size_t i = 0; i < (size_t)text.length(); ++i)
    {
        _matrix->write(text[i]);
    }
}

String NeoMatrixDisplay::truncateToColumns(const String &text, int maxColumns)
{
    if ((int)text.length() <= maxColumns)
        return text;
    if (maxColumns <= 3)
        return text.substring(0, maxColumns);
    return text.substring(0, maxColumns - 3) + String("...");
}

void NeoMatrixDisplay::displaySingleFlightCard(const FlightInfo &f)
{
    const uint16_t borderColor = _matrix->color565(UserConfiguration::TEXT_COLOR_R,
                                                   UserConfiguration::TEXT_COLOR_G,
                                                   UserConfiguration::TEXT_COLOR_B);
    _matrix->drawRect(0, 0, _matrixWidth, _matrixHeight, borderColor);

    // Default 6x8 GFX font (5x7 glyphs + 1px spacing)
    const int charWidth = 6;
    const int charHeight = 8;
    const int padding = 2;
    const int innerWidth = _matrixWidth - 2 - (2 * padding);
    const int innerHeight = _matrixHeight - 2 - (2 * padding);
    const int maxCols = innerWidth / charWidth;

    // Lines: airline / route / aircraft
    String airline = f.airline_display_name_full.length() ? f.airline_display_name_full
                                                          : (f.operator_iata.length() ? f.operator_iata : (f.operator_icao.length() ? f.operator_icao : f.operator_code));

    String origin = f.origin.code_icao;
    String dest = f.destination.code_icao;
    String line2 = origin + String(">") + dest;

    String line3 = f.aircraft_display_name_short.length() ? f.aircraft_display_name_short : f.aircraft_code;

    String line1 = truncateToColumns(airline, maxCols);
    line2 = truncateToColumns(line2, maxCols);
    line3 = truncateToColumns(line3, maxCols);

    const uint16_t textColor = _matrix->color565(UserConfiguration::TEXT_COLOR_R,
                                                 UserConfiguration::TEXT_COLOR_G,
                                                 UserConfiguration::TEXT_COLOR_B);
    const int lineCount = 3;
    const int lineSpacing = 1;
    const int totalTextHeight = lineCount * charHeight + (lineCount - 1) * lineSpacing;
    const int topOffset = 1 + padding + (innerHeight - totalTextHeight) / 2;
    const int16_t startX = 1 + padding;

    int16_t y = topOffset;
    drawTextLine(startX, y, line1, textColor);
    y += charHeight + lineSpacing;
    drawTextLine(startX, y, line2, textColor);
    y += charHeight + lineSpacing;
    drawTextLine(startX, y, line3, textColor);
}

void NeoMatrixDisplay::displayFlights(const std::vector<FlightInfo> &flights)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    if (!flights.empty())
    {
        const unsigned long now = millis();
        const unsigned long intervalMs = TimingConfiguration::DISPLAY_CYCLE_SECONDS * 1000UL;

        if (flights.size() > 1)
        {
            if (now - _lastCycleMs >= intervalMs)
            {
                _lastCycleMs = now;
                _currentFlightIndex = (_currentFlightIndex + 1) % flights.size();
            }
        }
        else
        {
            _currentFlightIndex = 0;
        }

        const size_t index = _currentFlightIndex % flights.size();
        displaySingleFlightCard(flights[index]);
    }
    else
    {
        displayLoadingScreen();
    }
}

void NeoMatrixDisplay::displayLoadingScreen()
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    const uint16_t borderColor = _matrix->color565(255, 255, 255);
    _matrix->drawRect(0, 0, _matrixWidth, _matrixHeight, borderColor);

    const int charWidth = 6;
    const int charHeight = 8;
    const String loadingText = "...";
    const int textWidth = loadingText.length() * charWidth;

    const int16_t x = (_matrixWidth - textWidth) / 2;
    const int16_t y = (_matrixHeight - charHeight) / 2 - 2;

    const uint16_t textColor = _matrix->color565(UserConfiguration::TEXT_COLOR_R,
                                                 UserConfiguration::TEXT_COLOR_G,
                                                 UserConfiguration::TEXT_COLOR_B);
    drawTextLine(x, y, loadingText, textColor);
}

void NeoMatrixDisplay::displayMessage(const String &message)
{
    if (_matrix == nullptr)
        return;

    _matrix->fillScreen(0);

    const int charWidth = 6;
    const int charHeight = 8;

    const uint16_t textColor = _matrix->color565(UserConfiguration::TEXT_COLOR_R,
                                                 UserConfiguration::TEXT_COLOR_G,
                                                 UserConfiguration::TEXT_COLOR_B);

    const int innerWidth = _matrixWidth;
    const int maxCols = innerWidth / charWidth;
    String line = truncateToColumns(message, maxCols);

    const int16_t x = 0;
    const int16_t y = (_matrixHeight - charHeight) / 2;
    drawTextLine(x, y, line, textColor);
}

void NeoMatrixDisplay::showLoading()
{
    displayLoadingScreen();
}
