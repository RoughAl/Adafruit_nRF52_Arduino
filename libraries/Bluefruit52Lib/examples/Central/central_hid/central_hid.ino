/*********************************************************************
 This is an example for our nRF52 based Bluefruit LE modules

 Pick one up today in the adafruit shop!

 Adafruit invests time and resources providing this open source code,
 please support Adafruit and open-source hardware by purchasing
 products from Adafruit!

 MIT license, check LICENSE for more information
 All text above, and the splash screen below must be included in
 any redistribution
*********************************************************************/

/*
 * This sketch demonstrate the central API(). A additional bluefruit
 * that has bleuart as peripheral is required for the demo.
 */
#include <bluefruit.h>

// Polling or callback implementation
#define POLLING       1

BLEClientHidAdafruit hid;

// Last checked report, to detect if there is changes between reports
hid_keyboard_report_t last_kbd_report = { 0 };
hid_mouse_report_t last_mse_report = { 0 };

void setup()
{
  Serial.begin(115200);

  Serial.println("Bluefruit52 Central HID (Keyboard + Mouse) Example");
  Serial.println("--------------------------------------------------\n");
  
  // Initialize Bluefruit with maximum connections as Peripheral = 0, Central = 1
  // SRAM usage required by SoftDevice will increase dramatically with number of connections
  Bluefruit.begin(0, 1);
  
  Bluefruit.setName("Bluefruit52 Central");

  // Init BLE Central Uart Serivce
  hid.begin();

  #if POLLING == 0  
  hid.setKeyboardReportCallback(keyboard_report_callback);
  #endif

  // Increase Blink rate to different from PrPh advertising mode
  Bluefruit.setConnLedInterval(250);

  // Callbacks for Central
  Bluefruit.Central.setConnectCallback(connect_callback);
  Bluefruit.Central.setDisconnectCallback(disconnect_callback);

  /* Start Central Scanning
   * - Enable auto scan if disconnected
   * - Interval = 100 ms, window = 80 ms
   * - Don't use active scan
   * - Start(timeout) with timeout = 0 will scan forever (until connected)
   */
  Bluefruit.Scanner.setRxCallback(scan_callback);
  Bluefruit.Scanner.restartOnDisconnect(true);
  Bluefruit.Scanner.setInterval(160, 80); // in unit of 0.625 ms
  Bluefruit.Scanner.useActiveScan(false);
  Bluefruit.Scanner.start(0);             // 0 = Don't stop scanning after n seconds

  Bluefruit.Scanner.filterService(hid);   // only report HID service
}

/**
 * Callback invoked when scanner pick up an advertising data
 * @param report Structural advertising data
 */
void scan_callback(ble_gap_evt_adv_report_t* report)
{
  // Connect to device
  Bluefruit.Central.connect(report);
}

/**
 * Callback invoked when an connection is established
 * @param conn_handle
 */
void connect_callback(uint16_t conn_handle)
{
  Serial.println("Connected");

  Serial.print("Discovering HID  Service ... ");

  if ( hid.discover(conn_handle) )
  {
    Serial.println("Found it");

    // HID device mostly require pairing/bonding
    if ( !Bluefruit.Gap.requestPairing(conn_handle) )
    {
      Serial.print("Failed to paired");
      return;
    }

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.hid_information.xml
    uint8_t hidInfo[4];
    hid.getHidInfo(hidInfo);

    Serial.printf("HID version: %d.%d\n", hidInfo[0], hidInfo[1]);
    Serial.print("Country code: "); Serial.println(hidInfo[2]);
    Serial.printf("HID Flags  : 0x%02X\n", hidInfo[3]);

    // BLEClientHidAdafruit currently only suports Boot Protocol Mode
    // for Keyboard and Mouse. Let's set the protocol mode on prph to Boot Mode
    hid.setProtocolMode(HID_PROTOCOL_MODE_BOOT);

    // Enable Keyboard report notification if present on prph
    if ( hid.keyboardPresent() ) hid.enableKeyboard();

    // Enable Mouse report notification if present on prph
    if ( hid.mousePresent() ) hid.enableMouse();
    
    Serial.println("Ready to receive from peripheral");
  }else
  {
    Serial.println("Found NONE");
    
    // disconect since we couldn't find bleuart service
    Bluefruit.Central.disconnect(conn_handle);
  }  
}

/**
 * Callback invoked when a connection is dropped
 * @param conn_handle
 * @param reason
 */
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;
  
  Serial.println("Disconnected");
}

void loop()
{
  
#if POLLING == 1
  // nothing to do if hid not discovered
  if ( !hid.discovered() ) return;
  
  /*------------- Polling Keyboard  -------------*/
  hid_keyboard_report_t kbd_report;

  // Get latest report
  hid.getKeyboardReport(&kbd_report);

  processKeyboardReport(&kbd_report);


  // polling interval is 5 ms
  delay(5);
#endif
  
}


void keyboard_report_callback(hid_keyboard_report_t* report)
{
  processKeyboardReport(report);
}

void processKeyboardReport(hid_keyboard_report_t* report)
{
  // Check with last report to see if there is any changes
  if ( memcmp(&last_kbd_report, report, sizeof(hid_keyboard_report_t)) )
  {
    bool shifted = false;
    
    if ( report->modifier  )
    {
      if ( report->modifier & (KEYBOARD_MODIFIER_LEFTCTRL | KEYBOARD_MODIFIER_RIGHTCTRL) )
      {
        Serial.print("Ctrl ");
      }

      if ( report->modifier & (KEYBOARD_MODIFIER_LEFTSHIFT | KEYBOARD_MODIFIER_RIGHTSHIFT) )
      {
        Serial.print("Shift ");

        shifted = true;
      }

      if ( report->modifier & (KEYBOARD_MODIFIER_LEFTALT | KEYBOARD_MODIFIER_RIGHTALT) )
      {
        Serial.print("Alt ");
      }      
    }
    
    for(uint8_t i=0; i<6; i++)
    {
      uint8_t kc = report->keycode[i];
      char ch = 0;
      
      if ( kc < 128 )
      {
        ch = shifted ? HID_KEYCODE_TO_ASCII[kc].shifted : HID_KEYCODE_TO_ASCII[kc].ascii;
      }else
      {
        // non-US keyboard !!??
      }

      // Printable
      if (ch)
      {
        Serial.print(ch);
      }
    }
  }

  // update last report
  memcpy(&last_kbd_report, report, sizeof(hid_keyboard_report_t));  
}

