#include "ScreenShares.h"
#include "Config.h"
#include "Settings.h"
#include "UI.h"
#include "Net.h"
#include <ArduinoJson.h>

static uint32_t s_lastUpdate = 0;
static const uint32_t REFRESH_MS = 600000; // 10 minutes



struct SharePrice {
  char ticker[10];
  float price;
  bool valid;
};
static SharePrice s_prices[5];
static float s_btcPrice = 0.0f;
static bool s_btcValid = false;


static void fetchPrices() {
  const char* tickers = Settings_WatchCallsign(); // Note: Settings_SharesTickers() is what we added earlier. Let's use it properly.
  const char* sharesStr = Settings_SharesTickers();
  if (!sharesStr || !sharesStr[0]) {
    sharesStr = "AAPL,MSFT,GOOGL,AMZN,META"; 
  }
  
  // Create a copy of the string to tokenize it
  char t_buf[128];
  strncpy(t_buf, sharesStr, sizeof(t_buf)-1);
  t_buf[sizeof(t_buf)-1] = '\0';
  
  char* t = strtok(t_buf, ", ");
  int idx = 0;
  
  while (t != NULL && idx < 5) {
    String url = String("https://finnhub.io/api/v1/quote?symbol=") + t + "&token=dak275pr01qrg9hqebd0dak275pr01qrg9hqebdg";
    //String url = String("https://query1.finance.yahoo.com/v8/finance/chart/") + t + "?interval=1d&range=1d";
    // String url = String("https://finnhub.io/api/v1/quote?symbol=") + t + "&token=dak275pr01qrg9hqebd0dak275pr01qrg9hqebdg";
    // String url = "https://query1.finance.yahoo.com/v8/finance/chart/" + String(symbol) + "?interval=1d&range=1d";
    // String url = "https://api.coingecko.com/api/v3/simple/price?ids=" + ids + "&vs_currencies=usd&include_24hr_change=true"1
    String out;
    
    // Default to invalid
    s_prices[idx].valid = false;
    strncpy(s_prices[idx].ticker, t, sizeof(s_prices[idx].ticker)-1);
    s_prices[idx].ticker[sizeof(s_prices[idx].ticker)-1] = '\0';

    if (Net_GetString(url.c_str(), out, "SHARES")) {
      JsonDocument doc;
      if (deserializeJson(doc, out) == DeserializationError::Ok) {
        // Finnhub returns current price in "c"
        if (doc.containsKey("c")) {
          s_prices[idx].price = doc["c"] | 0.0f;
          s_prices[idx].valid = s_prices[idx].price > 0;
        }
      }
    }
    
    idx++;
    t = strtok(NULL, ", ");
    delay(100);
  }
  
  // Clear the rest
  for (; idx < 5; idx++) {
    s_prices[idx].valid = false;
    s_prices[idx].ticker[0] = '\0';
  }
  
  
  // Fetch Bitcoin at the end
  String btcUrl = "https://api.coingecko.com/api/v3/simple/price?ids=bitcoin&vs_currencies=usd";
  String btcOut;
  s_btcValid = false;
  if (Net_GetString(btcUrl.c_str(), btcOut, "SHARES")) {
    JsonDocument docBtc;
    if (deserializeJson(docBtc, btcOut) == DeserializationError::Ok) {
      if (docBtc.containsKey("bitcoin") && docBtc["bitcoin"].containsKey("usd")) {
        s_btcPrice = docBtc["bitcoin"]["usd"] | 0.0f;
        s_btcValid = s_btcPrice > 0;
      }
    }
  }

  s_lastUpdate = millis();
}


void ScreenShares_Enter() {
  if (millis() - s_lastUpdate > REFRESH_MS || s_lastUpdate == 0) {
    fetchPrices();
  }
}

void ScreenShares_Draw() {
  gfx->fillScreen(C_BLACK);
  UI_TextCentered("AKCIE", 40, C_CYAN, 3);
  
  
  int y = 100;
  for (int i=0; i<5; i++) {
    if (s_prices[i].ticker[0] == '\0') continue; // Skip empty
    
    char buf[64];
    if (s_prices[i].valid) {
      snprintf(buf, sizeof(buf), "%s: $%.2f", s_prices[i].ticker, s_prices[i].price);
    } else {
      snprintf(buf, sizeof(buf), "%s: ---", s_prices[i].ticker);
    }
    UI_TextCentered(buf, y, C_WHITE, 3);
    y += 45;
  }
  y += 20;
  if (s_btcValid) {
    char btcBuf[64];
    snprintf(btcBuf, sizeof(btcBuf), "BTC: $%.2f", s_btcPrice);
    UI_TextCentered(btcBuf, y, C_YELLOW, 3);
  }
}


bool ScreenShares_Tick() {
  // Update every 10 mins if left open
  if (s_lastUpdate == 0 || millis() - s_lastUpdate > REFRESH_MS) {
    fetchPrices();
    return true; // redraw
  }
  return false;
}

bool ScreenShares_HandleTap(int x, int y) {
  fetchPrices();
  return true; // redraw on tap
}

void ScreenShares_Invalidate() { s_lastUpdate = 0; }

