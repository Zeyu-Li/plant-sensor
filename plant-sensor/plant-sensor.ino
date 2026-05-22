// --- HARDWARE CONFIGURATION ---
const int MOISTURE_PIN = 13; 
const int LED_PIN = LED_BUILTIN; // Uses the on-board LED. Change to an external pin if needed!

// --- CALIBRATION THRESHOLDS ---
// TODO: YOU WILL have to fine tune these levels, this worked for me see calibration.ino calibration
const int DRY_VALUE = 2920;  // Open-air/Dry soil baseline
const int WET_VALUE = 1800;  // Submerged/Wet soil baseline
const int MOISTURE_THRESHOLD_PCT = 25; // Alert if moisture drops below this %

// --- TIMING CONFIGURATION (In Seconds) ---
const unsigned long POLLING_INTERVAL_S = 3600; // How often to wake up and check (e.g., 60 for 1 minute, 1800 for 30 mins, 3600 for 1 hour)
const unsigned long ALERT_BLINK_DURATION_S = 5; // How long to blink the LED if dry
const unsigned long ALERT_PHASE_DURATION_S = 12UL * 3600UL; // 12h of blinking per cycle
const unsigned long FULL_CYCLE_DURATION_S  = 24UL * 3600UL; // 12h alert + 12h silent

// --- PERSISTENT STATE ---
// RTC memory survives deep sleep (but not power loss, which is fine: a fresh
// boot should always start a dry spell in the alert phase). Tracks how long
// the soil has continuously been dry, used to drive the 24h alert/silent cycle.
RTC_DATA_ATTR unsigned long dryDurationS = 0;

void setup() {
  Serial.begin(115200);
  
  // Brief delay to allow Serial Monitor to attach if plugged into USB
  delay(1000); 
  Serial.println("\n--- Waking up from sleep... ---");

  // Configure the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // 1. Take a smoothed sensor reading
  int total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(MOISTURE_PIN);
    delay(10); 
  }
  int smoothedValue = total / 10;

  // 2. Calculate moisture percentage
  int moisturePercent = map(smoothedValue, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  Serial.print("Current Soil Moisture: ");
  Serial.print(moisturePercent);
  Serial.println("%");

  // 3. Action Phase: alternate 12h blink / 12h silent while dry.
  // The user gets a clear 12-hour window to notice the alert at every poll,
  // then the device stays quiet for 12 hours so it isn't perpetually annoying.
  if (moisturePercent <= MOISTURE_THRESHOLD_PCT) {
    unsigned long cyclePosition = dryDurationS % FULL_CYCLE_DURATION_S;
    bool inAlertPhase = cyclePosition < ALERT_PHASE_DURATION_S;

    Serial.print("Soil is DRY. Cycle position: ");
    Serial.print(cyclePosition / 60);
    Serial.print(" min / ");
    Serial.print(FULL_CYCLE_DURATION_S / 60);
    Serial.println(" min.");

    if (inAlertPhase) {
      Serial.println("ALERT phase: blinking LED.");

      // Calculate how many times to loop the blink to fill the target seconds
      // Each loop iteration takes 500ms total (250ms ON + 250ms OFF), so 2 blinks per second
      int totalBlinks = ALERT_BLINK_DURATION_S * 2;

      for (int i = 0; i < totalBlinks; i++) {
        digitalWrite(LED_PIN, HIGH); // Turn LED on
        delay(250);
        digitalWrite(LED_PIN, LOW);  // Turn LED off
        delay(5000);
      }
    } else {
      Serial.println("SILENT phase: skipping blink so user can rest.");
    }

    // Advance dry-time accumulator for the next wake-up.
    dryDurationS += POLLING_INTERVAL_S;
  } else {
    Serial.println("Soil is moist enough. No alert needed.");
    // Reset so the next dry spell starts fresh in the alert phase.
    dryDurationS = 0;
  }

  // 4. Sleep Phase: Calculate sleep time and go under
  Serial.print("Going to deep sleep for ");
  Serial.print(POLLING_INTERVAL_S);
  Serial.println(" seconds...");
  Serial.flush(); // Ensure all serial text is transmitted before shutting down the chip

  // Convert seconds to microseconds for the ESP32 internal timer API
  esp_sleep_enable_timer_wakeup(POLLING_INTERVAL_S * 1000000ULL);
  
  // Put the chip into deep sleep mode
  esp_deep_sleep_start();
}

void loop() {
  // This remains totally empty! Execution will never reach here because 
  // the chip deep-sleeps at the end of setup() and restarts from setup() when it wakes up.
}