#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

//#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_PRODUCT_ID 2090

#define TFT_CS  D10
#define TFT_RST D9
#define TFT_DC  D6

#define COLOR_EDGE    0xFFFF
#define COLOR_LAMP_Y  0xFE80
#define COLOR_LAMP_G  0x07E0
#define COLOR_STOP_R  0xF800
#define COLOR_STOP_W  0xFFFF
#define COLOR_TIMER_BG    0x923D
#define COLOR_TIMER_CHAR  0xFFFF

enum DistantAspect
{
  DA_Clear1, // Double green
  DA_Clear2, // Yellow + green
  DA_Caution // Double yellow
};

class Display
{
public:
  Display() : _display(TFT_CS, TFT_DC, TFT_RST) {}

  void Init()
  {
    _display.begin();
    _display.setSPISpeed(40000000);
    _display.fillScreen(0);
 //   _display.setFont(&FreeSans9pt7b);
  }

  void DrawStopSign()
  {
    const int16_t width = _display.width();
    const int16_t centerX = width / 2;
    const int16_t halfHeight = _display.height() / 2;
    const int16_t centerY = halfHeight / 2;
    const int16_t diameter = halfHeight - 2 * _topMargin;
    const float radius = diameter / 2.f;

    _display.fillRect(0, 0, width, halfHeight, 0);
    _display.fillCircle(centerX, centerY, radius, COLOR_STOP_R);

    for (int i = 0; i < _stopLineThickness; i++)
    {
      int h = _stopLineThickness / 2 - i;
      float r = (float)h / radius;
      float angleOffset = 0.f;
      if (h != 0)
      {
        angleOffset = asin(r);
      }
      float a1 = _stopLineAngle - angleOffset;
      float a2 = _stopLineAngle + (M_PI / 2.f) - angleOffset;
      int16_t x1 = centerX + radius * sin(a1);
      int16_t y1 = centerY - radius * cos(a1);
      int16_t x2 = centerX - radius * sin(a2);
      int16_t y2 = centerY - radius * cos(a2);

      _display.drawLine(x1, y1, x2, y2, COLOR_STOP_W);
    }
  }

  void DrawDistantSignal(DistantAspect aspect)
  {
    const int16_t width = _display.width();
    const int16_t centerX = width / 2;
    const int16_t halfHeight = _display.height() / 2;
    const int16_t borderSize = halfHeight - 2 * _topMargin;
    const int16_t edgeSize = borderSize - _distCornerOffset * 2;

    _display.fillRect(0, 0, width, halfHeight, 0);
    for (int i = 0; i < _distBorderThickness - 1; i++)
    {
      // Top line
      _display.drawFastHLine(centerX - edgeSize / 2, _topMargin + i, edgeSize, COLOR_EDGE);
      // Bottom line
      _display.drawFastHLine(centerX - edgeSize / 2, _topMargin + borderSize - i, edgeSize, COLOR_EDGE);
      // Left line
      _display.drawFastVLine(centerX - borderSize / 2 + i, _topMargin + _distCornerOffset, edgeSize, COLOR_EDGE);
      // Right line
      _display.drawFastVLine(centerX + borderSize / 2 - i, _topMargin + _distCornerOffset, edgeSize, COLOR_EDGE);

      // Top left corner
      _display.drawLine(centerX - borderSize / 2 + i, _topMargin + _distCornerOffset, centerX - edgeSize / 2, _topMargin + i, COLOR_EDGE);
      // Top right corner
      _display.drawLine(centerX + edgeSize / 2, _topMargin + i, centerX + borderSize / 2 - i, _topMargin + _distCornerOffset, COLOR_EDGE);
      // Bottom left corner
      _display.drawLine(centerX - borderSize / 2 + i, _topMargin + _distCornerOffset + edgeSize, centerX - edgeSize / 2, _topMargin + borderSize - i, COLOR_EDGE);
      // Bottom right corner
      _display.drawLine(centerX + edgeSize / 2, _topMargin + borderSize - i, centerX + borderSize / 2 - i, _topMargin + _distCornerOffset + edgeSize, COLOR_EDGE);
    }

    const int16_t cellSize = (borderSize - 2 * _distBorderThickness) / 3;
    const int16_t cellH = cellSize / 2;
    const int16_t topLeftX = centerX - borderSize / 2 + _distBorderThickness;
    const int16_t topLeftY = _topMargin + _distBorderThickness;

    // Cell 0,0
    if (aspect >= DA_Clear2)
    {
      _display.fillCircle(topLeftX + cellH, topLeftY + cellH, _distLampRadius, COLOR_LAMP_Y);
    }
    // Cell 2,0
    if (aspect == DA_Caution)
    {
      _display.fillCircle(topLeftX + 2 * cellSize + cellH, topLeftY + cellH, _distLampRadius, COLOR_LAMP_Y);
    }
    // Cell 2,1
    if (aspect < DA_Caution)
    {
      _display.fillCircle(topLeftX + 2 * cellSize + cellH, topLeftY + cellSize + cellH, _distLampRadius, COLOR_LAMP_G);
    }
    // Cell 0,2
    if (aspect == DA_Clear1)
    {
      _display.fillCircle(topLeftX + cellH, topLeftY + 2 * cellSize + cellH, _distLampRadius, COLOR_LAMP_G);
    }
  }

  void ClearTimer()
  {
    if (_timerStr == "")
      return;

    const int16_t width = _display.width();
    const int16_t halfHeight = _display.height() / 2;
    _display.fillRect(0, halfHeight, width, halfHeight, 0);
    _timerStr = "";
  }

  void DrawTimer(int time)
  {
    const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
    int t = time;
    if (time < 0)
      t = 0;
    int d0 = t / 100;
    t -= (d0 * 100);
    int d1 = t / 10;
    t -= (d1 * 10);
    int d2 = t;

    String charStr = "T";
    charStr.concat(time);

    if (_timerStr == charStr)
      return;

    _timerStr = charStr;

    const int16_t halfHeight = _display.height() / 2;
    const int16_t topY = halfHeight + _timerY;
    const int16_t centerX = _display.width() / 2;
    const int16_t charH = _timerCharSize * 7;
    const int16_t charW = _timerCharSize * 5;

    DrawTimerBackground();

    _display.drawChar(centerX - 3 * charW / 2 - 3, topY + _timerH / 2 - charH / 2, digits[d0], COLOR_TIMER_CHAR, COLOR_TIMER_BG, _timerCharSize);
    _display.drawChar(centerX - charW / 2, topY + _timerH / 2 - charH / 2, digits[d1], COLOR_TIMER_CHAR, COLOR_TIMER_BG, _timerCharSize);
    _display.drawChar(centerX + charW / 2 + 3, topY + _timerH / 2 - charH / 2, digits[d2], COLOR_TIMER_CHAR, COLOR_TIMER_BG, _timerCharSize);
  }

  void DrawTimerText(String text)
  {
    if (_timerStr == text)
      return;

    _timerStr = text;

    const int16_t halfHeight = _display.height() / 2;
    const int16_t topY = halfHeight + _timerY;
    const int16_t centerX = _display.width() / 2;

    DrawTimerBackground();

    _display.setTextSize(_timerCharSize);
    _display.setTextColor(COLOR_TIMER_CHAR);
    int16_t x1, y1;
    uint16_t textW, textH;
    _display.getTextBounds(text, centerX - _timerW / 2, topY, &x1, &y1, &textW, &textH);
    _display.setCursor(centerX - textW / 2, topY + _timerH / 2 - textH / 2);
    _display.println(text);
  }

private:
  Adafruit_ILI9341 _display;

  String _timerStr = "";

  const int16_t _topMargin = 16;
  const int16_t _stopLineThickness = 10;
  const float _stopLineAngle = M_PI / 4.f;
  const int16_t _distCornerOffset = 14;
  const uint8_t _distBorderThickness = 6;
  const int16_t _distLampRadius = 12;

  const int16_t _textY = 16;
  const int16_t _textXMargin = 20;
  const int16_t _textH = 28;
  const int16_t _timerY = 40;
  const int16_t _timerH = 90;
  const int16_t _timerW = 160;
  const int16_t _timerR = 18;
  const int16_t _timerCharSize = 6;

  void DrawTimerBackground()
  {
    const int16_t halfHeight = _display.height() / 2;
    const int16_t topY = halfHeight;
    const int16_t centerX = _display.width() / 2;
    
    _display.fillRoundRect(centerX - _timerW / 2, topY + _timerY, _timerW, _timerH, _timerR, COLOR_TIMER_BG);
  }
};